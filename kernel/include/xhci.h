#pragma once

#include "pci.h"
#include <cstdint>
#include <cstddef>
#include <vector>
#include <map.h>
#include "future.h"
#include "usb.h"

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

struct InputContext;

struct xhci_command {
  uint64_t pointer;
  uint32_t status, control;
};

class XhciUsbDevice;
class XhciDevice : public PciDevice, public UsbHost {
  struct PendingCallback {
    uint64_t addr;
    s2::promise<uint64_t> p;
  };
public:
  XhciDevice(pcidevice dev);
private:
  s2::future<void> TryStartPort(uint8_t portno);
  uint64_t CreateContext(uint32_t scratchcount);
  s2::future<uint64_t> RunCommand(xhci_command cmd);
  void HandleInterrupt();
  void start();
  s2::future<uint64_t> RegisterStatus(uintptr_t address);
  pcidevice dev;
  mapping bar1;
  mapping dcbaa;
  mapping commandRing;
  mapping eventRing;
  mapping ports;
  uintptr_t cr, opregs, rr, doorbell, commandRingPhysical, eventRingPhysical;
  uint16_t currentCommand = 0, currentFlag = 0, commandRingSize = 0x100, eventRingIndex = 0, currentEventFlag;
  friend class XhciUsbDevice;
  s2::vector<XhciUsbDevice*> devices;
  s2::vector<PendingCallback> callbacks;
};

class XhciUsbDevice : public UsbDevice {
public:
  XhciUsbDevice(XhciDevice* host, uint8_t id);
  s2::future<void> start();
  uintptr_t EnqueueCommand(xhci_command cmd);
  void RingDoorbell(uint8_t endpoint = 0, bool in = true);
  mapping portmap;
  XhciDevice* host;
  InputContext* port;
  uint8_t slotId;
  uintptr_t op_port;
  DeviceDescriptor dd;
};


