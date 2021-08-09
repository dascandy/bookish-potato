#ifndef APIC_H
#define APIC_H

#include <stdint.h>

#ifdef __x86_64__

namespace Apic {
  void init();
  void write(uint8_t reg, uint64_t value);
  uint64_t read(uint8_t reg);
  void write_ICR(uint64_t value);
  uint64_t read_ICR();
}

#endif

#endif


