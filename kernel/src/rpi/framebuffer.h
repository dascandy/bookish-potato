#pragma once

#include <cstdint>
#include <cstddef>
#include <span>
#include <string>

struct Monitor {
  uint32_t width_mm, height_mm;
  uint32_t width, height;
  s2::string serial;
  s2::string name;
  s2::string brand;

  static void FromEdid(Monitor& m, s2::span<uint8_t> edid);
};

class RpiFramebuffer {
private:
  uint32_t width, height, pitch;
  RpiFramebuffer();
public:
  Monitor m;
  static RpiFramebuffer* Create();
  void SetMousePointer(uint32_t w, uint32_t h, const uint32_t* pixels, uint32_t hx, uint32_t hy); 
  s2::span<uint8_t> ReadEdid(uint8_t* buffer);
};


