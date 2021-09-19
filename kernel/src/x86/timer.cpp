#include "timer.h"
#include <cstdint>
#include "io.h"
#include <cstring>
#include "interrupt.h"
#include "debug.h"
#include "map.h"
#include <cassert>

#ifdef __x86_64__
struct Hpet {
  static const uint64_t HpetCapId = 0x0;
  static const uint64_t HpetGenCfg = 0x10;
  static const uint64_t HpetGenIntStatus = 0x20;
  static const uint64_t HpetMainCtrValue = 0xF0;
  static const uint64_t HpetTimerConfig = 0x100;
  static const uint64_t HpetTimerComparator = 0x108;
  static const uint64_t HpetTimerFsbRoute = 0x110;
  static const uint64_t TimerOffset = 0x20;

  static const uint16_t HpetRepeatingCapable = 0x10;
  static const uint16_t Hpet64bitCapable = 0x20;
  static const uint16_t HpetInterruptEnable = 0x4;
  static const uint16_t HpetFsbEnable = 0x4000;
  static const uint16_t HpetFsbCapable = 0x8000;

  Hpet(uintptr_t baseaddr)
  : map(baseaddr, 0x1000, MappingUse::DeviceRegisters)
  , base((uintptr_t)map.get())
  {
    uint64_t caps = mmio_read<uint64_t>((uintptr_t)map.get() + HpetCapId);
    clkperiodInFs = (uint32_t)(caps >> 32);
//    uint16_t vendorid = (uint16_t)(caps >> 16);
    if (clkperiodInFs < 1000000) {
      debug("[HPET] FATAL: found HPET device with clock indicated above 1GHz.\n");
      while (1) {}
    }
    debug("[HPET] found HPET timer rev {} with {} timers\n", caps & 0xFF, 1 + ((caps >> 8) & 0x1F));
    uint64_t confreg = mmio_read<uint64_t>(base + HpetGenCfg);
    mmio_write<uint64_t>(base + HpetGenCfg, confreg | 1);
    debug("[HPET] clock period of {} fs, legacy {s}{s}\n", clkperiodInFs, (confreg & 2) ? "enabled" : "disabled", (confreg & 1) ? ", timer running" : "");
    uint64_t timercaps = mmio_read<uint64_t>(base + HpetTimerConfig);
//    uint64_t curTimer = mmio_read<uint64_t>(base + HpetMainCtrValue);
    debug("[HPET] timer caps: {s}{s}{s}\n", 
          timercaps & HpetFsbCapable ? " FSB" : "", 
          timercaps & Hpet64bitCapable ? " 64" : "", 
          timercaps & HpetRepeatingCapable ? " rep" : "");
    assert(timercaps & HpetFsbCapable);
    assert(timercaps & Hpet64bitCapable);

    uint32_t vec = get_empty_interrupt_vector();
    interrupt_register(vec, timer_check);
    mmio_write<uint64_t>(base + HpetTimerFsbRoute, 0xFEE0000000000000 | vec);
    mmio_write<uint64_t>(base + HpetTimerConfig, timercaps | HpetFsbEnable | HpetInterruptEnable);
  }
  void set_interrupt(uint64_t nextTimeoutInUs) {
    uint64_t curTimer = mmio_read<uint64_t>(base + HpetMainCtrValue);
    curTimer += (nextTimeoutInUs + 1) * 1000000000 / clkperiodInFs;
    mmio_write<uint64_t>(base + HpetTimerComparator, curTimer);
    if (mmio_read<uint64_t>(base + HpetMainCtrValue) >= curTimer) {
      debug("[INT] TOO LATE\n");
    }
  }
  uint64_t get_timer_value() {
    // TODO: this does overflow after ~5 hours. Implement with uint128_t
    return mmio_read<uint64_t>(base + HpetMainCtrValue) * clkperiodInFs / 1000000000;
  }
  mapping map;
  uintptr_t base;
  uint64_t clkperiodInFs;
};

Hpet* hpet = nullptr;

void timer_init(uintptr_t hpet) {
  ::hpet = new Hpet(hpet);
}

void timer_set_interrupt(uint64_t nextTimeoutInUs) {
  hpet->set_interrupt(nextTimeoutInUs - get_timer_value());
}

uint64_t get_timer_value() {
  return hpet->get_timer_value();
}

void delay_ns(uint64_t count) {
  asm volatile("__delay_%=: pause; sub %[count], 1; jne __delay_%=\n"
     : "=r"(count): [count]"0"(count) : "cc");
}
#endif


