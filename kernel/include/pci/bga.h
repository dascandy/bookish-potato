#pragma once

#include "pci/PciCore.h"
#include <cstddef>
#include "map.h"
#include "Screen.h"

class BgaFramebuffer final : public PciDevice {
public:
  struct BgaScreen : public Screen {
    BgaScreen(volatile PciCfgSpace* conf, void* edid);
    bool SetActiveResolution(const Resolution& res, size_t bufferCount) override;
    s2::future<void> QueueBuffer(void*) override;
    void* GetFreeBuffer() override;
    uint16_t xres, yres;
    uint8_t displayBufferId, queuedBufferId, bufferCount;
    mapping map;
    void* activeScreen;
    uintptr_t bochsregs, qemuregs;
  };
  BgaFramebuffer(uintptr_t confSpacePtr);
  size_t getScreenCount() { return 1; }
  Screen* getScreen(size_t) { return &screen; }
private:
  mapping regs;
  BgaScreen screen;
};


