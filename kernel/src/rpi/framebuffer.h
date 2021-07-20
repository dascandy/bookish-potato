#pragma once

#include <cstdint>
#include <cstddef>
#include <span>
#include <string>
#include "Screen.h"

class RpiFramebuffer {
public:
  struct RpiScreen : public Screen {
    RpiScreen(RpiFramebuffer& dev, s2::span<const uint8_t> edid);
    bool SetActiveResolution(const Resolution& res, size_t bufferCount) override;
    s2::future<void> QueueBuffer(void*) override;
    void* GetFreeBuffer() override;
    RpiFramebuffer& fb;
    Resolution currentResolution;
    uint8_t displayBufferId, queuedBufferId, bufferCount;
    uint16_t xres, yres;
    uint32_t pitch;
    uintptr_t buffer_base;
    size_t buffer_size;
//    void SetMousePointer(uint32_t w, uint32_t h, const uint32_t* pixels, uint32_t hx, uint32_t hy); 
  };
  RpiFramebuffer();
  static RpiFramebuffer* Create();
  size_t getScreenCount() { return 1; }
  Screen* getScreen(size_t n) { return screen; }
private:
  s2::span<uint8_t> ReadEdid(uint8_t* buffer);
  RpiScreen* screen;
};


