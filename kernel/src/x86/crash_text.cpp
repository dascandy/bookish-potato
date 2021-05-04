#include "crash.h"

#ifdef __x86_64__
static const char hextab[] = "0123456789abcdef";
void crash_hexdump_region(size_t y, const uint8_t* num, size_t length, uint8_t attributes) {
  uint16_t* ptr = (uint16_t*)(0xb8000) + y * 80;
  for (size_t n = 0; n < length; n++) {
    *ptr++ = (attributes << 8) | hextab[num[n] >> 4];
    *ptr++ = (attributes << 8) | hextab[num[n] & 0xF];
    *ptr++ = (attributes << 8) | ' ';
    if (n % 16 == 7) {
      *ptr++ = (attributes << 8) | ' ';
    }
    if (n % 16 == 15) {
      y++;
      ptr = (uint16_t*)(0xb8000) + y * 80;
    }
  }
}

void crash_hexdump(size_t x, size_t y, const uint8_t* num, size_t length, uint8_t attributes) {
  uint16_t* ptr = (uint16_t*)(0xb8000) + y * 80 + x;
  for (size_t n = 0; n < length; n++) {
    *ptr++ = (attributes << 8) | hextab[num[length-n-1] >> 4];
    *ptr++ = (attributes << 8) | hextab[num[length-n-1] & 0xF];
  }
}

void crash_text(size_t x, size_t y, const char* text, uint8_t attributes) {
  uint16_t* ptr = (uint16_t*)(0xb8000) + y * 80 + x;
  while (*text) {
    *ptr++ = (attributes << 8) | *text++;
  }
}

static void clearscreen() {
  uint16_t* ptr = (uint16_t*)0xb8000, *end = (uint16_t*)0xb8000 + 80*25;
  for (; ptr != end; ++ptr) *ptr = 0x720;
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
    uint64_t errorcode;
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
};

[[noreturn]] void unhandled_interrupt_(const registers* regs) {
  clearscreen();
  crash_text(10, 0, "Unhandled exception/interrupt", 7);
  crash_text(0, 2, "rax=", 7); crash_hexdump(4, 2, (const uint8_t*)&regs->rax, 8, 7);
  crash_text(30, 2, "rbx=", 7); crash_hexdump(34, 2, (const uint8_t*)&regs->rbx, 8, 7);
  crash_text(0, 3, "rcx=", 7); crash_hexdump(4, 3, (const uint8_t*)&regs->rcx, 8, 7);
  crash_text(30, 3, "rdx=", 7); crash_hexdump(34, 3, (const uint8_t*)&regs->rdx, 8, 7);
  crash_text(0, 4, "rsi=", 7); crash_hexdump(4, 4, (const uint8_t*)&regs->rsi, 8, 7);
  crash_text(30, 4, "rdi=", 7); crash_hexdump(34, 4, (const uint8_t*)&regs->rdi, 8, 7);
  crash_text(0, 5, "rsp=", 7); crash_hexdump(4, 5, (const uint8_t*)&regs->rsp, 8, 7);
  crash_text(30, 5, "rbp=", 7); crash_hexdump(34, 5, (const uint8_t*)&regs->rbp, 8, 7);
  crash_text(0, 6, "r8 =", 7); crash_hexdump(4, 6, (const uint8_t*)&regs->r8 , 8, 7);
  crash_text(30, 6, "r9 =", 7); crash_hexdump(34, 6, (const uint8_t*)&regs->r9 , 8, 7);
  crash_text(0, 7, "r10=", 7); crash_hexdump(4, 7, (const uint8_t*)&regs->r10, 8, 7);
  crash_text(30, 7, "r11=", 7); crash_hexdump(34, 7, (const uint8_t*)&regs->r11, 8, 7);
  crash_text(0, 8, "r12=", 7); crash_hexdump(4, 8, (const uint8_t*)&regs->r12, 8, 7);
  crash_text(30, 8, "r13=", 7); crash_hexdump(34, 8, (const uint8_t*)&regs->r13, 8, 7);
  crash_text(0, 9, "r14=", 7); crash_hexdump(4, 9, (const uint8_t*)&regs->r14, 8, 7);
  crash_text(30, 9, "r15=", 7); crash_hexdump(34, 9, (const uint8_t*)&regs->r15, 8, 7);

  crash_text(0,  11, "err=", 7); crash_hexdump(4, 11, (const uint8_t*)&regs->errorcode, 8, 7);
  crash_text(30, 11, "rip=", 7); crash_hexdump(34, 11, (const uint8_t*)&regs->rip, 8, 7);
  crash_text(60, 11, "rfl=", 7); crash_hexdump(64, 11, (const uint8_t*)&regs->rflags, 4, 7);

  crash_text(0,  13, "cr0=", 7); crash_hexdump(4,  13, (const uint8_t*)&regs->cr0, 8, 7);
  crash_text(30, 13, "cr2=", 7); crash_hexdump(34, 13, (const uint8_t*)&regs->cr2, 8, 7);
  crash_text(0,  14, "cr3=", 7); crash_hexdump(4,  14, (const uint8_t*)&regs->cr3, 8, 7);
  crash_text(30, 14, "cr4=", 7); crash_hexdump(34, 14, (const uint8_t*)&regs->cr4, 8, 7);

  while (1) {}
}

#endif

