#include "usb/xhci.h"
#include "pci.h"
#include "debug.h"
#include "io.h"
#include "map.h"
#include <cstring>
#include "freepage.h"
#include "future.h"
#include "usb/UsbSpec.h"
#include "usb/UsbCore.h"

constexpr uint64_t CR_CAPLENGTH = 0;
constexpr uint64_t CR_HCIVERSION = 2;
constexpr uint64_t CR_HCSPARAMS1 = 4;
constexpr uint64_t CR_HCSPARAMS2 = 8;
constexpr uint64_t CR_HCSPARAMS3 = 12;
constexpr uint64_t CR_HCCPARAMS1 = 16;
constexpr uint64_t CR_DBOFF = 20;
constexpr uint64_t CR_RTSOFF = 24;
constexpr uint64_t CR_HCCPARAMS2 = 28;

constexpr uint64_t RT_USBCMD = 0x00;
constexpr uint64_t RT_USBSTS = 0x04;
constexpr uint64_t RT_PAGESIZE = 0x08;
constexpr uint64_t RT_DNCTRL = 0x14;
constexpr uint64_t RT_CRCR = 0x18;
constexpr uint64_t RT_DCBAAP = 0x30;
constexpr uint64_t RT_CONFIG = 0x38;

constexpr uint64_t RT_ERSTSZ = 0x28;
constexpr uint64_t RT_ERDP = 0x38;
constexpr uint64_t RT_ERSTBA = 0x30;
constexpr uint64_t RT_IMAN = 0x20;
constexpr uint64_t RT_IMOD = 0x24;

constexpr uint64_t P_SC = 0x00;
constexpr uint64_t P_PMSC = 0x04;
constexpr uint64_t P_LI = 0x08;

constexpr uint64_t RT_USBSTS_HCH = 0x1;
constexpr uint64_t RT_USBCMD_RUN = 0x00000001;
constexpr uint64_t RT_USBCMD_RESET = 0x00000002;
constexpr uint64_t RT_USBCMD_INTE = 0x00000004;

enum : uint8_t {
  XHCI_COMPLETION_INVALID = 0,
  XHCI_COMPLETION_SUCCESS = 1,
  XHCI_COMPLETION_DATA_BUFFER_ERROR = 2,
  XHCI_COMPLETION_BABBLE_DETECTED_ERROR = 3,
  XHCI_COMPLETION_USB_TRANSACTION_ERROR = 4,
  XHCI_COMPLETION_TRB_ERROR = 5,
  XHCI_COMPLETION_STALL_ERROR = 6,
  XHCI_COMPLETION_RESOURCE_ERROR = 7,
  XHCI_COMPLETION_BANDWIDTH_ERROR = 8,
  XHCI_COMPLETION_NO_SLOTS_AVAILABLE_ERROR = 9,
  XHCI_COMPLETION_INVALID_STREAM_TYPE_ERROR = 10,
  XHCI_COMPLETION_SLOT_NOT_ENABLED_ERROR = 11,
  XHCI_COMPLETION_ENDPOINT_NOT_ENABLED_ERROR = 12,
  XHCI_COMPLETION_SHORT_PACKET = 13,
  XHCI_COMPLETION_RING_UNDERRUN = 14,
  XHCI_COMPLETION_RING_OVERRUN = 15,
  XHCI_COMPLETION_VF_EVENT_RING_FULL_ERROR = 16,
  XHCI_COMPLETION_PARAMETER_ERROR = 17,
  XHCI_COMPLETION_BANDWIDTH_OVERRUN_ERROR = 18,
  XHCI_COMPLETION_CONTEXT_STATE_ERROR = 19,
  XHCI_COMPLETION_NO_PING_RESPONSE_ERROR = 20,
  XHCI_COMPLETION_EVENT_RING_FULL_ERROR = 21,
  XHCI_COMPLETION_INCOMPATIBLE_DEVICE_ERROR = 22,
  XHCI_COMPLETION_MISSED_SERVICE_ERROR = 23,
  XHCI_COMPLETION_COMMAND_RING_STOPPED = 24,
  XHCI_COMPLETION_COMMAND_ABORTED = 25,
  XHCI_COMPLETION_STOPPED = 26,
  XHCI_COMPLETION_STOPPED_LENGTH_INVALID = 27,
  XHCI_COMPLETION_STOPPED_SHORT_PACKET = 28,
  XHCI_COMPLETION_MAX_EXIT_LATENCY_TOO_LARGE_ERROR = 29,
  XHCI_COMPLETION_ISOCH_BUFFER_OVERRUN = 31,
  XHCI_COMPLETION_EVENT_LOST_ERROR = 32,
  XHCI_COMPLETION_UNDEFINED_ERROR = 33,
  XHCI_COMPLETION_INVALID_STREAM_ID_ERROR = 34,
  XHCI_COMPLETION_SECONDARY_BANDWIDTH_ERROR = 35,
  XHCI_COMPLETION_SPLIT_TRANSACTION_ERROR = 36,
};

