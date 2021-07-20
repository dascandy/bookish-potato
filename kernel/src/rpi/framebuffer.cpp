#include "framebuffer.h"
#include "mailbox.h"
#include <cstring>
#include "debug.h"
#include <span>
#include <string>

s2::span<uint8_t> RpiFramebuffer::ReadEdid(uint8_t* buffer) {
  uint32_t count = 0;
  do {
    alignas(16) uint32_t gpu_request[40] = {
      160, 0,
      (uint32_t)MailboxProperty::GetEDIDBlock, 136, 0, count, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
      0
    };
    mailbox_send(8, gpu_request);
    memcpy(buffer + count * 128, (uint8_t*)gpu_request + 28, 128);
    if (gpu_request[4] != 0x80000088) break;
    count++;
  } while (count < 1);

  return s2::span<uint8_t>(buffer, count << 7);
}

RpiFramebuffer::RpiFramebuffer() {
  uint8_t buffer[1024];
  auto s = ReadEdid(buffer);
  screen = new RpiScreen(*this, s);
}

RpiFramebuffer* RpiFramebuffer::Create() {
  static RpiFramebuffer r;
  return &r;
}

RpiFramebuffer::RpiScreen::RpiScreen(RpiFramebuffer& fb, s2::span<const uint8_t> edid) 
: Screen(edid)
, fb(fb)
{
  debug("[RPI] Found Raspberry Pi framebuffer\n");
  Register();
}

bool RpiFramebuffer::RpiScreen::SetActiveResolution(const Resolution& res, size_t bufferCount) {
  alignas(16) uint32_t gpu_request[] = { 
    12 + 16 + 20 + 20 + 20 + 16 + 16 + 20, 0, 
    (uint32_t)MailboxProperty::GpuGetPitch, 4, 0, 0,
    (uint32_t)MailboxProperty::GpuAllocateBuffer, 8, 0, 4096, 0,
    (uint32_t)MailboxProperty::GpuSetDisplaySize, 8, 0, res.width, res.height,
    (uint32_t)MailboxProperty::GpuSetVirtualSize, 8, 0, res.width, res.height*(uint32_t)bufferCount,
    (uint32_t)MailboxProperty::GpuSetDepth, 4, 0, 32,
    (uint32_t)MailboxProperty::GpuSetPixelOrder, 4, 0, 0,
    (uint32_t)MailboxProperty::GpuSetVirtualOffset, 8, 0, 0, 0,
    0
  };
  mailbox_send(8, gpu_request);
  this->bufferCount = bufferCount;
  xres = res.width;
  yres = res.height;
  pitch = gpu_request[5];
  buffer_base = gpu_request[9];
  buffer_size = gpu_request[10];
  debug("[RPI] Set resolution to {}x{}, buffer allocated at 0x{x} size 0x{x}\n", res.width, res.height, buffer_base, buffer_size);
  currentResolution = res;
  return true;
}

s2::future<void> RpiFramebuffer::RpiScreen::QueueBuffer(void* ptr) {
  switch(bufferCount) {
  case 1:
    return {};
  case 3:
    // Have no way to detect vsync on rpi graphics device, just use as double-buffered
  case 2:
    if ((uintptr_t)ptr == buffer_base) {
      displayBufferId = 1;
    } else {
      displayBufferId = 0;
    }
    alignas(16) uint32_t gpu_request[64] = { 
      32, 0, 
      (uint32_t)MailboxProperty::GpuSetVirtualOffset, 8, 0, 0, (uint32_t)(displayBufferId * yres),
      0
    };
    mailbox_send(8, gpu_request);
    break;
  }
  return {};
}

void* RpiFramebuffer::RpiScreen::GetFreeBuffer() {
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
  return (void*)(buffer_base + bufferToReturn * (xres * yres * 4));
}


