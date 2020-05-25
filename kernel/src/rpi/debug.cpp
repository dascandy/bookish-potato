#include "debug.h"
#include "mailbox.h"
#include "io.h"

#ifndef __x86_64__
#define UART0_DR        0x3F201000
#define UART0_FR        0x3F201018
#define UART0_IBRD      0x3F201024
#define UART0_FBRD      0x3F201028
#define UART0_LCRH      0x3F20102C
#define UART0_CR        0x3F201030
#define UART0_IMSC      0x3F201038
#define UART0_ICR       0x3F201044
#define GPFSEL1         0x3F200004
#define GPPUD           0x3F200094
#define GPPUDCLK0       0x3F200098

void debug_init() {
    // Disable UART (if on)
    mmio_write(UART0_CR, 0x00000000);

    // Set UART clock
    alignas(16) static uint32_t enable_uart[] = { 36, 0, 0x38002, 12, 8, 2, 4000000, 0, 0 };
    mailbox_send(8, enable_uart);

    // Map gpio pins to UART
    mmio_write(GPFSEL1, (mmio_read<uint32_t>(GPFSEL1) & 0xFFFC0FFF) | 0x24000);

    // Enable pins
    mmio_write(GPPUD, 0x00000000);
    delay(150);
    mmio_write(GPPUDCLK0, (1 << 14) | (1 << 15));
    delay(150);
    mmio_write(GPPUDCLK0, 0x00000000);

    mmio_write(UART0_ICR, 0x7FF);
    mmio_write(UART0_IBRD, 2);
    mmio_write(UART0_FBRD, 0xB);
    mmio_write(UART0_LCRH, 0x60);
    mmio_write(UART0_CR, 0x301);
}

void debug_char(char c) {
  while ( mmio_read<uint32_t>(UART0_FR) & (1 << 5) ) { asm volatile ("nop"); }
  mmio_write(UART0_DR, c);
}

void debug_field(std::string_view text, std::string_view spec) {
  for (auto c : text) debug_char(c);
}

#endif


