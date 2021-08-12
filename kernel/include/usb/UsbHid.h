#pragma once

#include "usb/UsbCore.h"

struct HidField {
  uint32_t usage;
  uint16_t bitOffset;
  uint8_t bitLength;
  bool isIndex, isRelative;
};

struct UsbHidDevice {
  enum class Type {
    UsbHidMouse = 0x10002,
    UsbHidJoystick = 0x10004,
    UsbHidGamepad = 0x10005,
    UsbHidKeyboard = 0x10006,
    UsbHidKeypad = 0x10007,
    UsbHidMultiAxisController = 0x10008,
    UsbHidTouchscreen = 0xd0004,
  };

  UsbHidDevice(UsbInterface& in);
  s2::future<void> start();
  void ParseReport(s2::span<const uint8_t> data);
  void HandleReport(s2::span<const uint8_t> report);
  
  UsbInterface& in;
  Type deviceType;
  s2::vector<HidField> input, output;
};

struct UsbHid : public UsbDriver {
  static void Initialize();
  void AddDevice(UsbDevice& dev) override;
  void AddInterface(UsbInterface& interface) override;
};