struct xhci_speed {
  bool fullduplex;
  uint8_t spd_exponent;
  uint16_t spd_mantissa;
  uint8_t linkprot = 0;
  uint8_t slottype = 0;
};

xhci_speed vtbl[16] = {
  {},
  { false, 2, 12 },
  { false, 1, 1500 },
  { false, 2, 480 },
  { true, 3, 5 },
  { true, 3, 10 },
  { true, 3, 10 },
  { true, 3, 20 },
};

struct EndpointContext {
  uint32_t a;
  uint32_t b;
  uint64_t tr_deq_ptr;
  uint32_t average;
  uint32_t res[3];
};
static_assert(sizeof(EndpointContext) == 0x20);

struct EndpointContextPair {
  EndpointContext out;
  EndpointContext in;
};

static_assert(sizeof(EndpointContextPair) == 0x40);

struct DevicePort {
  uint32_t route;
  uint32_t ports;
  uint32_t parent;
  uint32_t device;
  uint32_t pad[4];
  EndpointContext control;
  EndpointContextPair eps[15];
};

static_assert(sizeof(DevicePort) == 0x400);

struct InputContext {
  DevicePort deviceport; // Device-owned port object
  uint32_t d, a;
  uint32_t pad1[6];
  DevicePort port;       // Software-owned port object (for updates)
  DeviceDescriptor device_descriptor;
  uint8_t loopIndex;

  uint8_t buffer[0x3cd];
  xhci_command loop[64];
};

static_assert(sizeof(InputContext) == 0x1000);



static constexpr const char* plstab[16] = {
  "U0",
  "U1",
  "U2",
  "U3",
  "Disabled",
  "RxDetect",
  "Inactive",
  "Polling",
  "Recovery",
  "Hot Reset",
  "Compliance Mode",
  "Test Mode",
  "Reserved 12",
  "Reserved 13",
  "Reserved 14",
  "Resume",
};

namespace {

enum {
  TRB_FLAG_C = 0x1,
  TRB_FLAG_TC = 0x2,
  TRB_FLAG_ENT = 0x2,
  TRB_FLAG_ED = 0x4,
  TRB_FLAG_ISP = 0x4,
  TRB_FLAG_NS = 0x8,
  TRB_FLAG_CH = 0x10,
  TRB_FLAG_IOC = 0x20,
  TRB_FLAG_IDT = 0x40,
  TRB_FLAG_TSP = 0x200,
  TRB_FLAG_DC = 0x200,
  TRB_FLAG_BSR = 0x200,
  TRB_FLAG_BEI = 0x200,
  TRB_FLAG_DIR = 0x10000,
  TRB_FLAG_SIA = 0x80000000,

  TRB_TYPE_NORMAL = 0x0400,
  TRB_TYPE_SETUP_STAGE = 0x0800,
  TRB_TYPE_DATA_STAGE = 0x0c00,
  TRB_TYPE_STATUS_STAGE = 0x1000,
  TRB_TYPE_ISOCH = 0x1400,
  TRB_TYPE_LINK = 0x1800,
  TRB_TYPE_EVENT_DATA = 0x1c00,
  TRB_TYPE_TRANSFER_NOOP = 0x2000,

