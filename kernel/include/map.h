#pragma once

#include <vector>
#include <cstdint>
#include <cstddef>
#include "freepage.h"

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

struct PciCfgSpace;

void platform_map(void* virt_addr, uint64_t physaddr, MappingUse use);
uint64_t platform_unmap(void* addr);

struct PageSGList {
  s2::vector<uint64_t> pages;
};

struct mapping {
  mapping(uintptr_t address, size_t bytes, MappingUse use);
  mapping(PageSGList list);
  mapping();
  mapping(volatile PciCfgSpace* conf, PciBars barno, MappingUse use = MappingUse::DeviceRegisters);
  mapping& operator=(mapping&&);
  mapping(mapping&&);
  uint64_t to_physical(void* p);
  ~mapping();
  uint8_t* get();
  size_t bytecount;
  uintptr_t virtaddr;
  uintptr_t address;
  size_t offset;
};

template <typename T>
struct RegisterMapping : mapping {
  RegisterMapping() {}
  RegisterMapping(uintptr_t address) : mapping(address, sizeof(T), MappingUse::DeviceRegisters) {}
  RegisterMapping(volatile PciCfgSpace* conf, PciBars barno) : mapping(conf, barno, MappingUse::DeviceRegisters) {}
  ~RegisterMapping() {}
  volatile T* operator->() { return (volatile T*)get(); }
};

struct IoMemory {
  IoMemory(size_t pagecount) :
  m(freepage_get_range(pagecount), pagecount * 0x1000, DeviceMemory)
  {
  }
  ~IoMemory() {

  }
  IoMemory& operator=(IoMemory&&) = default;
  IoMemory(IoMemory&&) = default;
  uint8_t* get() { return (uint8_t*)m.get(); }
  uintptr_t hwaddress() { return m.to_physical(m.get()); }
private:
  mapping m;
};


