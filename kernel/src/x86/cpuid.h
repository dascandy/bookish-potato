#pragma once

#include <cstdint>

#ifdef __x86_64__

struct CpuidResult {
  uint32_t eax, ebx, ecx, edx; 
};

inline CpuidResult cpuid(uint32_t eax, uint32_t ecx = 0) {
  CpuidResult res;
  asm("cpuid" : "=a"(res.eax), "=b"(res.ebx), "=c"(res.ecx), "=d"(res.edx) : "a"(eax), "c"(ecx));
  return res;
}

#endif


