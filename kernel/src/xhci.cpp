#include "xhci.h"
#include "pci.h"
#include "debug.h"
#include "io.h"
#include "map.h"
#include <cstring>
#include "freepage.h"

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

constexpr uint64_t P_SC = 0x00;
constexpr uint64_t P_PMSC = 0x04;
constexpr uint64_t P_LI = 0x08;

constexpr uint64_t RT_USBSTS_HCH = 0x1;
constexpr uint64_t RT_USBCMD_RUN = 0x00000001;
constexpr uint64_t RT_USBCMD_RESET = 0x00000002;

/*
void disable_efi() {
  uint32_t legacy = findExtendedCap(XHCI_ECAP_LEGSUP);
  if (legacy != 0)
  {
    uint32_t legreg = readCapabilityRegister(legacy, 32);
    legreg |= (1 << 24);
    writeCapabilityRegister(legacy, legreg, 32);
    while (1)
    {
      legreg = readCapabilityRegister(legacy, 32);
      if (((legreg & (1 << 24)) != 0) && ((legreg & (1 << 16)) == 0))
        break;
    }
    kprintf(u"Taken control of XHCI from firmware\n");
  }
  //Disable SMIs
  uint32_t legctlsts = readCapabilityRegister(legacy + 4, 32);
  legctlsts &= XHCI_LEGCTLSTS_DISABLE_SMI;
  legctlsts |= XHCI_LEGCTLSTS_EVENTS_SMI;
  writeCapabilityRegister(legacy + 4, legctlsts, 32);
}
*/

