#include "timer.h"
#include <cstdint>
#include "io.h"
#include <cstring>
#include "interrupt.h"
#include "rpi_interrupt.h"
#include "debug.h"

#ifdef __aarch64__
static uintptr_t timer_base;

void rpi_timer_interrupt() {
  timer_check();
  mmio_write(timer_base, 2);
}

void timer_init(uintptr_t mmio_base) {
  timer_base = mmio_base + 0x3000;

  // TODO: change timer_check to instead schedule a timer_check on the main loop to avoid race conditions
  interrupt_unmask(1);
  interrupt_register([](){ rpi_timer_interrupt(); });
}

void timer_set_interrupt(uint64_t nextTimeout) {
  mmio_write(timer_base + 16, (uint32_t)nextTimeout);
}

uint64_t get_timer_value() {
  // This logic is for when the 64-bit timer overflows between the first and second reads of high. 
  uint32_t high1 = mmio_read<uint32_t>(timer_base + 8), 
           low1 = mmio_read<uint32_t>(timer_base + 4), 
           high2 = mmio_read<uint32_t>(timer_base + 8);
  // If it didn't overflow then low is fine.
  if (high1 == high2) return (uint64_t(high1) << 32) + low1;
  // If it did, then low might match high2 or high1 - we don't know. Just reread, because it's not overflowing now.
  return (uint64_t(high1) << 32) + mmio_read<uint32_t>(timer_base + 4);
}

void delay_ns(uint64_t count) {
  asm volatile("__delay_%=: nop; subs %[count], %[count], #1; bne __delay_%=\n"
     : "=r"(count): [count]"0"(count) : "cc");
}
#endif


