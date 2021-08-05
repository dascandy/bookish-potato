#include "apic.h"
#include "io.h"
#include "map.h"
#include "cpuid.h"
#include <cassert>

#ifdef __x86_64__

namespace Apic 
{
  static const uint32_t IA32_APIC_BASE = 0x1B;

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

  /*
  static volatile uint32_t *regs = (uint32_t *)0xFFFFFF00F0000000;
  static volatile struct ioapic {
    uint32_t address;
    uint32_t pad[3];
    uint32_t data;
  } *ioapic = (struct ioapic*)0xFFFFFF00E0000000;

  void ioapic_set_interrupt(uint8_t intsrc, uint8_t intno, uint8_t destcpu) {
    ioapic->address = intsrc * 2 + 0x10;
    ioapic->data |= 0x10000;
    ioapic->address = intsrc * 2 + 0x11;
    ioapic->data = (uint32_t)destcpu << 24;
    ioapic->address = intsrc * 2 + 0x10;
    ioapic->data = (ioapic->data & 0xF000) | intno;
  }
*/
  static bool isX2() {
    return (cpuid(1).ecx & 0x200000) == 0x200000;
  }

  void init() {
/*
    assert(isX2());
    wrmsr(IA32_APIC_BASE, rdmsr(IA32_APIC_BASE) | 0xC00);

    write(Spurious, read(Spurious) | 0x100);
    */
    /*
    platform_map((void*)ioapic, 0xFEC00000, DeviceRegisters);

//    for (size_t n = 0; n < 24; n++) {
    int n = 1; {
      ioapic_set_interrupt(n, 0x20 + n, 0);
    }
    */
  }

  void write(uint8_t reg, uint32_t value)
  {
    wrmsr(0x800 + reg, value);
  }

  uint32_t read(uint8_t reg)
  {
    return rdmsr(0x800 + reg);
  }

  void write_ICR(uint64_t value)
  {
    write(ICR, value);
  }

  uint64_t read_ICR()
  {
    return read(ICR);
  }
}

void x8664_endofinterrupt() {
  Apic::write(Apic::EOI, 0);
}

#endif


