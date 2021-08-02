#pragma once

#include "usb/UsbCore.h"

struct HidField {
  uint32_t usage;
  uint16_t bitOffset;
  uint8_t bitLength;
  bool isIndex;
};

struct UsbHidDevice {
  UsbHidDevice(UsbInterface& in);
  s2::future<void> start();
  void ParseReport(s2::span<const uint8_t> data);
  
  UsbInterface& in;
  s2::vector<HidField> input, output;
};

struct UsbHid : public UsbDriver {
  static void Initialize();
  void AddDevice(UsbDevice& dev) override;
  void AddInterface(UsbInterface& interface) override;
};


