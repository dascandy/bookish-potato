#pragma once

#include <cstdint>

[[gnu::packed]] struct UsbStorageCbw {
  uint32_t signature;
  uint32_t tag;
  uint32_t length;
  uint8_t direction;
  uint8_t lun;
  uint8_t cmdLength;
  uint8_t command[32];
};

[[gnu::packed]] struct UsbStorageCsw {
  uint32_t signature;
  uint32_t tag;
  uint32_t residue;
  uint8_t status;
};



