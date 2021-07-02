#pragma once

#include <cstdint>
#include <cstddef>

enum class MailboxProperty : uint32_t {
  VideocoreVersion = 1,
  HardwareBoardModel = 0x10001,
  HardwareBoardRevision = 0x10002,
  HardwareBoardMacAddress = 0x10003,
  HardwareBoardSerial = 0x10004,
  HardwareArmMemory = 0x10005,
  HardwareVcMemory = 0x10006,
  CommandLine = 0x50001,

  GetTemperature = 0x30006,

  GetEDIDBlock = 0x30020,
  GpuAllocateBuffer = 0x40001,
  GpuSetDisplaySize = 0x48003,
  GpuSetVirtualSize = 0x48004,
  GpuSetDepth = 0x48005,
  GpuSetPixelOrder = 0x48006,
  GpuGetPitch = 0x40008,
  GpuSetVirtualOffset = 0x48009,
  GpuSetCursorInfo = 0x8010,
  GpuSetCursorState = 0x8011,

};

void mailbox_init(uintptr_t mmio_base);
void mailbox_send(uint8_t port, uint32_t* ptr);
size_t property_read(MailboxProperty property, uint8_t* buffer, size_t size);

