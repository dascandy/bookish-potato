#pragma once

#include <cstdint>

enum class IST {
  Interrupt = 1,
  Unhandled = 7,
};
void interrupt_init();
void interrupt_set_vector(uint8_t vector, void (*)(), IST ist);


