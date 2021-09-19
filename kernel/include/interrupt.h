#pragma once

#include <cstdint>
#include <functional>

void interrupt_init(uintptr_t mmio_base = 0);
void interrupt_register(uint32_t vectorId, s2::function<void()> handler);
uint32_t get_empty_interrupt_vector();
void interrupt_check(int vector);
void interrupt_check();
void plat_endofinterrupt();
void halt_for_interrupts();
void platform_enable_interrupts();
void platform_disable_interrupts();


