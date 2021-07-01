#include "timer.h"
#include <cstdint>
#include "io.h"
#include <cstring>
#include "interrupt.h"
#include "debug.h"
#include "map.h"

struct Hpet {
  static const uint64_t HpetCapId = 0x0;
  static const uint64_t HpetGenCfg = 0x10;
  static const uint64_t HpetGenIntStatus = 0x20;
  static const uint64_t HpetMainCtrValue = 0xF0;
  static const uint64_t HpetTimerConfig = 0x100;
  static const uint64_t HpetTimerComparator = 0x108;
  static const uint64_t HpetTimerFsbRoute = 0x110;
  static const uint64_t TimerOffset = 0x20;

  Hpet(uintptr_t baseaddr)
  : map(baseaddr, 0x1000, MappingUse::DeviceRegisters)
  , base((uintptr_t)map.get())
  {
    uint64_t caps = mmio_read<uint64_t>((uintptr_t)map.get() + HpetCapId);
    clkperiodInFs = (uint32_t)(caps >> 32);
    uint16_t vendorid = (uint16_t)(caps >> 16);
    if (clkperiodInFs < 1000000) {
      debug("FATAL: found HPET device with clock indicated above 1GHz.\n");
      while (1) {}
    }
    debug("found HPET timer rev {} with {} timers\n", caps & 0xFF, 1 + ((caps >> 8) & 0x1F));
    uint64_t confreg = mmio_read<uint64_t>(base + HpetGenCfg);
    debug("clock period of {} fs, legacy {s}{s}\n", clkperiodInFs, (confreg & 2) ? "enabled" : "disabled", (confreg & 1) ? ", timer running" : "");
    uint64_t timercaps = mmio_read<uint64_t>(base + HpetTimerConfig);
    debug("timer caps: {s}{s}{s}\n", timercaps & 0x8000 ? " FSB" : "", timercaps & 0x20 ? " 64" : "", timercaps & 0x10 ? " rep" : "");
    //mmio_write<uint64_t>(base + HpetTimerConfig, 
  }
  void set_interrupt(uint64_t nextTimeoutInUs) {
    uint64_t curTimer = mmio_read<uint64_t>(base + HpetMainCtrValue);
    curTimer += (nextTimeoutInUs + 1) * clkperiodInFs / 1000000000;
    mmio_write<uint64_t>(base + HpetTimerComparator, curTimer);
  }
  uint64_t get_timer_value() {
    // TODO: this does overflow after ~5 hours. Implement with uint128_t
    return mmio_read<uint64_t>(base + HpetMainCtrValue) * clkperiodInFs / 1000000000;
  }
  mapping map;
  uintptr_t base;
  uint64_t clkperiodInFs;
};

#ifdef __x86_64__
Hpet* hpet = nullptr;

void timer_init(uintptr_t hpet) {
  ::hpet = new Hpet(hpet);
}

void timer_set_interrupt(uint64_t nextTimeoutInUs) {
  hpet->set_interrupt(nextTimeoutInUs);
}

uint64_t get_timer_value() {
  return hpet->get_timer_value();
}

void delay_ns(uint64_t count) {
  asm volatile("__delay_%=: pause; sub %[count], 1; jne __delay_%=\n"
     : "=r"(count): [count]"0"(count) : "cc");
}
#endif


