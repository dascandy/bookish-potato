#include "asa.h"
#include <cstdint>
#include <vector>
#include "debug.h"

struct asa {
  uintptr_t start;
  size_t length;
};
static auto& asas() {
  static std::vector<asa> asas;
  return asas;
}

void asa_init() {
  // after the initial 16M, spanning the remainder of space
  asa_free(0x0000'0000'0100'0000ULL, 0x7fff'ffff'ff00'0000ULL);
};

uintptr_t asa_alloc(size_t size) {
  if (size & 0xFFF) { size += (0x1000 - (size & 0xFFF)); }

  for (auto& a : asas()) {
    if (a.length >= size) {
      uintptr_t rv = a.start;
      a.start += size;
      a.length -= size;
      return rv;
    }
  }
  debug("PANIC: out of address space!\n");
  return 0;
}

void asa_free(uintptr_t ptr, size_t size) {
  asas().push_back(asa{ptr, size});
}

