#include "cpuid.h"
#include "debug.h"
#include <cstring>

void cpu_init() {
  auto zero = cpuid(0);
  uint32_t max = zero.eax;
  (void)max;
  char CpuVendor[13] = {};
  memcpy(CpuVendor, &zero.ebx, 4);
  memcpy(CpuVendor + 4, &zero.edx, 4);
  memcpy(CpuVendor + 8, &zero.ecx, 4);
  auto one = cpuid(1);
  uint8_t stepping = (one.eax & 0xF);
  uint8_t model = (one.eax & 0xF0) >> 4;
  uint16_t family = (one.eax & 0xF00) >> 8;
  if (family == 6 || family == 15) {
    model |= (one.eax & 0xF0000) >> 12;
  }
  if (family == 15) {
    family += (one.eax & 0xFF00000) >> 20;
  }
  auto ext = cpuid(0x80000000);
  debug("[CPU] Max base function {x}, max ext function {x}\n", zero.eax, ext.eax);
  debug("[CPU] Found {s} CPU family {} model {} stepping {}\n", CpuVendor, family, model, stepping);

  char CpuBrandedName[49] = {};
  auto ext2 = cpuid(0x80000002), ext3 = cpuid(0x80000003), ext4 = cpuid(0x80000004);
  memcpy(CpuBrandedName, &ext2, 16);
  memcpy(CpuBrandedName + 16, &ext3, 16);
  memcpy(CpuBrandedName + 32, &ext4, 16);
  debug("[CPU] {s}\n", CpuBrandedName);

  auto cpuinfo = cpuid(0x80000008);
  debug("[CPU] {} phys addr bits, {} linear addr bits, {} cores\n", (cpuinfo.eax & 0xFF), (cpuinfo.eax & 0xFF00) >> 8, (cpuinfo.ecx & 0xFF) + 1);
}


