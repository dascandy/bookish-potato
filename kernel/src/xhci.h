#pragma once

#include "pci.h"
#include <cstdint>

class UsbHost {
public:
  
};

enum UsbDeviceState {
  New,
  Broken,
  Active,
  Suspend,
  Off
};

struct UsbDevice {
  UsbDevice();
  UsbDeviceState state = New;
  UsbHost& host;
  uint8_t slotId;
  char descriptors[256];
  char buffer[1024];
};

class XhciDevice : public PciDevice, public UsbHost {
public:
  XhciDevice(pcidevice dev);
private:
  uint64_t CreateContext(uint32_t scratchcount);
  void port_startup(size_t portno);
  void start();
  pcidevice dev;
  uintptr_t cr, opregs, rr, doorbell;
  std::vector<UsbDevice*> devices;
};


