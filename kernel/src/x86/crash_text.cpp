#include "crash.h"
#include "debug.h"

#ifdef __x86_64__
static const char hextab[] = "0123456789abcdef";
void crash_hexdump_region(size_t y, const uint8_t* num, size_t length, uint8_t attributes) {
  for (size_t n = 0; n < length; n++) {
    debug_char((attributes << 8) | hextab[num[n] >> 4]);
    debug_char((attributes << 8) | hextab[num[n] & 0xF]);
    debug_char(' ');
    if (n % 16 == 7) {
      debug_char(' ');
    }
    if (n % 16 == 15) {
      debug_char('\n');
    }
  }
}

void crash_hexdump(size_t x, size_t y, const uint8_t* num, size_t length, uint8_t attributes) {
  for (size_t n = 0; n < length; n++) {
    debug_char((attributes << 8) | hextab[num[length-n-1] >> 4]);
    debug_char((attributes << 8) | hextab[num[length-n-1] & 0xF]);
  }
}

void crash_text(size_t x, size_t y, const char* text, uint8_t attributes) {
  while (*text) {
    debug_char(*text++);
  }
}

static void clearscreen() {
  for (size_t n = 0; n < 3; n++) {
    debug_char('\n');
  }
}

struct registers {
    uint64_t cr4;
    uint64_t cr3;
    uint64_t cr2;
    uint64_t cr0;
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12; 
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rbp;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rbx;
    uint64_t rax;
    uint64_t exception;
    uint64_t errorcode;
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
};

[[noreturn]] void unhandled_interrupt_(const registers* regs) {
  clearscreen();
  debug("\x1b[31mUnhandled exception/interrupt 0x{02x}\x1b[0m\n", regs->exception);
  debug("rip={16x} rfl={16x} errc={x}\n\n", regs->rip, regs->rflags, regs->errorcode);
  debug("rax={16x} rbx={16x}\n", regs->rax, regs->rbx);
  debug("rcx={16x} rdx={16x}\n", regs->rcx, regs->rdx);
  debug("rsi={16x} rdi={16x}\n", regs->rsi, regs->rdi);
  debug("rsp={16x} rbp={16x}\n", regs->rsp, regs->rbp);
  debug(" r8={16x}  r9={16x}\n", regs->r8, regs->r9);
  debug("r10={16x} r11={16x}\n", regs->r10, regs->r11);
  debug("r12={16x} r13={16x}\n", regs->r12, regs->r13);
  debug("r14={16x} r15={16x}\n\n", regs->r14, regs->r15);
  debug("cr0={16x} cr2={16x}\n", regs->cr0, regs->cr2);
  debug("cr3={16x} cr4={16x}\n", regs->cr3, regs->cr4);

  while (1) {}
}

#endif