  TRB_TYPE_ENABLE_SLOT = 0x2400,
  TRB_TYPE_DISABLE_SLOT = 0x2800,
  TRB_TYPE_ADDRESS_DEVICE = 0x2c00,
  TRB_TYPE_CONFIGURE_ENDPOINT = 0x3000,
  TRB_TYPE_EVALUATE_CONTEXT = 0x3400,
  TRB_TYPE_RESET_ENDPOINT = 0x3800,
  TRB_TYPE_STOP_ENDPOINT = 0x3c00,
  TRB_TYPE_TR_DEQUEUE_POINTER = 0x4000,
  TRB_TYPE_RESET_DEVICE = 0x4400,
  TRB_TYPE_COMMAND_NOOP = 0x5c00,

  TRB_TYPE_TRANSFER = 0x8000,
  TRB_TYPE_COMMAND_COMPLETION = 0x8400,
  TRB_TYPE_PORT_STATUS_CHANGE = 0x8800,
  TRB_TYPE_BANDWIDTH_REQUEST = 0x8c00,
  TRB_TYPE_HOST_CONTROLLER = 0x9400,
  TRB_TYPE_DEVICE_NOTIFICATION = 0x9800,
  TRB_TYPE_MFINDEX_WRAP = 0x9c00,
};

static xhci_command EnableSlot() {
  return { 0, 0, TRB_TYPE_ENABLE_SLOT };
}

static xhci_command DisableSlot(uint8_t slotId) {
  return { 0, 0, TRB_TYPE_DISABLE_SLOT | (slotId << 24) };
}

static xhci_command AddressDevice(uint64_t inputContextPointer, uint8_t slotId) {
  return { inputContextPointer, 0, ((uint32_t)slotId << 24) | TRB_TYPE_ADDRESS_DEVICE };
}

static xhci_command ConfigureEndpoint(uint64_t inputContextPointer, uint8_t slotId) {
  return { inputContextPointer, 0, ((uint32_t)slotId << 24) | TRB_TYPE_CONFIGURE_ENDPOINT };
}

static xhci_command NormalTRB(uint64_t dataBuffer, uint32_t trbBytes, uint8_t td_size) {
  return { dataBuffer, (td_size << 17) | (trbBytes), TRB_TYPE_NORMAL };
}

static xhci_command SetupStage(uint8_t requestType, uint8_t request, uint16_t value, uint16_t length, uint16_t index, uint8_t transferType) {
  return {
          ((uint64_t)length << 48) | 
          ((uint64_t)index << 32) | 
          ((uint64_t)value << 16) |
          ((uint64_t)request << 8) |
          requestType, 
          8, 
          ((uint32_t)transferType << 16) | TRB_TYPE_SETUP_STAGE | TRB_FLAG_IDT };
}

static xhci_command DataStage(uint64_t dataBuffer, uint32_t trbBytes, uint8_t td_size, bool writeData) {
  return { dataBuffer, (td_size << 17) | (trbBytes), TRB_TYPE_DATA_STAGE | TRB_FLAG_DIR | TRB_FLAG_ISP };
}

static xhci_command StatusStage() {
  return { 0, 0, TRB_TYPE_STATUS_STAGE | TRB_FLAG_IOC };
}

}