struct xhci_speed {
  void debugprint() {
    if (spd_mantissa == 0) {
      debug("<reserved>");
    } else {
      debug("{} {} ", spd_mantissa, (spd_exponent == '0' ? "bps" : (spd_exponent == '1' ? "Kbps" : (spd_exponent == '2' ? "Mbps" : "Gbps"))));
      if (fullduplex) debug("full duplex ");
    }
  }
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

void extractPsids(pcidevice dev) {

}

uint64_t XhciDevice::CreateContext(uint32_t scratchcount) {
  uint64_t page = freepage_get();
  mapping dcb(page, 0x1000, DeviceMemory);
  memset(dcb.get(), 0, 0x1000);

  uint64_t scratchpad_index = freepage_get();
  mmio_write<uint64_t>((uintptr_t)dcb.get(), scratchpad_index);

  mapping index(scratchpad_index, 0x1000, DeviceMemory);
  memset(index.get(), 0, 0x1000);
  uint64_t* scratchpads = (uint64_t*)index.get();
  for (size_t n = 0; n < scratchcount; n++) {
    scratchpads[n] = freepage_get_zeroed();
  }
  return page;
}

struct xhci_command {
	uint64_t a, b;
};

static xhci_command create_enableslot_command(uint8_t slottype) {
//  return { 0, XHCI_TRB_ENABLED | XHCI_TRB_TYPE(XHCI_TRB_TYPE_ENABLE_SLOT) | (slottype << 16) };
	return {};
}

XhciDevice::XhciDevice(pcidevice dev)
: dev(dev)
, bar1(dev, PciBars::Bar0)
{
  cr = (uintptr_t)bar1.get();
  opregs = cr + (mmio_read<uint32_t>(cr + CR_CAPLENGTH) & 0xFF);
  rr = cr + mmio_read<uint32_t>(cr + CR_RTSOFF);
  doorbell = cr + mmio_read<uint32_t>(cr + CR_DBOFF);

  uint32_t hcsparams1 = mmio_read<uint32_t>(cr + CR_HCSPARAMS1);
  uint32_t maxports = (hcsparams1 >> 24) & 0xFF;
  uint32_t maxinterrupters = (hcsparams1 >> 8) & 0x7FF;
  uint32_t maxslots = (hcsparams1 >> 0) & 0xFF;

  pciwrite16(dev, 0x04, pciread16(dev, 0x04) | 6);

  mmio_write<uint32_t>(opregs + RT_USBCMD, mmio_read<uint32_t>(opregs + RT_USBCMD) & ~RT_USBCMD_RUN);
  while ((mmio_read<uint32_t>(opregs + RT_USBSTS) & RT_USBSTS_HCH) == 0) {}

  mmio_write<uint32_t>(opregs + RT_USBCMD, mmio_read<uint32_t>(opregs + RT_USBCMD) | RT_USBCMD_RESET);
  // TODO: sleep(1us)
  while ((mmio_read<uint32_t>(opregs + RT_USBCMD) & RT_USBCMD_RESET) != 0) {}

  mmio_write<uint32_t>(opregs + RT_DNCTRL, 0x02);

  uint32_t hcsparams2 = mmio_read<uint32_t>(cr + CR_HCSPARAMS2);
  uint32_t scratchcount = ((hcsparams2 >> 27) & 0x1F) | ((hcsparams2 >> 16) & 0x3E0);
  if (scratchcount > 512)
    debug("XHCI: requires more scratchpads than fit the index?\n");
  uint64_t dcbPage = CreateContext(scratchcount);
  mmio_write<uint64_t>(opregs + RT_DCBAAP, dcbPage);
  mmio_write<uint32_t>(opregs + RT_CONFIG, mmio_read<uint32_t>(opregs + RT_CONFIG) | maxslots);

  uint64_t cr_page = freepage_get_zeroed();
  mapping cr(cr_page, 0x1000, DeviceMemory); // or registers?
  
  uint64_t primaryEventRing = freepage_get_zeroed();
  mapping pev(primaryEventRing, 0x1000, DeviceMemory);

  mmio_write<uint64_t>(opregs + RT_CRCR, cr_page + 1);

  uint64_t evtSeg = freepage_get_zeroed();
  mapping evtSegm(evtSeg, 0x1000, DeviceMemory);
  uint64_t evtSegList = freepage_get_zeroed();
  mapping evtSegListm(evtSegList, 0x1000, DeviceMemory);

  mmio_write<uint64_t>((uintptr_t)evtSegListm.get(), evtSeg);
  mmio_write<uint64_t>((uintptr_t)evtSegListm.get() + 8, 0x100);
 
  // this is the place to write stuff to the event ring / read from it
  uint64_t p = evtSeg;
  mmio_write<uint32_t>(rr + RT_ERSTSZ, 1);
  mmio_write<uint64_t>(rr + RT_ERDP, evtSegList | 8 | (mmio_read<uint64_t>(rr + RT_ERDP) & 0x7) );
  mmio_write<uint64_t>(rr + RT_ERSTBA, evtSegList);
  mmio_write<uint32_t>(rr + RT_IMAN, mmio_read<uint32_t>(rr + RT_IMAN));

  mmio_write<uint32_t>(opregs + RT_USBCMD, mmio_read<uint32_t>(opregs + RT_USBCMD) | RT_USBCMD_RUN);

  /*
  Runtime.Interrupter(0).IMAN.InterruptEnable = 1;
  Operational.USBCMD.INTE = 1;
  //Just make sure we don't miss interrupts
  Runtime.Interrupter(0).IMAN = Runtime.Interrupter(0).IMAN;
  Runtime.Interrupter(0).IMOD.InterruptInterval = 4000;
  Operational.USBSTS = Operational.USBSTS;

*/
  //Work on ports
  debug("[XHCI] controller enabled, {} ports\n", maxports);
  for (size_t n = 0; n < maxports; ++n)
  {
    uint64_t op_port = opregs + 0x400 + n*16;
    uint32_t sc = mmio_read<uint32_t>(op_port + P_SC);
    if (sc & 1) {
      debug("[XHCI] Found device on port {}\n", n);
      devices[n] = new XhciUsbDevice(this, n);
    }
  }
}

XhciUsbDevice::XhciUsbDevice(XhciDevice* host, uint8_t id)
       : host(host)
 , slotId(id)
{
  uint64_t op_port = host->opregs + 0x400 + slotId*16;
  uint32_t pmsc = mmio_read<uint32_t>(op_port + P_PMSC);
  uint32_t li = mmio_read<uint32_t>(op_port + P_LI);
  mmio_write<uint32_t>(op_port + P_SC, 0x10);
  while (mmio_read<uint32_t>(op_port + P_SC) & 0x10) {}
/*
  void* last_command = nullptr;
  last_command = cmdring.enqueue(create_enableslot_command(protocol_speeds[portspeed].slottype));
  //Now get a command completion event from the event ring
  uint64_t* curevt = (uint64_t*)waitComplete(last_command, 1000);
  uint8_t slotid = curevt[1] >> 56;
  if (get_trb_completion_code(curevt) != XHCI_COMPLETION_SUCCESS || slotid == 0) {
    debug("Could not get a slot ID from xhci controller, turning off port");
    state = Off;
    return;
  }

  //kprintf(u"  DSE command complete: %d\n", slotid);
  xhci_port_info* port_info = new xhci_port_info(this, slotid);
  port_info->controller = cinfo;
  port_info->portindex = n;
  port_info->slotid = slotid;
  if (!createSlotContext(portspeed, port_info))
    return;

  // set address first, then request descriptors
  enqueue_command(create_setup_stage_trb(0x80, 6, 0x100, 0, 20, 3));
  enqueue_command(create_data_stage_trb(get_physical_address((void*)descriptors), 20, true));
  enqueue_command(create_status_stage_trb(true));
  send_commands();
  */
/*
  void* statusevt = port_info->cmdring.enqueue_commands(devcmds);
  void* resulttrb = waitComplete(statusevt, 1000);
*/ // how?
/*  if (get_trb_completion_code(resulttrb) != XHCI_COMPLETION_SUCCESS)
  {
    kprintf(u"Error getting device descriptor (code %d)\n", get_trb_completion_code(resulttrb));
    return;
  }
  kprintf_a("   Device %x:%x class %x:%x:%x USB version %x\n", devdesc->idVendor, devdesc->idProduct, devdesc->bDeviceClass, devdesc->bDeviceSublass, devdesc->bDeviceProtocol, devdesc->bcdUSB);
*/
/*
  if (devdesc->iManufacturer != 0)
  {
    //Get device string
    devcmds[0] = create_setup_stage_trb(0x80, 6, 0x300 + devdesc->iManufacturer, 0, 256, 3);
    volatile wchar_t* devstr = new wchar_t[256];
    devcmds[1] = create_data_stage_trb(get_physical_address((void*)devstr), 256, true);
    devcmds[2] = create_status_stage_trb(true);
    devcmds[3] = nullptr;
    statusevt = port_info->cmdring.enqueue_commands(devcmds);
    resulttrb = waitComplete(statusevt, 1000);
    kprintf(u"   Device vendor %s\n", ++devstr);
  }
  if (devdesc->iProduct != 0)
  {
    //Get device string
    devcmds[0] = create_setup_stage_trb(0x80, 6, 0x300 + devdesc->iProduct, 0, 256, 3);
    volatile wchar_t* devstr = new wchar_t[256];
    devcmds[1] = create_data_stage_trb(get_physical_address((void*)devstr), 256, true);
    devcmds[2] = create_status_stage_trb(true);
    devcmds[3] = nullptr;
    statusevt = port_info->cmdring.enqueue_commands(devcmds);
    resulttrb = waitComplete(statusevt, 1000);
    kprintf(u"   %s\n", ++devstr);
  }
  */
}

