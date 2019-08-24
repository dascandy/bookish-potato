#pragma once

#include "pci.h"

class XhciDevice : public PciDevice {
public:
  XhciDevice(pcidevice dev);
private:
  void start();
  pcidevice dev;
  uintptr_t cr, rr, doorbell;
};


