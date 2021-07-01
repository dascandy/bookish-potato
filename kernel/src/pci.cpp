#include "pci.h"
#include "io.h"
#include "nvme.h"
#include "xhci.h"
#include "debug.h"
#ifdef __x86_64__
#include "x86/bga.h"
#endif

PciBridge::PciBridge(pcidevice dev, uint8_t subbusid) {}
void PciBridge::AddDevice(pcidevice dev, PciDevice* devobj) {}

uint8_t pciread8(pcidevice dev, uint8_t regno) {
  uint32_t value = pciread32(dev, regno);
  return (value >> (8*(regno&3))) & 0xFF;
}

uint16_t pciread16(pcidevice dev, uint8_t regno) {
  uint32_t value = pciread32(dev, regno);
  return (value >> (8*(regno&2))) & 0xFFFF;
}

uint32_t pciread32(pcidevice dev, uint8_t regno) {
#ifdef __x86_64__
  outd(0xCF8, 0x80000000 |            // enable
     (dev << 8) |    // function
     (regno & 0xFC));          // register
  return ind(0xCFC);
#else
  return mmio_read<uint32_t>(0xfd500000 + dev * 4096 + (regno & 0xFFC));
#endif
}

void pciwrite8(pcidevice dev, uint8_t regno, uint8_t value) {
  uint32_t cvalue = pciread32(dev, regno);
  cvalue &= ~(0xFF << (8*(regno & 3)));
  cvalue |= value << (8*(regno & 3));
  pciwrite32(dev, regno, cvalue);
}

void pciwrite16(pcidevice dev, uint8_t regno, uint16_t value) {
  uint32_t cvalue = pciread32(dev, regno);
  cvalue &= ~(0xFFFF << (8*(regno & 2)));
  cvalue |= value << (8*(regno & 2));
  pciwrite32(dev, regno, cvalue);
}

void pciwrite32(pcidevice dev, uint8_t regno, uint32_t value) {
#ifdef __x86_64__
  outd(0xCF8, 0x80000000 |
       (dev << 8) |
       (regno & 0xFC));
  outd(0xCFC, value);
#else
  mmio_write(0x600000000 + dev * 4096 + (regno & 0xFFC), value);
#endif
}

void pci_detect(PciDevice* (*onDevice)(pcidevice), uint8_t bus, PciBridge* rootdev) {
  for ( unsigned int slot = 0; slot < 32; slot++ )
  {
    unsigned int num_functions = 1;
    for ( unsigned int function = 0; function < num_functions; function++ )
    {
      PciDevice* devobj = nullptr;
      pcidevice dev = (bus << 8) | (slot << 3) | function;
//      debug("pciread for dev {x}\n", dev);

      uint8_t header = pciread8(dev, 0x0E);
      if ( header & 0x80 )
        num_functions = 8;
      uint32_t vendor_device = pciread32(dev, 0);
      if (vendor_device != 0 && vendor_device != 0xFFFFFFFF)
        rootdev->AddDevice(dev, onDevice(dev));
    }
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

PciDevice* pci_find_driver(pcidevice dev, uint16_t vendor, uint16_t device, uint32_t devClass) {
  // 0. if it's a bridge, make it a bridge
  uint8_t header = pciread8(dev, 0x0E);
  if ( (header & 0x7F) == 0x01 )
  {
    uint8_t subbusid = pciread8(dev, 0x19);
    return pci_handle_bridge(dev, subbusid);
  }

  // 1. find match for vendor/device
#ifdef __x86_64__
  if (vendor == 0x1234 && device == 0x1111) {
    new BgaFramebuffer(dev);
  }
#endif

  // 2. find match for device class
  if (devClass == 0x010802) {
    return new NvmeDevice(dev);
  } else if (devClass == 0x0c0330) {
    return new XhciDevice(dev);
  }

  // 3. give up
  return nullptr;
}

void pci_debug_print_device(uint16_t vendor, uint16_t device, uint32_t devClass) {
  debug("found PCI vid={4x}", vendor);
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

PciDevice* pci_handle_bridge(pcidevice dev, uint8_t subbusid) {
  PciBridge* bridge = new PciBridge(dev, subbusid);
  debug("bridge starting\n");
  pci_detect([](pcidevice dev){
    uint32_t vendor_device = pciread32(dev, 0);
    uint16_t device = vendor_device >> 16;
    uint16_t vendor = vendor_device & 0xFFFF;
    uint32_t devClass = (pciread32(dev, 8) & 0xFFFFFF00) >> 8;
    pci_debug_print_device(vendor, device, devClass);
    uint8_t cap = pciread8(dev, 0x34);
    if (cap) {
      debug(" cap(");
      while (cap) {
        uint16_t val = pciread16(dev, cap);
        uint8_t capid = val & 0xFF;
        debug("{} ", s2::string_view(pci_caps[capid]));
        cap = val >> 8;
      }
      debug(")");
    }
    debug("\n");
    return pci_find_driver(dev, vendor, device, devClass);
  }, subbusid, bridge);
  debug("bridge ending\n");
  return bridge;
}


