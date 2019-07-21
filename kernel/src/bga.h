#pragma once

#include "pci.h"
#include <cstddef>

class BgaFramebuffer {
public:
  BgaFramebuffer(pcidevice dev);
  void setResolution(size_t x, size_t y, size_t bufferCount);
  size_t getWidth();
  size_t getHeight();
  uint32_t *getBufferLine(size_t lineno);
private:
  size_t xres, yres, bufferId;
  uint32_t *buffer;
};


