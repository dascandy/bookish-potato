#include "interrupt.h"
#include "map.h"
#include <cstdint>
#include "crash.h"
#include "io.h"
#include <cstring>
#include "debug.h"
#include "apic.h"

#ifdef __x86_64__
struct gdt {
  uint16_t limit;
  uint16_t base1;
  uint8_t base2;
  uint8_t access;
  uint8_t limit_flags;
  uint8_t base3;
} gdt_[] = {
  { 0, 0, 0, 0, 0, 0 },
  { 0xFFFF, 0x0000, 0x00, 0x98, 0x2F, 0x00 },
  { 0xFFFF, 0x0000, 0x00, 0xF8, 0x2F, 0x00 },
  { 0xFFFF, 0x0000, 0x00, 0x92, 0x2F, 0x00 },
  { 0xFFFF, 0x0000, 0x00, 0x89, 0x1F, 0x00 }, // this and next are TSS locator
  { 0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00 },
};

struct load {
  uint16_t size;
  uint64_t offset;
} __attribute__((packed));

static load _loadgdt = {
  sizeof(gdt_)-1,
  (uint64_t)gdt_
};

struct tss {
  uint32_t reserve1;
  uint64_t rsp0;
  uint64_t rsp1;
  uint64_t rsp2;
  void* ist[8];
  uint64_t reserve3;
  uint16_t reserve4;
  char iobitmap[1];
} __attribute__ ((packed));
static tss _tss = {};

void tss_set_ist(IST ist, void* stackptr) {
  _tss.ist[(uint8_t)ist] = stackptr;
}

static struct idt_entry {
  uint16_t offset_low;
  uint16_t selector;
  uint8_t ist;
  uint8_t type_attr;
  uint16_t offset_middle;
  uint32_t offset_high;
  uint32_t zero;
} idt[0x100];

extern void x8664_oninterrupt();
static char stack_for_interrupts[4096] __attribute__((aligned(4096)));
static char stack_for_crash_exceptions[4096] __attribute__((aligned(4096)));

static load _loadidt = {
  sizeof(idt), 
  (uint64_t)idt
};

void interrupt_set_vector(uint8_t vector, void (*function)(), IST ist) {
  auto& entry = idt[vector];
  uintptr_t func = (uintptr_t)function;
  entry.offset_low = func & 0xFFFF;
  entry.offset_middle = (func >> 16) & 0xFFFF;
  entry.offset_high = (func >> 32);
  entry.ist = (uint8_t)ist;
  entry.selector = 0x08;
  entry.type_attr = 0x8E;
  entry.zero = 0;
}

static void disable_pic() {
  outb(0x20, 0x11);
  outb(0xa0, 0x11);
  outb(0x21, 0x20);
  outb(0xa1, 0x28);
  outb(0x21, 4);
  outb(0xa1, 2);
  outb(0x21, 1);
  outb(0xa1, 1);
  outb(0x21, 0x00);
  outb(0xa1, 0xFF);
}

void interrupt_init() {
  disable_pic();
  
  debug("stack for ist = {}\n", (void*)stack_for_interrupts);
  debug("tss = {}\n", &_tss);
  tss_set_ist(IST::Interrupt, stack_for_interrupts + sizeof(stack_for_interrupts));
  tss_set_ist(IST::Unhandled, stack_for_crash_exceptions + sizeof(stack_for_crash_exceptions));

  for (size_t n = 0; n < 256; n++) {
    switch(n) {
      case 8:
      case 10:
      case 11:
      case 12:
      case 13:
      case 14:
      case 17:
      case 30:
        interrupt_set_vector(n, unhandled_interrupt, IST::Unhandled);
        break;
      default:
        interrupt_set_vector(n, unhandled_interrupt_noec, IST::Unhandled);
        break;
    }
  }

  uint64_t p = (uint64_t)&_tss;
  uint32_t size = sizeof(_tss);
  gdt_[4].access = 0x89;
  gdt_[4].limit_flags = ((size & 0xF0000) >> 16);
  gdt_[4].limit = (size & 0xFFFF);
  gdt_[4].base1 = p & 0xFFFF;
  gdt_[4].base2 = (p >> 16) & 0xFF;
  gdt_[4].base3 = (p >> 24) & 0xFF;
  gdt_[5].limit = (p >> 32) & 0xFFFF;
  gdt_[5].base1 = (p >> 48) & 0xFFFF;
  gdt_[5].base2 = 0;
  gdt_[5].limit_flags = 0;
  gdt_[5].access = 0;
  gdt_[5].base3 = 0;

  asm volatile ("lidt (%%rax)" :: "a"(&_loadidt));
  asm volatile ("lgdt (%%rax)" :: "a"(&_loadgdt));
  asm volatile ("ltr %%ax\n" :: "a"(0x20));
//  Apic::init();
}

void platform_eoi() {
  outb(0xA0, 0x20);
  outb(0x20, 0x20);
}

void platform_enable_interrupts() {
  platform_eoi();
  asm volatile ("sti");
}

void platform_disable_interrupts() {
  asm volatile ("cli");
}

#else

void interrupt_init() {

}

#endif


