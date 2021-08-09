#include "pci.h"
#include "io.h"
#include "nvme.h"
#include "usb/xhci.h"
#include "debug.h"
#include "bga.h"
#include "virtiogpu.h"

/*
PciBridge::PciBridge(pcidevice dev, uint8_t subbusid) {}
void PciBridge::AddDevice(pcidevice dev, PciDevice* devobj) {}
*/

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

PciDevice* pci_find_driver(uintptr_t cfgAddress, uint16_t vendor, uint16_t device, uint32_t devClass) {
  // 0. if it's a bridge, make it a bridge
  /*
  uint8_t header = pciread8(dev, 0x0E);
  if ( (header & 0x7F) == 0x01 )
  {
    uint8_t subbusid = pciread8(dev, 0x19);
    return pci_handle_bridge(dev, subbusid);
  }
  */

  // 1. find match for vendor/device
#ifdef __x86_64__
  if (vendor == 0x1234 && device == 0x1111) {
    new BgaFramebuffer(cfgAddress);
  } else if (vendor == 0x1af4 && device == 0x1050) {
    new VirtioGpu(cfgAddress);
  }
#endif

  // 2. find match for device class
  if (devClass == 0x010802) {
    return new NvmeDevice(cfgAddress);
  } else if (devClass == 0x0c0330) {
    return new XhciDevice(cfgAddress);
  }

  // 3. give up
  return nullptr;
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

PciDevice* pci_discover(uintptr_t hwAddress, uint8_t busCount) {
  for (size_t bus = 0; bus < busCount; bus++) {
    for (size_t slot = 0; slot < 32; slot++) {
      size_t num_functions = 1;
      for (size_t function = 0; function < num_functions; function++) {
        uintptr_t cfgAddress = hwAddress + bus * 1048576 + function * 4096 + slot * 32768;
        mapping cfg(cfgAddress, 0x1000, DeviceRegisters);
        volatile PciCfgSpace* conf = (volatile PciCfgSpace*)cfg.get();

        if (conf->vendor_device == 0 || conf->vendor_device == 0xFFFFFFFF)
          continue;
        if ((conf->cache & 0x800000) != 0)
          num_functions = 8;

        uint16_t device = conf->vendor_device >> 16;
        uint16_t vendor = conf->vendor_device & 0xFFFF;
        uint32_t devClass = (conf->classcode & 0xFFFFFF00) >> 8;
        pci_debug_print_device(vendor, device, devClass);
        debug(" cap( ");
        for (auto [cap, ptr] : CapIterator((volatile uint8_t*)conf, conf->cap_ptr)) {
          debug("{} ", to_string(cap));
        }
        debug(")\n");
        pci_find_driver(cfgAddress, vendor, device, devClass);
      }
    }
  }
  return nullptr;
}

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


