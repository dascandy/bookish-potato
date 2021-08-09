#include <string.h>
#include "freepage.h"
#include "map.h"
#include "asa.h"
#include "pci.h"
#include "debug.h"
#include <cassert>

struct pte {
        uint64_t p:1;
        uint64_t rw:1;
        uint64_t us:1;
        uint64_t pwt:1;
        uint64_t pcd:1;
        uint64_t a:1;
        uint64_t d:1;
        uint64_t pat:1;
        uint64_t g:1;
        uint64_t avl1:3;
        uint64_t addr:40;
        uint64_t avl2:11;
        uint64_t nx:1;
};

#ifdef __x86_64__
static inline void invlpg(void* x) {
  asm volatile ("invlpg (%0)"::"r"(x));
}
#endif

void platform_map(void* virt_addr, uint64_t physaddr, MappingUse use) {
  uint64_t t;
  struct pte *entries;

  entries = (struct pte *)0xFFFFFFFFFFFFF000ULL;
  t = ((uint64_t)virt_addr >> (12+9*3));
  t &= 0x1FF;

  if (entries[t].p == 0) {
    entries[t].addr = freepage_get() >> 12;
    entries[t].p = 1;
    entries[t].rw = 1;
    entries[t].us = 1;
    entries[t].pwt = 1;
    entries[t].pcd = 1;
    memset((void *)(0xFFFFFFFFFFE00000 + (t << 12)), 0, 4096);
  }

  entries = (struct pte *)0xFFFFFFFFFFE00000ULL;
  t = ((uint64_t)virt_addr >> (12+9*2));
  t &= 0x3FFFF;

  if (entries[t].p == 0) {
    entries[t].addr = freepage_get() >> 12;
    entries[t].p = 1;
    entries[t].rw = 1;
    entries[t].us = 1;
    entries[t].pwt = 1;
    entries[t].pcd = 1;
    memset((void *)(0xFFFFFFFFC0000000 + (t << 12)), 0, 4096);
  }

  entries = (struct pte *)0xFFFFFFFFC0000000ULL;
  t = ((uint64_t)virt_addr >> (12+9*1));
  t &= 0x7FFFFFF;

  if (entries[t].p == 0) {
    entries[t].addr = freepage_get() >> 12;
    entries[t].p = 1;
    entries[t].rw = 1;
    entries[t].us = 1;
    entries[t].pwt = 1;
    entries[t].pcd = 1;
    memset((void *)(0xFFFFFF8000000000 + (t << 12)), 0, 4096);
  }

  entries = (struct pte *)0xFFFFFF8000000000;
  t = ((uint64_t)virt_addr >> (12+9*0));
  t &= 0xFFFFFFFFF;

  entries[t].addr = physaddr >> 12;
  entries[t].p = 1;
  entries[t].rw = 1;
  entries[t].us = 0;
  entries[t].nx = 1;
  entries[t].g = 1;
  entries[t].pcd = 0;
  entries[t].pwt = 0;
  entries[t].pat = 0;
  switch(use) {
  case DeviceRegisters:
  case DeviceMemory:
    entries[t].pcd = 1;
    entries[t].pwt = 1;
    break;
  case ReadWriteMemory:
    break;
  case ReadOnlyMemory:
  case CopyOnWriteMemory:
    entries[t].rw = 0;
    break;
  case ExecutableMemory:
    entries[t].nx = 0;
    break;
  }
}

