#pragma once

#include <flatmap>
#include <functional>
#include <cstdint>
#include "map.h"
#include "interrupt.h"

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
  void RegisterInterruptHandler(size_t number, s2::function<void()> OnInterrupt);
  mapping confSpace;
  volatile PciCfgSpace* conf;
};

class PciBridge : public PciDevice {
public:
  PciBridge(uintptr_t dev, uint8_t bus);
  void RegisterDevice(PciDevice* devobj);
private:
  s2::vector<PciDevice*> devices;
};

class PciCore {
public:
  static PciCore& Instance();
  void RegisterVidPidDriver(uint16_t vid, uint16_t pid, s2::function<PciDevice*(uintptr_t cfgSpace)>);
  void RegisterClassDriver(uint32_t classId, s2::function<PciDevice*(uintptr_t cfgSpace)>);
  PciDevice* RegisterPciDevice(uintptr_t configSpace, uint32_t vendorDevice, uint32_t deviceClass);
  void RegisterBusAddress(uintptr_t base, uint8_t busId, uint8_t busCount);
private:
  s2::flatmap<uint32_t, s2::function<PciDevice*(uintptr_t cfgSpace)>> pidVidDrivers;
  s2::flatmap<uint32_t, s2::function<PciDevice*(uintptr_t cfgSpace)>> classDrivers;
  s2::flatmap<uint8_t, uintptr_t> baseaddress;
  friend class PciBridge;
};

static_assert(sizeof(PciCfgSpace) == 0x1000);