XhciDevice::XhciDevice(pcidevice dev)
: dev(dev)
, bar1(dev, PciBars::Bar0)
{
  currentFlag = TRB_FLAG_C;
  currentEventFlag = TRB_FLAG_C;
  // Look up the registers for this controller
  cr = (uintptr_t)bar1.get();
  opregs = cr + (mmio_read<uint32_t>(cr + CR_CAPLENGTH) & 0xFF);
  rr = cr + mmio_read<uint32_t>(cr + CR_RTSOFF);
  doorbell = cr + mmio_read<uint32_t>(cr + CR_DBOFF);

  uint16_t hciversion = mmio_read<uint32_t>(cr + CR_CAPLENGTH) >> 16;
  uint32_t hcsparams1 = mmio_read<uint32_t>(cr + CR_HCSPARAMS1);
  uint32_t maxports = (hcsparams1 >> 24) & 0xFF;
  uint32_t maxinterrupters = (hcsparams1 >> 8) & 0x7FF;
  uint32_t maxslots = (hcsparams1 >> 0) & 0xFF;
  debug("[XHCI] Initializing device version {x} (maxports {} maxslots {} maxinterrupters {})\n", hciversion, maxports, maxslots, maxinterrupters);

// Reset device, wait for it to come back up
  pciwrite16(dev, 0x04, pciread16(dev, 0x04) | 6);

  mmio_write<uint32_t>(opregs + RT_USBCMD, mmio_read<uint32_t>(opregs + RT_USBCMD) & ~RT_USBCMD_RUN);
  while ((mmio_read<uint32_t>(opregs + RT_USBSTS) & RT_USBSTS_HCH) == 0) {}

  mmio_write<uint32_t>(opregs + RT_USBCMD, mmio_read<uint32_t>(opregs + RT_USBCMD) | RT_USBCMD_RESET);
  // TODO: co_await delay(1us);
  while ((mmio_read<uint32_t>(opregs + RT_USBCMD) & RT_USBCMD_RESET) != 0) {}
  mmio_write<uint32_t>(opregs + RT_DNCTRL, 0x02);

  uint32_t hcsparams2 = mmio_read<uint32_t>(cr + CR_HCSPARAMS2);

  // Create scratchpad pages
  uint32_t scratchcount = ((hcsparams2 >> 27) & 0x1F) | ((hcsparams2 >> 16) & 0x3E0);
  if (scratchcount > 512)
    debug("[XHCI] requires more scratchpads than fit the index?\n");
  uint64_t scratchpad_index = freepage_get();
  mapping index(scratchpad_index, 0x1000, DeviceMemory);
  memset(index.get(), 0, 0x1000);
  uint64_t* scratchpads = (uint64_t*)index.get();
  for (size_t n = 0; n < scratchcount; n++) {
    scratchpads[n] = freepage_get_zeroed();
  }

  // Create DCBAA page & slot backing stores, pass to device
  uint64_t dcbPage = freepage_get_zeroed();
  dcbaa = mapping(dcbPage, 0x1000, DeviceMemory);
  mmio_write<uint64_t>((uintptr_t)dcbaa.get(), scratchpad_index);
  mmio_write<uint64_t>(opregs + RT_DCBAAP, dcbPage);

  // Map command ring
  commandRingPhysical = freepage_get_zeroed();
  commandRing = mapping(commandRingPhysical, 0x1000, DeviceMemory);
  mmio_write<uint64_t>(opregs + RT_CRCR, commandRingPhysical + 1);

  // Map event ring (1 page only, but requires indirection) and enable interrupts for when it gets events
  uint64_t erst = freepage_get_zeroed();
  mapping evtSegm(erst, 0x1000, DeviceMemory);
  uint64_t eventRingPhysical = freepage_get_zeroed();
  eventRing = mapping(eventRingPhysical, 0x1000, DeviceMemory);
  mmio_write<uint64_t>((uintptr_t)evtSegm.get(), eventRingPhysical);
  mmio_write<uint64_t>((uintptr_t)evtSegm.get() + 8, 0x100);
  mmio_write<uint32_t>(rr + RT_ERSTSZ, 1);
  mmio_write<uint64_t>(rr + RT_ERDP, eventRingPhysical);
  mmio_write<uint64_t>(rr + RT_ERSTBA, erst);
  mmio_write<uint32_t>(rr + RT_IMAN, 2);
  mmio_write<uint32_t>(rr + RT_IMOD, 0);
  
  // Enable all device slots
  mmio_write<uint32_t>(opregs + RT_CONFIG, mmio_read<uint32_t>(opregs + RT_CONFIG) | maxslots);

  mmio_write<uint32_t>(opregs + RT_USBCMD, mmio_read<uint32_t>(opregs + RT_USBCMD) | RT_USBCMD_RUN | RT_USBCMD_INTE);
  // Reset all pending interrupts
  mmio_write<uint32_t>(opregs + RT_USBSTS, mmio_read<uint32_t>(opregs + RT_USBSTS));

  //Work on ports
  debug("[XHCI] controller enabled\n", maxports);
  for (size_t n = 0; n < maxports; ++n)
  {
    uint64_t op_port = opregs + 0x400 + n*16;
    uint32_t sc = mmio_read<uint32_t>(op_port + P_SC);
    if ((sc & 1) == 1) {
      // We know that there is *something* here. 
      // Trigger a reset, so that we will get a PORT_STATUS_CHANGE for this port
      mmio_write<uint32_t>(op_port + P_SC, 0x10);
    }
  }

  for (size_t n = 0; n < 3; n++) {
    HandleInterrupt();
  }
}

