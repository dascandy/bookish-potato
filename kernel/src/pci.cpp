#include "pci.h"
#include "io.h"

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
  // TODO
  return 0;
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
  // TODO
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

      uint8_t header = pciread8(dev, 0x0E);
      if ( header & 0x80 )
        num_functions = 8;
      uint32_t vendor_device = pciread32(dev, 0);
      if (vendor_device != 0 && vendor_device != 0xFFFFFFFF)
        rootdev->AddDevice(dev, onDevice(dev));
    }
  }
}



