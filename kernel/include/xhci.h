#pragma once

#include "pci.h"
#include <cstdint>
#include <cstddef>
#include <vector>
#include <map.h>
#include "future.h"
#include "UsbCore.h"
#include <string>

struct InputContext;

struct xhci_command {
  uint64_t pointer;
  uint32_t status, control;
};

class XhciUsbDevice;
class XhciDevice : public PciDevice {
  struct PendingCallback {
    uint64_t addr;
    s2::promise<uint64_t> p;
  };
public:
  XhciDevice(pcidevice dev);
private:
  uint64_t CreateContext(uint32_t scratchcount);
  s2::future<uint64_t> RunCommand(xhci_command cmd);
  void HandleInterrupt();
  void start();
  s2::future<uint64_t> RegisterStatus(uintptr_t address);
 
  void ReportDevice(XhciUsbDevice& device);

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

class XhciUsbDevice final : public UsbDevice {
public:
  XhciUsbDevice(XhciDevice* host, uint8_t id);
  s2::future<void> start();

  s2::future<s2::span<const uint8_t>> RunCommandRequest(uint8_t requestType, uint8_t request, uint16_t value, uint16_t index, uint16_t length) override;
  DeviceDescriptor& GetDeviceDescriptor() override;
  const s2::vector<ConfigurationDescriptor*>& GetConfigurationDescriptors() override;
  s2::future<void> SetConfiguration(uint8_t configuration) override;

private:
  s2::future<bool> StartUp();
  uintptr_t EnqueueCommand(xhci_command cmd);
  void RingDoorbell(uint8_t endpoint = 0, bool in = true);
  s2::future<s2::string> GetStringDescriptor(uint8_t descriptorId, uint16_t languageId);
  s2::future<bool> GetDescriptor(DescriptorType type, uint8_t descriptorId, s2::span<uint8_t> descriptor);

  mapping portmap;
  XhciDevice* host;
  InputContext* port;
  uint8_t slotId;
  uintptr_t op_port;
  uint8_t portid;
  bool active;
  DeviceDescriptor dd;
  s2::vector<ConfigurationDescriptor*> cd;
  s2::vector<UsbInterface*> activeInterfaces;
  s2::string manufacturer, product, serial, config, interface;
};