s2::future<uint64_t> XhciDevice::RegisterStatus(uintptr_t address) {
  s2::promise<uint64_t> rv;
  s2::future<uint64_t> rv_v = rv.get_future();
  callbacks.push_back({ address, s2::move(rv)});
  return rv_v;
}

s2::future<uint64_t> XhciDevice::RunCommand(xhci_command cmd) {
  xhci_command* commandRingPtr = (xhci_command*)commandRing.get();

  // register handler for return
  s2::future<uint64_t> rv = RegisterStatus(commandRingPhysical + currentCommand * 16);

  commandRingPtr[currentCommand].pointer = cmd.pointer;
  commandRingPtr[currentCommand].status = cmd.status;
  commandRingPtr[currentCommand].control = cmd.control ^ currentFlag;
  currentCommand++;
  if (currentCommand == commandRingSize + 1) {
    // TODO: write wrap command
    currentCommand = 0;
    currentFlag ^= TRB_FLAG_C;
  }

  mmio_write<uint32_t>(doorbell, 0);

  return rv;
}

void XhciDevice::HandleInterrupt() {
  debug("[XHCI] Interrupt\n");
  xhci_command* cmd = (xhci_command*)eventRing.get();
  while (true) {
    xhci_command& current = cmd[eventRingIndex];
    if ((current.control & TRB_FLAG_C) != currentEventFlag) {
      break;
    }
    uint8_t trbType = (current.control >> 10) & 0x3F;
    switch(trbType) {
      case 35: // bandwidth request
      case 36: // doorbell
      case 37: // host controller
      case 38: // device notification
      case 39: // mfindex wrap
      default:
        debug("[XHCI] Found valid unrecognized event {x} {x} {x}\n", current.pointer, current.status, current.control);
        break;
      case 32: // transfer
      case 33: // command completion
//        debug("[XHCI] Command completion event\n");
        for (size_t n = 0; n < callbacks.size(); n++) {
          if (current.pointer == callbacks[n].addr) {
//            debug("[XHCI] Found handler, informing\n");
            callbacks[n].p.set_value(((uint64_t)current.status << 32) | current.control);
            callbacks[n] = s2::move(callbacks.back());
            callbacks.pop_back();
            break;
          }
        }
        break;
      case 34: // port status change
//        debug("[XHCI] Port status change event\n");
        {
          uint8_t port = ((current.pointer >> 24) & 0xFF) - 1;
          uint64_t op_port = opregs + 0x400 + port*16;
          uint32_t sc = mmio_read<uint32_t>(op_port + P_SC);
          uint8_t port_speed = (sc >> 10) & 0xF;
          static const char* speeds[16] = { "Unk0", "Full", "Low", "High", "Super", "Super2x1", "Super1x2", "Super2x2","Unk","Unk","Unk","Unk","Unk","Unk","Unk","Unk"};
//          debug("[XHCI] Found device at {s} on port {}\n", speeds[port_speed], port);
          if (port_speed != 0) {
            (new XhciUsbDevice(this, port))->start();
          }
        }
        break;
    }
    eventRingIndex++;
  }
}

XhciUsbDevice::XhciUsbDevice(XhciDevice* host, uint8_t port_entry)
: host(host)
, portid(port_entry)
, portmap(freepage_get_zeroed(), 0x1000, DeviceMemory)
, port((InputContext*)portmap.get())
, active(false)
{
  memset(port, 0, sizeof(*port));

  port->d = 0;
  port->a = 3;
  uint8_t rootHubPortNo = (port_entry & 0x3) + 1; // ??

  port->port.route = 0x08000000;
  port->port.ports = 0x00000000 | (rootHubPortNo << 16);
  port->port.parent = 0;
  port->port.device = 0;

  port->port.control.a = 0;
  port->port.control.b = 0x00080020;
  port->port.control.tr_deq_ptr = portmap.to_physical((void*)&port->loop) | 1;
  port->port.control.average = 8;
}

