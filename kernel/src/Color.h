#pragma once

#include <cstdint>

class Color
{
public:
  uint8_t r, g, b;
  Color(uint8_t r, uint8_t g, uint8_t b)
  : r(r)
  , g(g)
  , b(b)
  {
  }
  uint32_t toInt() {
    return (r << 16) | (g << 8) | (b);
  }
};


