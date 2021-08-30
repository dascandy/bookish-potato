#pragma once

#include <cstdint>
#include <cstddef>

void platform_setup(void* platform_ptr);
void platform_map(void* virt_addr, uint64_t physaddr, MappingUse use);
uint64_t platform_unmap(void* addr);
extern "C" void kernel_secondary_cpu();

void platform_enable_interrupts();
void platform_disable_interrupts();