uintptr_t XhciUsbDevice::EnqueueCommand(xhci_command cmd) {
  uint8_t index = (port->loopIndex++) % 128;
  if ((port->loopIndex & 0x80) == 0) cmd.control ^= TRB_FLAG_C;
  port->loop[index] = cmd;
  return portmap.to_physical((void*)&port->loop[index]);
}

void XhciUsbDevice::RingDoorbell(uint8_t endpoint, bool in) {
  asm volatile ("" : : : "memory");
  mmio_write<uint32_t>(host->doorbell + slotId * 4, endpoint * 2 + in);
}

s2::future<s2::span<const uint8_t>> XhciUsbDevice::RunCommandRequest(uint8_t requestType, uint8_t request, uint16_t value, uint16_t index, uint16_t length) {
  assert(length <= sizeof(port->buffer));
  if (length > sizeof(port->buffer)) co_return {};
  EnqueueCommand(SetupStage(requestType, request, value, length, index, 3));
  EnqueueCommand(DataStage(portmap.to_physical(port->buffer), length, 0, true));
  s2::future<uint64_t> statusResult = host->RegisterStatus(EnqueueCommand(StatusStage()));
  RingDoorbell();

  if ((((co_await statusResult) >> 56) & 0xFF) != XHCI_COMPLETION_SUCCESS) co_return {};
  co_return s2::span<const uint8_t>(port->buffer, length);
}

s2::future<bool> XhciUsbDevice::GetDescriptor(DescriptorType type, uint8_t descriptorId, s2::span<uint8_t> descriptor) {
  auto data = co_await RunCommandRequest(0x80, 6, ((uint16_t)type << 8) | descriptorId, 0, descriptor.size());
  memcpy(descriptor.data(), data.data(), data.size());
  co_return not data.empty();
}

s2::future<s2::string> XhciUsbDevice::GetStringDescriptor(uint8_t descriptorId, uint16_t languageId) {
  if (descriptorId == 0) {
    co_return "";
  }
  auto size = co_await RunCommandRequest(0x80, 6, 0x300 | descriptorId, languageId, 1);
  if (size.empty()) {
    // Log error
    co_return "";
  }
  uint8_t strLength = size[0];
  auto str = co_await RunCommandRequest(0x80, 6, 0x300 | descriptorId, languageId, strLength);
  if (str.empty()) {
    // Log error
    co_return "";
  }
  co_return s2::string::from_utf16le(str.subspan(2));
}

