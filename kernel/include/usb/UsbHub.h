#pragma once

#include "usb/UsbCore.h"

struct UsbHubDevice {
  UsbHubDevice(UsbDevice& dev);
  s2::future<void> start();
  UsbDevice& dev;
};

struct UsbHub : public UsbDriver {
  static void Initialize();
  void AddDevice(UsbDevice& dev) override;
  void AddInterface(UsbInterface& interface) override;
};


