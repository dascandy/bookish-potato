#pragma once

#include <cstdint>
#include "map.h"

struct PciCfgSpace {
  uint32_t vendor_device;
  uint32_t status_cmd;
  uint32_t classcode;
  uint32_t cache;
  uint32_t bar[6];
  uint32_t cardbus_cis;
  uint32_t subsys_vendor_device;
  uint32_t expansion_rom;
  uint32_t cap_ptr;
  uint32_t reserved;
  uint32_t latency_interrupt;
  uint32_t caps[0x30];
  uint32_t extcap;
  uint32_t extcapspace[0x3bf];
};

struct PciDevice {
  PciDevice(uintptr_t cfgSpace);
  mapping confSpace;
  volatile PciCfgSpace* conf;
};

/*
class PciBridge : public PciDevice {
public:
  PciBridge(pcidevice dev, uint8_t subbusid);
  void AddDevice(pcidevice dev, PciDevice* devobj);
};
*/

static_assert(sizeof(PciCfgSpace) == 0x1000);

PciDevice* pci_discover(uintptr_t hwAddress, uint8_t busCount);