s2::future<bool> XhciUsbDevice::StartUp() {
  uint64_t op_port = host->opregs + 0x400 + portid*16;
  uint32_t sc = mmio_read<uint32_t>(op_port + P_SC);
  uint8_t port_speed = (sc >> 10) & 0xF;

  uint64_t addr_evt = co_await host->RunCommand(AddressDevice(portmap.to_physical(&port->d), slotId));
  if ((addr_evt >> 56) != XHCI_COMPLETION_SUCCESS) {
    debug("[XHCI] Could not address device\n");
    co_return false;
  }

  if (!co_await GetDescriptor(DescriptorType::Device, 0, s2::span<uint8_t>((uint8_t*)&port->device_descriptor, 18))) {
    debug("[XHCI] Could not retrieve device descriptor\n");
    co_return false;
  }
  debug("[XHCI] Found USB device {x}:{x} class {x}:{x}:{x}\n", port->device_descriptor.vendorId, port->device_descriptor.deviceId,port->device_descriptor.deviceClass,port->device_descriptor.subClass,port->device_descriptor.protocol);

  uint8_t length;
  if (!co_await GetDescriptor(DescriptorType::String, 0, s2::span<uint8_t>(&length, 1))) {
    debug("[XHCI] Could not retrieve string language descriptor (1)\n");
    co_return false;
  }
  StringLanguages* sRoot = (StringLanguages*)new uint8_t[length];
  if (!co_await GetDescriptor(DescriptorType::String, 0, s2::span<uint8_t>((uint8_t*)sRoot, length))) {
    debug("[XHCI] Could not retrieve string language descriptor (2)\n");
    co_return false;
  }

  // Select the best portable language; US English if it exists, another English if US 
  // English did not exist, or first language if no English exists.
  uint16_t bestLanguageId = sRoot->languages[0];
  for (size_t index = 0; index < (sRoot->length - 2) / 2; index++) {
    if (sRoot->languages[index] == 0x409) {
      bestLanguageId = 0x409;
      break;
    } else if ((sRoot->languages[index] & 0x3FF) == 0x9 &&
               (sRoot->languages[index] < bestLanguageId)) {
      bestLanguageId = sRoot->languages[index];
    }
  }
  debug("[XHCI] Using language ID {x}\n", bestLanguageId);

  manufacturer = co_await GetStringDescriptor(port->device_descriptor.manufacturer, bestLanguageId);
  product = co_await GetStringDescriptor(port->device_descriptor.product, bestLanguageId);
  serial = co_await GetStringDescriptor(port->device_descriptor.serialnum, bestLanguageId);
  debug("[XHCI] Found {s} {s} {s}\n", manufacturer, product, serial);

  s2::vector<ConfigurationDescriptor*> confDescriptors;
  for (size_t n = 0; n < port->device_descriptor.configurations; n++) {
    ConfigurationDescriptor desc;
    if (not co_await GetDescriptor(DescriptorType::Configuration, n, s2::span<uint8_t>((uint8_t*)&desc, sizeof(desc)))) {
      debug("[XHCI] Could not retrieve configuration descriptor {} (1)\n", n);
      co_return false;
    }
    uint8_t* copy = new uint8_t[desc.totalLength];
    if (not co_await GetDescriptor(DescriptorType::Configuration, n, s2::span<uint8_t>(copy, desc.totalLength))) {
      debug("[XHCI] Could not retrieve configuration descriptor {} (2)\n", n);
      co_return false;
    }
    cd.push_back((ConfigurationDescriptor*)copy);
  }
  co_return true;
}

s2::future<void> XhciUsbDevice::start() {
  uint64_t curevt = co_await host->RunCommand(EnableSlot());
  slotId = (curevt >> 24) & 0xFF;
  if ((curevt >> 56) != XHCI_COMPLETION_SUCCESS || slotId == 0) {
    delete this;
    co_return;
  }

  mmio_write<uint64_t>((uintptr_t)host->dcbaa.get() + 8 + 8*slotId, (uintptr_t)portmap.get());
  host->devices[slotId] = this;

  if (co_await StartUp()) {
    UsbCore::Instance().RegisterUsbDevice(*this);
  } else {
    // Log error
    mmio_write<uint64_t>((uintptr_t)host->dcbaa.get() + 8 + 8*slotId, 0);
    host->devices[slotId] = nullptr;
    host->RunCommand(DisableSlot(slotId));
    delete this;
    co_return;
  }
}

DeviceDescriptor& XhciUsbDevice::GetDeviceDescriptor() {
  return dd;  
}

const s2::vector<ConfigurationDescriptor*>& XhciUsbDevice::GetConfigurationDescriptors() {
  return cd;
}

struct XhciEndpoint final : public UsbEndpoint {
  XhciEndpoint(XhciUsbDevice& dev, uint8_t endpointId, bool isIn) 
  : dev(dev)
  , address(freepage_get_zeroed())
  , map(address, 0x1000, DeviceMemory)
  , loop((xhci_command*)map.get())
  , endpointId(endpointId)
  , isIn(isIn)
  {
  }
  s2::future<void> ReadData(uint64_t physAddr, size_t length) override;
  XhciUsbDevice& dev;
  uint64_t address;
  mapping map;
  xhci_command* loop;
  uint8_t loopIndex = 0;
  uint8_t endpointId;
  bool isIn;
};

s2::future<void> XhciEndpoint::ReadData(uint64_t physAddr, size_t length) {
  auto& cmd = loop[loopIndex++];
  cmd = NormalTRB(physAddr, length, 0);
  cmd.control |= TRB_FLAG_IOC | TRB_FLAG_C;

  s2::future<uint64_t> statusF = dev.host->RegisterStatus(map.to_physical(&cmd));
  dev.RingDoorbell(endpointId, isIn);
  uint64_t status = co_await statusF;
  debug("B\n");
  if ((status >> 56) != XHCI_COMPLETION_SUCCESS) {
    debug("[XHCI] Read data request failed\n");
  }
} 

