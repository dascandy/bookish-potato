#include "pci/PciCore.h"
#include "debug.h"
#include "io.h"
#include <string>

PciBridge::PciBridge(uintptr_t dev, uint8_t bus)
: PciDevice(dev)
{
  if (dev != 0) {
    bus = (conf->bar[2] >> 8) & 0xFF;
  }
  uintptr_t busBase = PciCore::Instance().baseaddress[bus];
  for (size_t slot = 0; slot < 32; slot++) {
    size_t num_functions = 1;
    for (size_t function = 0; function < num_functions; function++) {
      uintptr_t cfgAddress = busBase + function * 4096 + slot * 32768;
      mapping cfg(cfgAddress, 0x1000, DeviceRegisters);
      volatile PciCfgSpace* conf = (volatile PciCfgSpace*)cfg.get();

      if (conf->vendor_device == 0 || conf->vendor_device == 0xFFFFFFFF)
        continue;
      if ((conf->cache & 0x800000) != 0)
        num_functions = 8;

      uint16_t device = conf->vendor_device >> 16;
      uint16_t vendor = conf->vendor_device & 0xFFFF;
      uint32_t devClass = (conf->classcode & 0xFFFFFF00) >> 8;
      PciDevice* dev = PciCore::Instance().RegisterPciDevice(cfgAddress, (vendor << 16) | device, devClass);
      if (dev) 
        RegisterDevice(dev);
    }
  }
}

void PciBridge::RegisterDevice(PciDevice* devobj) {
  devices.push_back(devobj);
}

PciCore& PciCore::Instance() {
  static PciCore core;
  return core;
}

void PciCore::RegisterVidPidDriver(uint16_t vid, uint16_t pid, s2::function<PciDevice*(uintptr_t cfgSpace)> driver) {
  pidVidDrivers.emplace(((uint32_t)vid << 16) | pid, driver);
}

void PciCore::RegisterClassDriver(uint32_t classId, s2::function<PciDevice*(uintptr_t cfgSpace)> driver) {
  classDrivers.emplace(classId, driver);
}

PciDevice* PciCore::RegisterPciDevice(uintptr_t configSpace, uint32_t vendorDevice, uint32_t deviceClass) {
  if (auto it = pidVidDrivers.find(vendorDevice); it != pidVidDrivers.end()) {
    return it->second(configSpace);
  } else if (auto it1 = classDrivers.find(deviceClass); it1 != classDrivers.end()) {
    return it1->second(configSpace);
  } else if (auto it2 = classDrivers.find(deviceClass & 0xFFFF00); it2 != classDrivers.end()) {
    return it2->second(configSpace);
  } else if (auto it3 = classDrivers.find(deviceClass & 0xFF0000); it3 != classDrivers.end()) {
    return it3->second(configSpace);
  } else {
    debug("[PCI] Unimplemented device {x}/{x} {x}:{x}:{x} found\n", 
        vendorDevice >> 16, vendorDevice & 0xFFFF,
        (deviceClass >> 16) & 0xFF, (deviceClass >> 8) & 0xFF, (deviceClass) & 0xFF);

    // TODO: create placeholder device for device tree exposure
    return nullptr;
  }
}

void PciCore::RegisterBusAddress(uintptr_t base, uint8_t busId, uint8_t busCount) {
  for (uint8_t id = busId; id < busId + busCount; id++) {
    baseaddress[id] = base;
    base += 1048576;
  }
}

const char* pci_caps[] = {
  "-",
  "pm",
  "agp",
  "vpd",
  "slot",
  "msi",
  "cpci_hs",
  "pci-x",
  "ht",
  "vendor",
  "debug",
  "ccrc",
  "hotplug",
  "subsys_vid",
  "agp-br",
  "inv-0xF",
  "pci-ex",
  "msi-x",
  "inv-0x12",
  "pciadv",
};

void pci_debug_print_device(uint16_t vendor, uint16_t device, uint32_t devClass) {
  debug("[PCI] found PCI vid={4x}", vendor);
  if (vendor == 0x8086) debug("(intel)");
  else if (vendor == 0x10EC) debug("(realtek)");
  else if (vendor == 0x1234) debug("(qemu)");
  else if (vendor == 0x1B36) debug("(qemu)");
  debug(" did={4x} class={6x} (", device, devClass);

  if (devClass == 0x010180)
    debug("IDE controller device");
  else if (devClass == 0x010601)
    debug("AHCI device");
  else if (devClass == 0x010801)
    debug("NVMHCI device");
  else if (devClass == 0x010802)
    debug("NVMExpress device");
  else if (devClass == 0x020000)
    debug("Ethernet controller device");
  else if (devClass == 0x030000)
    debug("VGA Graphics device");
  else if (devClass == 0x038000)
    debug("non-VGA graphics device");
  else if (devClass == 0x040300)
    debug("audio device");
  else if (devClass == 0x0c0330)
    debug("XHCI device");
  else if (devClass == 0x060000)
    debug("host bridge device");
  else if (devClass == 0x060100)
    debug("ISA bridge device");
  else if (devClass == 0x060400)
    debug("PCI bridge device");
  else if (devClass == 0x068000)
    debug("some weird bridge device");
  else if (devClass == 0x080501)
    debug("SDHCI device");
  else 
    debug("unknown");
  debug(")");
}

