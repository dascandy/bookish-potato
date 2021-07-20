#pragma once

#include "pci.h"
#include <cstddef>
#include "map.h"
#include "Screen.h"
#include <span>

class VirtioGpu {
public:
  struct VirtioGpuScreen : public Screen {
    VirtioGpuScreen(pcidevice dev, s2::span<const uint8_t> edid);
    bool SetActiveResolution(const Resolution& res, size_t bufferCount) override;
    s2::future<void> QueueBuffer(void*) override;
    void* GetFreeBuffer() override;
  };
  VirtioGpu(pcidevice dev);
  size_t getScreenCount() { return 1; }
  Screen* getScreen(size_t n) { return &screen; }
private:
  VirtioGpuScreen screen;
};


