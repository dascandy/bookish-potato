#include "interrupt.h"
#include <cstddef>

void (*handlers[80])();
interrupt_handler int_handlers[80];
int last_handler = 0;

void interrupt_check() {
  for (size_t n = 0; n < last_handler; n++) {
    int_handlers[n]();
  }
}

void interrupt_register(interrupt_handler handler) {
  int_handlers[last_handler++] = handler;
}


