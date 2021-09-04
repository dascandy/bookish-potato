#pragma once

#include "usb/UsbCore.h"
#include "ui.h"

struct HidField {
  uint32_t usage;
  uint16_t bitOffset;
  uint8_t bitLength;
  bool isIndex, isRelative;
};

struct UsbHidDevice : public HidDevice {
  UsbHidDevice(UsbInterface& in);
  s2::future<void> start();
  void ParseReport(s2::span<const uint8_t> data);
  void HandleReport(s2::span<const uint8_t> report);
  Type getType() override;
  
  UsbInterface& in;
  Type deviceType;
  s2::vector<HidField> input, output;
};

struct UsbHid : public UsbDriver {
  static void Initialize();
  void AddDevice(UsbDevice& dev) override;
  void AddInterface(UsbInterface& interface) override;
};


