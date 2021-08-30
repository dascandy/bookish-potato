#include "interrupt.h"
#include <cstddef>
#include <flatmap>
#include "debug.h"
#include <cassert>

s2::flatmap<uint32_t, s2::vector<s2::function<void()>>> handlers;

void interrupt_check(int vector) {
  //debug("[INT] Got interrupt {}\n", vector);
  auto it = handlers.find(vector);
  if (it == handlers.end()) {
    debug("[INT] Unknown interrupt vector {} triggered? Nobody is listening.\n", (uint32_t)vector);
  } else {
    for (auto& h : it->second) {
      h();
    }
  }
  plat_endofinterrupt();
  //debug("[INT] Interrupt {} done\n", vector);
}

void interrupt_register(uint32_t vector, s2::function<void()> handler) {
  handlers[vector].push_back(handler);
}

uint32_t get_empty_interrupt_vector() {
  for (size_t n = 255; n >= 32; n--) {
    if (handlers[n].empty()) return n;
  }
  assert(!"TODO: add support for scanning the table again for most-empty interrupt handler\n");
  return 0;
}