uint64_t platform_unmap(void* addr) {
  uint64_t t;
  struct pte *entries;

  entries = (struct pte *)0xFFFFFFFFFFFFF000ULL;
  t = ((uint64_t)addr >> (12+9*3));
  t &= 0x1FF;
  if (entries[t].p == 0) {
    return (uint64_t)0;
  }

  entries = (struct pte *)0xFFFFFFFFFFE00000;
  t = ((uint64_t)addr >> (12+9*2));
  t &= 0x3FFFF;

  if (entries[t].p == 0) {
    return (uint64_t)0;
  }

  entries = (struct pte *)0xFFFFFFFFC0000000;
  t = ((uint64_t)addr >> (12+9*1));
  t &= 0x7FFFFFF;

  if (entries[t].p == 0) {
    return (uint64_t)0;
  }

  entries = (struct pte *)0xFFFFFF8000000000;
  t = ((uint64_t)addr >> (12+9*0));
  t &= 0xFFFFFFFFF;

  if (entries[t].p == 0) {
    return (uint64_t)0;
  }
  uint64_t p = entries[t].addr << 12;
  entries[t].p = 0;
#ifdef __x86_64__
  invlpg(addr);
#endif
  return p;
}

mapping::mapping()
: bytecount(0)
, virtaddr(0)
, address(0)
, offset(0)
{}

mapping::mapping(uintptr_t address, size_t bytes, MappingUse use) 
: bytecount(bytes)
, virtaddr(asa_alloc(bytes))
, address(address)
, offset(address & 0xFFF)
{
  if (offset) {
    bytes += 0x1000;
    address -= address & 0xFFF;
  }
  for (size_t n = 0; n < bytecount; n += 4096) {
    platform_map((void*)(virtaddr + n), address + n, use);
  }
}

mapping::mapping(volatile PciCfgSpace* conf, PciBars barno, MappingUse use) 
{
  assert((uint32_t)barno < 6);
  address = conf->bar[(int)barno];
  switch (address & 0x7) {
    case 0x0:
      // 32-bit
    case 0x2:
      // 20-bit
      conf->bar[(int)barno] = 0xFFFFFFFF;
      bytecount = ~(conf->bar[(int)barno] & 0xFFFFFFF0) + 1;
      conf->bar[(int)barno] = (uint32_t)address;
      break;
    case 0x4:
      // 64-bit
      address |= (uint64_t)conf->bar[(int)barno + 1] << 32;
      conf->bar[(int)barno] = 0xFFFFFFFF;
      conf->bar[(int)barno + 1] = 0xFFFFFFFF;
      bytecount = ~((((uint64_t)conf->bar[(int)barno + 1] << 32) | conf->bar[(int)barno]) & 0xFFFFFFFFFFFFFFF0ULL) + 0x1;
      conf->bar[(int)barno] = (uint32_t)address;
      conf->bar[(int)barno + 1] = (uint32_t)(address >> 32);
      break;
    default:
    // invalid or IO space
      bytecount = 0;
      virtaddr = 0;
      return;
  }
  address -= (address & 0xF);
  offset = address & 0xFFF;
  if (offset) {
    bytecount += 0x1000;
    address -= address & 0xFFF;
  }
  virtaddr = asa_alloc(bytecount);
  for (size_t n = 0; n < bytecount; n += 4096) {
    platform_map((void*)(virtaddr + n), address + n, use);
  }
}

mapping& mapping::operator=(mapping&& rhs) {
  bytecount = rhs.bytecount;
  virtaddr = rhs.virtaddr;
  address = rhs.address;
  offset = rhs.offset;
  rhs.bytecount = 0;
  rhs.virtaddr = 0;
  rhs.address = 0;
  rhs.offset = 0;
  return *this;
}

mapping::mapping(mapping&& rhs) 
: bytecount(rhs.bytecount)
, virtaddr(rhs.virtaddr)
, address(rhs.address)
, offset(rhs.offset)
{
  rhs.bytecount = 0;
  rhs.virtaddr = 0;
  rhs.address = 0;
  rhs.offset = 0;
}

mapping::~mapping() {
  for (size_t n = 0; n < bytecount; n += 4096) {
    platform_unmap((void*)(virtaddr + n));
  }
  if (bytecount)
    asa_free(virtaddr, bytecount);
}

void* mapping::get() {
  return reinterpret_cast<void*>(virtaddr + offset);
}

uint64_t mapping::to_physical(void* p) {
  uintptr_t virt = (uintptr_t)p;
  return virt - virtaddr + address;
}


