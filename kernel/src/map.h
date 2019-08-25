#pragma once

#include <cstdint>

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

