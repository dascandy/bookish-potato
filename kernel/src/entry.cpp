#include <cstddef>
#include <cstdint>
#include "bookish_potato_version.h"
#include "interrupt.h"
#include "crash.h"
#include "acpi.h"

[[noreturn]] extern "C" void kernel_secondary_cpu() {
  while (1) {}
}

[[noreturn]] extern "C" void kernel_entry() {
  interrupt_init();
  acpi_init();
  while(1) {}
}


