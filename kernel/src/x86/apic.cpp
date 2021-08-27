#include "apic.h"
#include "io.h"
#include "map.h"
#include "cpuid.h"
#include <cassert>
#include "debug.h"

#ifdef __x86_64__

namespace Apic 
{
  static const uint32_t IA32_APIC_BASE = 0x1B;
  static bool useMsr = false;
  static mapping mmio;

  enum {
    LocalAPIC_ID = 2,
    LocalAPIC_Version = 3,
    TPR = 8,
    PPR = 0x0A,
    EOI = 0x0B,
    LDR = 0x0D,
    Spurious = 0x0F,
    ISRBASE = 0x10,
    TMRBASE = 0x18,
    IRRBASE = 0x20,
    ESR = 0x28,
    ICR = 0x30,
    // 0x31 == ICR high bits, only in MMIO mode
    LVT_Timer = 0x32,
    LVT_Thermal = 0x33,
    LVT_Perfmon = 0x34,
    LVT_LINT0 = 0x35,
    LVT_LINT1 = 0x36,
    LVT_Error = 0x37,
    TimerInitialCount = 0x38,
    TimerCurrentCount = 0x39,
    TimerDivideConfig = 0x3E,
    SelfIPI = 0x3F,
  };

  static bool isX2() {
    return (cpuid(1).ecx & 0x200000) == 0x200000;
  }

  void init() {
    if (isX2()) {
      useMsr = true;
      wrmsr(IA32_APIC_BASE, rdmsr(IA32_APIC_BASE) | 0xC00);
    } else {
      useMsr = false;
      mmio = mapping(0xFEE00000, 0x1000, DeviceRegisters);
    }

    write(Spurious, 0x130);
  }

  void write(uint8_t reg, uint64_t value)
  {
    if (useMsr) {
      wrmsr(0x800 + reg, value);
    } else {
      mmio_write<uint32_t>((uintptr_t)mmio.get() + reg * 0x10, value);
    }
  }

  uint64_t read(uint8_t reg)
  {
    if (useMsr) {
      return rdmsr(0x800 + reg);
    } else {
      return mmio_read<uint32_t>((uintptr_t)mmio.get() + reg * 0x10);
    }
  }

  void start_secondary_cpus() {
    
  }
}

void plat_endofinterrupt() {
  Apic::write(Apic::EOI, 0);
}

#endif


