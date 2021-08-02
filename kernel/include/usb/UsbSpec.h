#pragma once

#include <cstdint>

enum class DescriptorType {
  Device = 0x1,
  Configuration = 0x2,
  String = 0x3,
  Interface = 0x4,
  Endpoint = 0x5,
  HID = 0x21,
  Report = 0x22,
};

struct UsbDescriptor {
  uint8_t length;
  uint8_t type;
};

struct DeviceDescriptor {
  uint8_t length;
  uint8_t type;
  uint16_t releasenum;
  uint8_t deviceClass;
  uint8_t subClass;
  uint8_t protocol;
  uint8_t maxPacketSize;
  uint16_t vendorId;
  uint16_t deviceId;
  uint16_t deviceRel;
  uint8_t manufacturer;
  uint8_t product;
  uint8_t serialnum;
  uint8_t configurations;
};

struct ConfigurationDescriptor {
  uint8_t length;
  uint8_t type;
  uint16_t totalLength;
  uint8_t numInterfaces;
  uint8_t configVal;
  uint8_t configString;
  uint8_t attributes;
  uint8_t maxPower;
};

struct InterfaceDescriptor {
  uint8_t length;
  uint8_t type;
  uint8_t interfaceNum;
  uint8_t alternateSet;
  uint8_t endpoints;
  uint8_t deviceClass;
  uint8_t subClass;
  uint8_t protocol;
  uint8_t interface;
};

struct EndpointDescriptor {
  uint8_t length;
  uint8_t type;
  uint8_t address;
  uint8_t attributes;
  uint16_t maxPacketSize;
  uint8_t interval;
};

struct StringLanguages {
  uint8_t length;
  uint8_t type;
  uint16_t languages[1];
};

struct HIDDescriptor {
  struct Entry {
    uint8_t descriptorType;
    int16_t descriptorLength;
  } __attribute__((packed));
  uint8_t length;
  uint8_t type;
  uint16_t hidVersion;
  uint8_t countryCode;
  uint8_t numDescriptors;
  Entry entries[1];
} __attribute__((packed));

static_assert(sizeof(HIDDescriptor) == 9);
static_assert(sizeof(HIDDescriptor::Entry) == 3);

struct ReportDescriptor {
  uint8_t length;
  uint8_t type;
  uint8_t data[1];
};

