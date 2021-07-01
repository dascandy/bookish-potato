#pragma once

#include <span>
#include <cstdint>
#include <string>
#include <vector>
#include "future.h"

enum class PixelFormat {
  Unknown,
  Indexed256,
  Gray1,
  Gray8,
  Alpha1,
  Alpha8,
  RGB_565,
  RGB_1555,
  RGBA_8888,
  RGBX_8888,
  RGB_101010,
  RGBA_1010102,
  RGB_161616,
  RGBA_16161616,
  RGB_32f,
  RGBA_32f,
};

struct Screen {
  struct Resolution {
    uint16_t width, height;
    PixelFormat pf;
  };

protected:
  Screen(s2::span<const uint8_t> edid);
  Screen(uint16_t xres, uint16_t yres); // for VM screens, or screens without EDID data
  void Register();
  ~Screen();
public:
  uint16_t widthMm, heightMm, dotsPerInch;
  s2::string connection, manufacturer, serialno, name;
  uint8_t displayBpp;
  std::vector<Resolution> supportedResolutions;
  Resolution currentResolution;

  // Up to three buffers are supported:
  // 1: There is one buffer, call GetFreeBuffer to get its address. No queueing needed; tearing & edit artifacts will show. QueueBuffer is a no-op.
  // 2: There are two buffers. Call GetFreeBuffer to get a free one, update it and queue it with QueueBuffer. This call then blocks until the buffer is displayed. Wait until it's displayed before asking for a new one.
  // 3: There are three buffers. Call GetFreeBuffer to get a free buffer, update it and queue it with QueueBuffer. This call does not block. You can request a new free buffer immediately; this is either the third free buffer (if requested before the frame was used) or the last displayed frame (if there was a buffer transition).
  // If this function returns false, the modeset did not work.
  virtual bool SetActiveResolution(const Resolution& res, size_t bufferCount) = 0;
  // Queue this buffer for display and return a future that indicates when a new free buffer should be requested for drawing an update. See @SetActiveResolution for rationale.
  virtual future<void> QueueBuffer(void*) = 0;
  // Request the address of the currently free buffer.
  virtual void* GetFreeBuffer() = 0;
};

