#pragma once

#include <cstddef>
#include <cstdint>

void crash_hexdump(size_t x, size_t y, const uint8_t* num, size_t length, uint8_t attribute);
void crash_hexdump_region(size_t y, const uint8_t* num, size_t length, uint8_t attribute);
void crash_text(size_t x, size_t y, const char* text, uint8_t attributes);
void unhandled_interrupt();
void unhandled_interrupt_noec();

