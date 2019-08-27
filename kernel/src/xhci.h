#pragma once

#include "pci.h"
#include <cstdint>

class XhciDevice : public PciDevice {
public:
  XhciDevice(pcidevice dev);
private:
  uint64_t CreateContext(uint32_t scratchcount);
  void start();
  pcidevice dev;
  uintptr_t cr, rr, doorbell;
};


