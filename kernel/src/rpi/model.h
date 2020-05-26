#pragma once

#include <cstdint>

struct rpi_entry {
  uint32_t modelno;
  const char* modelname;
  const char* revision;
  uint16_t memory;
  const char* manufacturer;
  uintptr_t mmio_base;
};

rpi_entry getModel();

