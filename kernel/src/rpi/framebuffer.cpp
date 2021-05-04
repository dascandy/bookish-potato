#include "framebuffer.h"
#include "mailbox.h"
#include <cstring>
#include "debug.h"
#include <span>
#include <string>

s2::string translate(s2::string brand) {
  if (brand == "IVM") return "Iiyama";
  return "Unknown";
}


void Monitor::FromEdid(Monitor& m, s2::span<uint8_t> edid) {
  // TODO: validate
  char brand[4] = { char('@' + ((edid[8] >> 2) & 0x1F)), char('@' + (((edid[8] << 3) | (edid[9] >> 5)) & 0x1F)), char('@' + ((edid[9]) & 0x1F)), 0 };
  m.brand = translate(brand);
  m.width_mm = edid[66] + ((edid[68] & 0xF0) << 4);
  m.height_mm = edid[67] + ((edid[68] & 0xF) << 8);
  m.width = edid[56] + ((edid[58] & 0xF0) << 4);
  m.height = edid[59] + ((edid[61] & 0xF0) << 4);
  for (size_t n = 0; n < 4; n++) {
    if (edid[54 + 18 * n] == 0 && 
        edid[55 + 18 * n] == 0 &&
        edid[56 + 18 * n] == 0) {
      switch(edid[57 + 18 * n]) {
      case 0xFF:
      {
        uint8_t* b = edid.data() + 58 + 18*n;
        uint8_t* e = b;
        while (e - b != 13 && *e != 10) e++;
        // TODO: transcode from CP437
        m.serial = s2::string_view((const char*)b, (const char*)e);
      }
        break;
      case 0xFC:
      {
        uint8_t* b = edid.data() + 58 + 18*n;
        uint8_t* e = b;
        while (e - b != 13 && *e != 10) e++;
        // TODO: transcode from CP437
        m.name = s2::string_view((const char*)b, (const char*)e);
      }
        break;
      }
    }
  }
}

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
  Monitor::FromEdid(m, s);

  alignas(16) uint32_t gpu_request[64] = { 
    148, 0, 
    (uint32_t)MailboxProperty::GpuGetPitch, 4, 0, 0,
    (uint32_t)MailboxProperty::GpuSetDisplaySize, 8, 0, m.width, m.height,
    (uint32_t)MailboxProperty::GpuSetVirtualSize, 8, 0, m.width, m.height*2,
    (uint32_t)MailboxProperty::GpuSetDepth, 4, 0, 32,
    (uint32_t)MailboxProperty::GpuSetPixelOrder, 4, 0, 0,
    (uint32_t)MailboxProperty::GpuSetVirtualOffset, 8, 0, 0, 0,
    (uint32_t)MailboxProperty::GpuSetCursorState, 16, 0, 1, m.width/2, m.height/2, 0,
    0
  };
  mailbox_send(8, gpu_request);
  pitch = gpu_request[5];
}

RpiFramebuffer* RpiFramebuffer::Create() {
  static RpiFramebuffer r;
  return &r;
}

void RpiFramebuffer::SetMousePointer(uint32_t w, uint32_t h, const uint32_t* pixels, uint32_t hx, uint32_t hy) {
  alignas(16) uint32_t gpu_request[64] = { 
    148, 0, 
    (uint32_t)MailboxProperty::GpuSetCursorInfo, 24, 0, w, h, 0, (uint32_t)(uintptr_t)pixels, hx, hy,
    0
  };
  mailbox_send(8, gpu_request);
}


