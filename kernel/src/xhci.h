#pragma once

#include "pci.h"
#include <cstdint>
#include <cstddef>
#include <vector>
#include <map.h>

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
  UsbDeviceState state = New;
  char descriptors[256];
  char buffer[1024];
};

class XhciUsbDevice;
class XhciDevice : public PciDevice, public UsbHost {
public:
  XhciDevice(pcidevice dev);
private:
  uint64_t CreateContext(uint32_t scratchcount);
  void start();
  pcidevice dev;
  mapping bar1;
  uintptr_t cr, opregs, rr, doorbell;
  friend class XhciUsbDevice;
  std::vector<XhciUsbDevice*> devices;
};

class XhciUsbDevice : public UsbDevice {
public:
	XhciUsbDevice(XhciDevice* host, uint8_t id);
  XhciDevice* host;
  uint8_t slotId;
};


