#include "UsbCore.h"
#include "debug.h"

UsbCore& UsbCore::Instance() {
  static UsbCore core;
  return core;
}

void UsbCore::RegisterPidVidDriver(uint16_t vid, uint16_t pid, UsbDriver& driver) {
  pidVidDrivers.emplace(((uint32_t)vid << 16) | pid, &driver);
}

void UsbCore::RegisterClassDriver(uint32_t classId, UsbDriver& driver) {
  classDrivers.emplace(classId, &driver);
}

void UsbCore::RegisterUsbDevice(UsbDevice& dev) {
  DeviceDescriptor& dd = dev.GetDeviceDescriptor();
  auto it = pidVidDrivers.find(((uint32_t)dd.vendorId << 16) | dd.deviceId);
  if (it != pidVidDrivers.end()) {
    // Pid-vid drivers get first choice for all devices they match
    // Keep a handle; we're not telling you when this thing disappears.
    it->second->AddDevice(dev);
  } else if (dev.GetConfigurationDescriptors().size() == 1) {
    dev.SetConfiguration(0);
  } else {
    debug("[USB] Found multi-configuration device {x}:{x} without owning driver?\n", dd.vendorId, dd.deviceId);
  }
}

void UsbCore::RegisterUsbInterface(UsbInterface& dev) {
  InterfaceDescriptor* id = (InterfaceDescriptor*)dev.GetInterfaceDescriptors()[0];
  uint32_t devClass = (id->deviceClass << 16) | (id->subClass << 8) | (id->protocol);
  if (auto it1 = classDrivers.find(devClass); it1 != classDrivers.end()) {
    it1->second->AddInterface(dev);
  } else if (auto it2 = classDrivers.find(devClass & 0xFFFF00); it2 != classDrivers.end()) {
    it2->second->AddInterface(dev);
  } else if (auto it3 = classDrivers.find(devClass & 0xFF0000); it3 != classDrivers.end()) {
    it3->second->AddInterface(dev);
  } else {
    debug("[USB] Unimplemented class {x}:{x}:{x} found\n", id->deviceClass, id->subClass, id->protocol);
  }
}

s2::future<s2::span<const uint8_t>> UsbInterface::GetDescriptor(DescriptorType descriptorType, uint8_t descriptorId) {
  auto start = co_await dev.RunCommandRequest(0x81, 6, ((uint16_t)descriptorType << 8) | descriptorId, interfaceNum, sizeof(UsbDescriptor));
  if (start.empty()) co_return {};
  UsbDescriptor* desc = (UsbDescriptor*)start.data();
  co_return co_await dev.RunCommandRequest(0x81, 6, ((uint16_t)descriptorType << 8) | descriptorId, interfaceNum, desc->length);
}

s2::vector<const UsbDescriptor*>& UsbInterface::GetInterfaceDescriptors() {
  return interfaceDescriptors;
}

UsbInterface::UsbInterface(UsbDevice& dev, uint8_t id, s2::vector<const UsbDescriptor*> interfaceDescriptor)
: dev(dev)
, interfaceNum(id)
, interfaceDescriptors(interfaceDescriptor)
{
}


