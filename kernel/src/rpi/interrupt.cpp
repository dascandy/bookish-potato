#ifdef __aarch64__

#include "timer.h"
#include "interrupt.h"
#include "rpi_interrupt.h"
#include "debug.h"
#include "io.h"
#include <cstring>

extern uint32_t Vector_table_el1;
alignas(4096) uint8_t interrupt_stack[8192];

uintptr_t int_base;

enum registers {
  pending3 = 0x200,
  pending1 = 0x204,
  pending2 = 0x208,
  fiq = 0x20c,
  enable1 = 0x210,
  enable2 = 0x214,
  enable3 = 0x218,
  disable1 = 0x21c,
  disable2 = 0x220,
  disable3 = 0x224,
};

void interrupt_init(uintptr_t mmio_base) {
  int_base = mmio_base + 0xB000;
  asm volatile ("msr vbar_el1, %0" :: "r"(&Vector_table_el1));
  asm volatile ("msr daifclr, #0x7");
}

void halt_for_interrupts() {
  // enable interrupts
  asm volatile ("wfe");
  // disable interrupts
}

void interrupt_unmask(int vector) {
  if (vector < 32)
    mmio_write(int_base + enable1, 1U << vector);
  else if (vector < 64)
    mmio_write(int_base + enable2, 1U << (vector - 32));
  else
    mmio_write(int_base + enable3, 1U << (vector - 64));
}

[[noreturn]] void crash(unsigned long type, unsigned long esr, unsigned long elr, unsigned long spsr, unsigned long far) {
  // print out interruption type
  switch(type % 4) {
    case 0: debug("Synchronous"); break;
    case 1: debug("IRQ"); break;
    case 2: debug("FIQ"); break;
    case 3: debug("SError"); break;
  }
  switch (type >> 2) {
    case 0: debug(" from sys SP0"); break;
    case 1: debug(" from sys"); break;
    case 2: debug(" from user AArch64"); break;
    case 3: debug(" from user AArch32"); break;
  }
  debug(": ");
  // decode exception type (some, not all. See ARM DDI0487B_b chapter D10.2.28)
  switch(esr>>26) {
    case 0b000000: debug("Unknown"); break;
    case 0b000001: debug("Trapped WFI/WFE"); break;
    case 0b001110: debug("Illegal execution"); break;
    case 0b010101: debug("System call"); break;
    case 0b100000: debug("Instruction abort, lower EL"); break;
    case 0b100001: debug("Instruction abort, same EL"); break;
    case 0b100010: debug("Instruction alignment fault"); break;
    case 0b100100: debug("Data abort, lower EL"); break;
    case 0b100101: debug("Data abort, same EL"); break;
    case 0b100110: debug("Stack alignment fault"); break;
    case 0b101100: debug("Floating point"); break;
    default: debug("Unknown"); break;
  }
  // decode data abort cause
  if(esr>>27==0b10010) {
    debug(", ");
    switch((esr>>2)&0x3) {
      case 0: debug("Address size fault"); break;
      case 1: debug("Translation fault"); break;
      case 2: debug("Access flag fault"); break;
      case 3: debug("Permission fault"); break;
    }
    switch(esr&0x3) {
      case 0: debug(" at level 0:\n"); break;
      case 1: debug(" at level 1:\n"); break;
      case 2: debug(" at level 2:\n"); break;
      case 3: debug(" at level 3:\n"); break;
    }
  }
  // dump registers
  debug(" ESR_EL1 {x}\n", esr);
  debug(" ELR_EL1 {x}\n", elr);
  debug(" SPSR_EL1 {x}\n", spsr);
  debug(" FAR_EL1 {x}\n", far);
  // no return from exception for now
  asm ("wfe");
  while(1) {}
}

extern "C" void serror_handler(unsigned long type, unsigned long esr, unsigned long elr, unsigned long spsr, unsigned long far) {
  crash(type, esr, elr, spsr, far);
}

extern "C" void sync_handler(unsigned long type, unsigned long esr, unsigned long elr, unsigned long spsr, unsigned long far) {
  crash(type, esr, elr, spsr, far);
}

extern "C" void fiq_handler(unsigned long type, unsigned long esr, unsigned long elr, unsigned long spsr, unsigned long far) {
  crash(type, esr, elr, spsr, far);
}

extern "C" void irq_handler(unsigned long type, unsigned long esr, unsigned long elr, unsigned long spsr, unsigned long far) {
  interrupt_check();
//  crash(type, esr, elr, spsr, far);
}

#endif

