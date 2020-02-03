#include "debug.h"
#include "io.h"
#include <cstring>

#ifdef __x86_64__
void debug_init() { }
void debug_field(std::string_view text, std::string_view spec) {
  for (auto& c : text) {
    outb(0xE9, c);
  }
}
#endif
