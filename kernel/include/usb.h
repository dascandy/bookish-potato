#pragma once

#include <cstdint>

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


