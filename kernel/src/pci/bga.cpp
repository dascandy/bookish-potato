#include "pci/bga.h"
#include "io.h"
#include "pci/PciCore.h"
#include "map.h"
#include "debug.h"

enum BgaReg {
  Id,
  XRes,
  YRes,
  Bpp,
  Enable,
  Bank,
  VirtWidth,
  VirtHeight,
  XOffset,
  YOffset
};

BgaFramebuffer::BgaFramebuffer(uintptr_t confSpaceAddr) 
: PciDevice(confSpaceAddr)
, regs(conf, PciBars::Bar2)
, screen(conf, regs.get())
{
  uintptr_t p = (uintptr_t)regs.get();
}

//    uint32_t *buffer;
//    size_t xres, yres, displayBufferId;

BgaFramebuffer::BgaScreen::BgaScreen(volatile PciCfgSpace* conf, void* edid) 
: Screen(s2::span<const uint8_t>((const uint8_t*)edid, 256))
, map(conf, PciBars::Bar0, DeviceMemory)
, bochsregs((uintptr_t)edid + 0x500)
, qemuregs((uintptr_t)edid + 0x400)
{
  debug("[BGA] Found Bochs graphics adapter model {x}\n", mmio_read<uint16_t>(bochsregs));
  Register();
}

bool BgaFramebuffer::BgaScreen::SetActiveResolution(const Resolution& res, size_t bufferCount) {
  // TODO: any checking on result values
  this->bufferCount = bufferCount;
  xres = res.width;
  yres = res.height;
  mmio_write<uint16_t>(bochsregs + 2*Enable, 0);
  mmio_write<uint16_t>(bochsregs + 2*XRes, xres);
  mmio_write<uint16_t>(bochsregs + 2*YRes, yres);
  mmio_write<uint16_t>(bochsregs + 2*Bpp, 32);
  mmio_write<uint16_t>(bochsregs + 2*VirtWidth, xres);
  mmio_write<uint16_t>(bochsregs + 2*VirtHeight, bufferCount * yres);
  mmio_write<uint16_t>(bochsregs + 2*XOffset, 0);
  mmio_write<uint16_t>(bochsregs + 2*YOffset, 0);
  mmio_write<uint16_t>(bochsregs + 2*Enable, 0xE1);
  displayBufferId = 0;
  queuedBufferId = 3;
  currentResolution = res;
  return true;
}

s2::future<void> BgaFramebuffer::BgaScreen::QueueBuffer(void* ptr) {
    uint8_t displayBufferId, queuedBufferId, bufferCount;
  switch(bufferCount) {
  case 1:
    return {};
  case 3:
    // Have no way to detect vsync on bochs graphics device, just use as double-buffered
  case 2:
    if (ptr == map.get()) {
      displayBufferId = 1;
      mmio_write<uint16_t>(bochsregs + 2*YOffset, 0);
    } else {
      displayBufferId = 0;
      mmio_write<uint16_t>(bochsregs + 2*YOffset, yres);
    }
    break;
  }
  return {};
}

void* BgaFramebuffer::BgaScreen::GetFreeBuffer() {
  uint8_t bufferToReturn;
  switch(bufferCount) {
  default:
  case 1: 
    bufferToReturn = 0;
    break;
  case 2:
    bufferToReturn = 1 - displayBufferId;
    break;
  case 3:
    if (queuedBufferId == 3) { // No queued buffer, just pick a non-visible one
      bufferToReturn = (displayBufferId == 0 ? 1 : 0);
    } else {  // A buffer is queued, a buffer is displayed, the sum of all buffer IDs is always 3 - find the one that remains.
      bufferToReturn = 3 - (displayBufferId + queuedBufferId);
    }
    break;
  }
  return (void*)((uintptr_t)map.get() + bufferToReturn * (xres * yres * 4));
}


