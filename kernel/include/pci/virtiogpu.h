#pragma once

#include "pci/PciCore.h"
#include <cstddef>
#include "map.h"
#include "Screen.h"
#include <span>

class VirtioGpu : public PciDevice {
public:
  struct VirtioGpuScreen : public Screen {
    VirtioGpuScreen(s2::span<const uint8_t> edid);
    bool SetActiveResolution(const Resolution& res, size_t bufferCount) override;
    s2::future<void> QueueBuffer(void*) override;
    void* GetFreeBuffer() override;
  };
  VirtioGpu(uintptr_t confSpaceAddr);
  size_t getScreenCount() { return 1; }
  Screen* getScreen(size_t n) { return &screen; }
private:
  VirtioGpuScreen screen;
};


