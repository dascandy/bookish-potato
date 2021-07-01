#pragma once

#include <cstdint>

typedef void(*interrupt_handler)();
void interrupt_init(uintptr_t mmio_base = 0);
void interrupt_register(interrupt_handler handler);
void interrupt_check();
void halt_for_interrupts();