s2::future<UsbEndpoint*> XhciUsbDevice::StartupEndpoint(EndpointDescriptor& desc) {
  uint8_t epId = (desc.address & 0xF);
  bool isIn = (desc.address & 0x80);
  XhciEndpoint* ep = new XhciEndpoint(*this, epId, isIn);
  EndpointContext& epc = (isIn ? port->port.eps[epId - 1].in : port->port.eps[epId - 1].out);
  uint8_t epType = desc.attributes & 0x03; // control / iso / bulk / interrupt
  switch(epType) {
    case 0x01: // iso
    case 0x03: // interrupt
      epc.a = (desc.interval << 16) * 8;
      epc.b = (desc.maxPacketSize << 16) | (epType << 4) | (isIn << 3) | 6;
      epc.tr_deq_ptr = ep->address | 1;
      epc.average = desc.maxPacketSize;// | (desc.maxPacketSize << 16);
      break;
    case 0x02:
      // TODO: Bulk
      break;
  }
/*
EP Type = Isoch IN, Isoch OUT, Interrupt IN or Interrupt OUT. Refer to Table 6-9 for
the encoding.
• Max Packet Size = Endpoint Descriptor:wMaxPacketSize & 07FFh.
• Max Burst Size = SuperSpeed Endpoint Companion Descriptor:bMaxBurst or
(Endpoint Descriptor: wMaxPacketSize & 1800h) >> 11.
• Mult = SuperSpeed Endpoint Companion Descriptor:bmAttributes:Mult field.
Always ‘0’ for Interrupt endpoints.
• CErr = 3 for Interrupt endpoints. Enables 3 retries.
CErr = 0 for Isoch endpoints. Retries are not performed for Isoch endpoints.
• TR Dequeue Pointer = Start address of the first segment of the previously allocated
Transfer Ring.
• Dequeue Cycle State (DCS) = 1. Assuming that all TRBs in the segment referenced
by the TR Dequeue Pointer have been initialized to ‘0’, this field reflects Cycle bit
state for valid TRBs written by software.
• Max ESIT Payload = Refer to section 4.14.2 for value.
*/

  debug("{08x} {08x} {016x} {08x}\n", epc.a, epc.b, epc.tr_deq_ptr, epc.average);
  port->a = (1 << (epId * 2 + isIn)) | 1;
  port->port.route = (epId + 1) << 27;

  uint64_t addr_evt = co_await host->RunCommand(ConfigureEndpoint(portmap.to_physical(&port->d), slotId));
  if ((addr_evt >> 56) != XHCI_COMPLETION_SUCCESS) {
    debug("[XHCI] Could not address device\n");
    co_return nullptr;
  }

  co_return ep;
}

s2::future<void> XhciUsbDevice::SetConfiguration(uint8_t configuration) {
  if (configuration >= cd.size()) co_return;
  if (not activeInterfaces.empty()) {
    for (auto i : activeInterfaces) {
      delete i;
    }
    activeInterfaces.clear();
  }
  
  co_await RunCommandRequest(0, 9, cd[configuration]->configVal, 0, 0);

  uint8_t* configDesc = (uint8_t*)(cd[configuration]);
  size_t size = cd[configuration]->totalLength;

  size_t offset = cd[configuration]->length;
  s2::vector<const UsbDescriptor*> descriptors;
  for (size_t n = 0; offset < size; n++) {
    uint8_t* start = configDesc + offset;
    UsbDescriptor* desc = (UsbDescriptor*)(configDesc + offset);
    do {
      if (desc->length < 2) {
        debug("[XHCI] Found invalid descriptor\n");
        co_return;
      }
      descriptors.push_back(desc);
      offset += desc->length;
      desc = (UsbDescriptor*)(configDesc + offset);
    } while (offset < size && desc->type != 2);

    UsbInterface* i = new UsbInterface(*this, n, s2::move(descriptors));
    descriptors.clear();
    // create interface
    activeInterfaces.push_back(i);
    UsbCore::Instance().RegisterUsbInterface(*i);
  }
}