enum class PciCapability {
  Null,
  PowerManagement,
  AGP,
  VitalProductData,
  SlotIdentification,
  MSI,
  CPCIHotSwap,
  PCIX,
  HyperTransport,
  VendorSpecific,
  DebugPort,
  CentralResourceControl,
  PCIHotPlug,
  PCISubsysVendorId,
  AGP8x,
  SecureDevice,
  PCIExpress,
  MSIX,
  SATADataIndex,
  AdvancedFeatures,
  EnhancedAlloc,
  FlatteningPortalBridge,
};

s2::string to_string(PciCapability p) {
  const char* pci_caps[] = {
    "-",
    "pm",
    "agp",
    "vpd",
    "slot",
    "msi",
    "cpci_hs",
    "pci-x",
    "ht",
    "vendor",
    "debug",
    "ccrc",
    "hotplug",
    "subsys_vid",
    "agp-br",
    "secure",
    "pci-ex",
    "msi-x",
    "satadata",
    "pciadv",
    "enhalloc",
    "flatportal",
  };
  if ((int)p < sizeof(pci_caps) / sizeof(void*)) {
    return pci_caps[(int)p];
  }
  return "unknown";
}

struct CapIterator {
  CapIterator(volatile uint8_t* p, uint8_t capptr) : p(p), cap(capptr) {
  }
  s2::pair<PciCapability, volatile uint8_t*> operator*() {
    return { (PciCapability)p[cap], p + cap + 2 };
  }
  struct sentinel {};
  bool operator==(const sentinel&) const {
    return cap == 0;
  }
  auto& operator++() {
    cap = p[cap+1];
    return *this;
  }
  auto& begin() { return *this; }
  auto end() { return sentinel(); }
  volatile uint8_t* p;
  uint8_t cap;
};

PciDevice::PciDevice(uintptr_t cfgSpace)
: confSpace(cfgSpace, 0x1000, DeviceRegisters)
, conf((volatile PciCfgSpace*)confSpace.get())
{}

struct Msi32 {
  uint16_t messageControlReg;
  uint32_t messageAddr;
  uint16_t messageDataReg;
} __attribute__((packed));

static_assert(sizeof(Msi32) == 8);

struct Msi {
  uint16_t messageControlReg;
  uint64_t messageAddr;
  uint16_t messageDataReg;
} __attribute__((packed));

static_assert(sizeof(Msi) == 12);

struct MsiX {
  uint16_t messageControlReg;
  uint32_t bar_offset;
} __attribute__((packed));

static_assert(sizeof(MsiX) == 6);

struct MsiXEntry {
  uint64_t address;
  uint32_t value;
  uint32_t flags;
};

void PciDevice::RegisterInterruptHandler(s2::function<void()> OnInterrupt) {
  uint32_t vec = get_empty_interrupt_vector();
  interrupt_register(vec, OnInterrupt);
  debug("[PCI] Using interrupt vector {}\n", vec);

  for (auto [cap, ptr] : CapIterator((volatile uint8_t*)conf, conf->cap_ptr)) {
    if (cap == PciCapability::MSI) {
      uint32_t tptr = 0xFEE00000;
      uint16_t data = (uint16_t)vec;
      volatile Msi* msi = (volatile Msi*)ptr;
      uint16_t msgControlIn = msi->messageControlReg & 0xFF8F; // ensure we do not enable more than one interrupt
      if ((msgControlIn & 0x80) == 0x80) {
        msi->messageAddr = tptr;
        msi->messageDataReg = data;
      } else {
        volatile Msi32* msi32 = (volatile Msi32*)ptr;
        msi32->messageAddr = tptr;
        msi32->messageDataReg = data;
      }

      msi->messageControlReg = msgControlIn | 0x1;
      return;
    } else if (cap == PciCapability::MSIX) {
      uint32_t tptr = 0xFEE00000;
      uint16_t data = (uint16_t)vec;
      volatile MsiX* msix = (volatile MsiX*)ptr;
      uint32_t bar_offset = msix->bar_offset;
      uint32_t barId = bar_offset & 0x7;
      mapping bar(conf, (PciBars)barId);

      uint32_t offset = bar_offset & 0xFFFFFFF8;
      MsiXEntry* list = (MsiXEntry*)((uintptr_t)bar.get() + offset);
      list[0].address = tptr;
      list[0].value = data;
      list[0].flags = 0;
      
      msix->messageControlReg = msix->messageControlReg | 0x8000;
      return;
    }
  }
  debug("[PCI] Device has no usable interrupt handling\n");
  assert(false);
}


