#include "debug.h"
#include "mailbox.h"
#include "io.h"

#ifndef __x86_64__
#define UART0_DR        0x201000
#define UART0_FR        0x201018
#define UART0_IBRD      0x201024
#define UART0_FBRD      0x201028
#define UART0_LCRH      0x20102C
#define UART0_CR        0x201030
#define UART0_IMSC      0x201038
#define UART0_ICR       0x201044
#define GPFSEL1         0x200004
#define GPPUD           0x200094
#define GPPUDCLK0       0x200098

static uintptr_t mmio_base = 0x3f000000;

void debug_init(uintptr_t mmio_base) {
    ::mmio_base = mmio_base;
    // Disable UART (if on)
    mmio_write(mmio_base + UART0_CR, 0x00000000);

    // Set UART clock
    alignas(16) static uint32_t enable_uart[] = { 36, 0, 0x38002, 12, 8, 2, 4000000, 0, 0 };
    mailbox_send(8, enable_uart);

    // Map gpio pins to UART
    mmio_write(mmio_base + GPFSEL1, (mmio_read<uint32_t>(mmio_base + GPFSEL1) & 0xFFFC0FFF) | 0x24000);

    // Enable pins
    mmio_write(mmio_base + GPPUD, 0x00000000);
    delay(150);
    mmio_write(mmio_base + GPPUDCLK0, (1 << 14) | (1 << 15));
    delay(150);
    mmio_write(mmio_base + GPPUDCLK0, 0x00000000);

    mmio_write(mmio_base + UART0_ICR, 0x7FF);
    mmio_write(mmio_base + UART0_IBRD, 2);
    mmio_write(mmio_base + UART0_FBRD, 0xB);
    mmio_write(mmio_base + UART0_LCRH, 0x60);
    mmio_write(mmio_base + UART0_CR, 0x301);
}

void debug_char(char c) {
  while ( mmio_read<uint32_t>(mmio_base + UART0_FR) & (1 << 5) ) { asm volatile ("nop"); }
  mmio_write(mmio_base + UART0_DR, c);
}

void debug_field(s2::string_view text, s2::string_view spec) {
  for (auto c : text) debug_char(c);
}

#endif


