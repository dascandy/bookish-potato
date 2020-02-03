#include "debug.h"
#include "io.h"
#include <cstring>

static char hextab[] = "0123456789abcdefghijklmnopqrstuvwxyz";

void debug_field(uint64_t value, std::string_view spec) {
  size_t pad = 0;
  int base = 10;
  auto b = spec.begin();
  while (b != spec.end() && *b >= '0' && *b <= '9') { pad = (pad * 10) + (*b - '0'); b++; }
  if (b != spec.end() && *b == 'x') base = 16;
  if (b != spec.end() && *b == 'b') base = 2;
  if (pad == 0) pad = 1;

  char buffer[64];
  char* bp = buffer+63;
  while (value) {
    *bp-- = hextab[value % base];
    value /= base;
  }
  while (bp > buffer + 63 - pad) *bp-- = '0';
  bp++;
  debug_field(std::string_view(bp, buffer+64), "");
}

void debug_field(void* value, std::string_view spec) {
  debug_field((uintptr_t)value, "16x");
}

void debug_field(int64_t value, std::string_view spec) {
  if (value < 0) debug_field("-", "");
  debug_field((uint64_t)value, spec);
}



