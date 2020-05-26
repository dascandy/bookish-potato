#pragma once

#include <cstdint>
#include <cstddef>

enum class MailboxProperty {
  VideocoreVersion = 1,
  HardwareBoardModel = 0x10001,
  HardwareBoardRevision = 0x10002,
  HardwareBoardMacAddress = 0x10003,
  HardwareBoardSerial = 0x10004,
  HardwareArmMemory = 0x10005,
  HardwareVcMemory = 0x10006,
  CommandLine = 0x50001,
};

void mailbox_init(uintptr_t mmio_base);
void mailbox_send(uint8_t port, uint32_t* ptr);
size_t property_read(MailboxProperty property, uint8_t* buffer, size_t size);

