#pragma once

#include <cstdint>

typedef uint32_t pcidevice;

class PciDevice {
};

class PciBridge : public PciDevice {
public:
  PciBridge(pcidevice dev, uint8_t subbusid);
  void AddDevice(pcidevice dev, PciDevice* devobj);
};

PciDevice* pci_find_driver(pcidevice dev, uint16_t vendor, uint16_t device, uint32_t devClass);
void pci_debug_print_device(uint16_t vendor, uint16_t device, uint32_t devClass);
PciDevice* pci_handle_bridge(pcidevice dev, uint8_t subbusid);

void pci_detect(PciDevice* (*onDevice)(pcidevice), uint8_t bus, PciBridge* bridge);
uint8_t pciread8(pcidevice dev, uint8_t regno);
uint16_t pciread16(pcidevice dev, uint8_t regno);
uint32_t pciread32(pcidevice dev, uint8_t regno);
void pciwrite8(pcidevice dev, uint8_t regno, uint8_t value);
void pciwrite16(pcidevice dev, uint8_t regno, uint16_t value);
void pciwrite32(pcidevice dev, uint8_t regno, uint32_t value);


