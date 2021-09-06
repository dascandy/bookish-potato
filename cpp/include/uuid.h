#pragma once

#include <cstdint>

using uuid_t = uint128_t;
constexpr inline uuid_t uuid(uint32_t a, uint16_t b, uint16_t c, uint16_t d, uint64_t e) {
  d = ((d & 0xFF) << 8) | ((d & 0xFF00) >> 8);
  uint64_t altE = 0;
  for (size_t n = 0; n < 6; n++) {
    altE = (altE << 8) | (e & 0xFF);
    e >>= 8;
  }
  return ((uint128_t)a) | ((uint128_t)b << 32) | ((uint128_t)c << 48) | ((uint128_t)d << 64) | ((uint128_t)altE << 80);
}


