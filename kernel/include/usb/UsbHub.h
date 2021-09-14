#pragma once

#include "usb/UsbCore.h"

struct UsbHubDevice {
  UsbHubDevice(UsbInterface& in);
  s2::future<void> start();
  UsbInterface& in;
};

struct UsbHub : public UsbDriver {
  static void Initialize();
  void AddDevice(UsbDevice& dev) override;
  void AddInterface(UsbInterface& interface) override;
};


