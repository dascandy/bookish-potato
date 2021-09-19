#pragma once

#include "usb/UsbCore.h"
#include "map.h"
#include "vfs.h"

struct UsbStorageDevice final : public Disk {
  UsbStorageDevice(UsbInterface& in);
  s2::future<void> start();
  UsbInterface& in;
  UsbEndpoint* oute;
  UsbEndpoint* ine;
  uint32_t sectorsize;
  s2::future<s2::pair<uint8_t, uint32_t>> RunCommand(s2::span<const uint8_t> command, bool write, uint8_t lun, size_t transferSize, PageSGList buffer);
  s2::future<PageSGList> read(uint64_t startblock, uint32_t blockCount);
  s2::future<void> write(uint64_t startblock, uint32_t blockCount, PageSGList buffer);
  s2::future<void> trim(uint64_t startblock, uint32_t blockCount);
  s2::future<void> flush();
};

struct UsbStorage : public UsbDriver {
  static void Initialize();
  void AddDevice(UsbDevice& dev) override;
  void AddInterface(UsbInterface& interface) override;
};


