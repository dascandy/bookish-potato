#pragma once

#include <cstdint>
#include <cstddef>

enum MappingUse {
  DeviceRegisters,
  DeviceMemory,
  ReadWriteMemory,
  ReadOnlyMemory,
  CopyOnWriteMemory,
  ExecutableMemory,
};

void platform_map(void* virt_addr, uint64_t physaddr, MappingUse use);
uint64_t platform_unmap(void* addr);

struct mapping {
  mapping(uintptr_t address, size_t bytes, MappingUse use);
  ~mapping();
  void* get();
  uintptr_t virtaddr;
  size_t bytecount;
  size_t offset;
};

