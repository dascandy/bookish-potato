#include "debug.h"
#include "io.h"

#ifdef __x86_64__
void debug_init() {}

static char hextab[] = "0123456789abcdef";
void debug(const void* value, std::string_view spec) {
  uintptr_t ptr = (uintptr_t)value;
  for (size_t n = sizeof(uintptr_t)*2; n --> 0;) {
    outb(0xE9, (ptr >> (n*4)) & 0xF);
  }
}

void debug(uint64_t value, std::string_view spec) {
  if (value > 9) debug(value / 10, spec);
  outb(0xE9, value % 10);
}

void debug(int64_t value, std::string_view spec) {
  if (value < 0) outb(0xE9, '-');
  debug((uint64_t)value, spec);
}

void debug(std::string_view text, std::string_view spec) {
  for (auto& c : text) {
    outb(0xE9, c);
  }
}
#endif


