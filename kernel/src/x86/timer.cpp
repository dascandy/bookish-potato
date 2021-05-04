#include "timer.h"
#include <cstdint>
#include "io.h"
#include <cstring>
#include "interrupt.h"
#include "debug.h"

#ifdef __x86_64__
void timer_init(uintptr_t) {
}

void timer_set_interrupt(uint64_t nextTimeout) {
  // TODO: set interrupt for this timeout value
}

uint64_t get_timer_value() {
  return 42;
}

void delay_ns(uint64_t count) {
  asm volatile("__delay_%=: pause; sub %[count], 1; jne __delay_%=\n"
     : "=r"(count): [count]"0"(count) : "cc");
}
#endif


