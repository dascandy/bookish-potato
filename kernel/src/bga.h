#pragma once

#include "pci.h"
#include <cstddef>
#include "map.h"
#include "Screen.h"

#ifdef __x86_64__

class BgaFramebuffer {
public:
  struct BgaScreen : public Screen {
    BgaScreen(pcidevice dev, void* edid);
    bool SetActiveResolution(const Resolution& res, size_t bufferCount) override;
    s2::future<void> QueueBuffer(void*) override;
    void* GetFreeBuffer() override;
    uint16_t xres, yres;
    uint8_t displayBufferId, queuedBufferId, bufferCount;
    mapping map;
    void* activeScreen;
    uintptr_t bochsregs, qemuregs;
  };
  BgaFramebuffer(pcidevice dev);
  size_t getScreenCount() { return 1; }
  Screen* getScreen(size_t n) { return &screen; }
private:
  mapping regs;
  BgaScreen screen;
};

#endif


