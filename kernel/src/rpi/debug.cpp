#include "debug.h"
#include "mailbox.h"

#ifndef __x86_64__
#define UART0_DR        ((volatile uint32_t*)(0x3F2013F0))
#define UART0_FR        ((volatile uint32_t*)(0x3F201018))
#define UART0_IBRD      ((volatile uint32_t*)(0x3F201024))
#define UART0_FBRD      ((volatile uint32_t*)(0x3F201028))
#define UART0_LCRH      ((volatile uint32_t*)(0x3F20102C))
#define UART0_CR        ((volatile uint32_t*)(0x3F201030))
#define UART0_IMSC      ((volatile uint32_t*)(0x3F201038))
#define UART0_ICR       ((volatile uint32_t*)(0x3F201044))
#define GPFSEL1         ((volatile uint32_t*)(0x3F200004))
#define GPPUD           ((volatile uint32_t*)(0x3F200094))
#define GPPUDCLK0       ((volatile uint32_t*)(0x3F200098))

void debug_init() {
    *UART0_CR = 0;
    alignas(16) uint32_t enable_uart[] = { 36, 0, 0x38002, 12, 8, 2, 4000000, 0, 0 };

    mailbox_send(8, enable_uart);

    unsigned int r=*GPFSEL1;
    r&=~((7<<12)|(7<<15));
    r|=(4<<12)|(4<<15);
    *GPFSEL1 = r;
    *GPPUD = 0;
    for (int r = 150; r; r--) { asm volatile("nop"); }
    *GPPUDCLK0 = (1<<14)|(1<<15);
    for (int r = 150; r; r--) { asm volatile("nop"); }
    *GPPUDCLK0 = 0;        // flush GPIO setup

    *UART0_ICR = 0x7FF;
    *UART0_IBRD = 0x02;
    *UART0_FBRD = 0x0B;
    *UART0_LCRH = 0x60;
    *UART0_CR = 0x301;     // enable Tx, Rx, FIFO
}

void debug_field(std::string_view text, std::string_view spec) {
  for (auto& c : text) {
    do{asm volatile("nop");}while(*UART0_FR&0x20);
    *UART0_DR=c;
  }
}

#endif


