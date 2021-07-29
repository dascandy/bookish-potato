#pragma once

#include "usb.h"
#include <string>
#include <future.h>
#include <flatmap>

struct UsbDevice {
  virtual s2::future<s2::span<const uint8_t>> RunCommandRequest(uint8_t requestType, uint8_t request, uint16_t value, uint16_t index, uint16_t length) = 0;
  virtual DeviceDescriptor& GetDeviceDescriptor() = 0;
  virtual const s2::vector<ConfigurationDescriptor*>& GetConfigurationDescriptors() = 0;
  virtual s2::future<void> SetConfiguration(uint8_t configuration) = 0;
  s2::string manufacturer, product, serial;
};

struct UsbInterface {
  UsbInterface(UsbDevice& dev, uint8_t id, s2::vector<const UsbDescriptor*> interfaceDescriptor);
  s2::span<const uint8_t> descriptors;
  s2::future<s2::span<const uint8_t>> GetDescriptor(DescriptorType descriptorType, uint8_t descriptorId);
  UsbDevice& dev;
  s2::vector<const UsbDescriptor*> interfaceDescriptors;
  s2::vector<const UsbDescriptor*>& GetInterfaceDescriptors();
  uint8_t interfaceNum;
};

struct UsbDriver {
  virtual ~UsbDriver() = default;
  virtual void AddDevice(UsbDevice& dev) = 0;
  virtual void AddInterface(UsbInterface& interface) = 0;
};

class UsbCore {
public:
  static UsbCore& Instance();
  void RegisterPidVidDriver(uint16_t vid, uint16_t pid, UsbDriver& driver);
  void RegisterClassDriver(uint32_t classId, UsbDriver& driver);
  void RegisterUsbDevice(UsbDevice& dev);
  void RegisterUsbInterface(UsbInterface& dev);
private:
  s2::flatmap<uint32_t, UsbDriver*> pidVidDrivers;
  s2::flatmap<uint32_t, UsbDriver*> classDrivers;
};


