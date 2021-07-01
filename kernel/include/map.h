#pragma once

#include <cstdint>
#include <cstddef>
#include "pci.h"

enum MappingUse {
  DeviceRegisters,
  DeviceMemory,
  ReadWriteMemory,
  ReadOnlyMemory,
  CopyOnWriteMemory,
  ExecutableMemory,
};

enum class PciBars {
  Bar0,
  Bar1,
  Bar2,
  Bar3,
  Bar4,
  Bar5,
};

void platform_map(void* virt_addr, uint64_t physaddr, MappingUse use);
uint64_t platform_unmap(void* addr);

struct mapping {
  mapping(uintptr_t address, size_t bytes, MappingUse use);
  mapping(pcidevice dev, PciBars barno, MappingUse use = MappingUse::DeviceRegisters);
  ~mapping();
  void* get();
  uintptr_t virtaddr;
  size_t bytecount;
  size_t offset;
};

