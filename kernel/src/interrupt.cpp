#include "interrupt.h"
#include <cstddef>
#include <flatmap>
#include "debug.h"

s2::flatmap<uint32_t, s2::vector<s2::function<void()>>> handlers;

void interrupt_check(int vector) {
  auto it = handlers.find(vector);
  if (it == handlers.end()) {
    debug("[INT] Unknown interrupt vector {} triggered? Nobody is listening.\n", (uint32_t)vector);
  } else {
    for (auto& h : it->second) {
      h();
    }
  }
}

void interrupt_register(uint32_t vector, s2::function<void()> handler) {
  handlers[vector].push_back(handler);
}


