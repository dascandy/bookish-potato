#include "mailbox.h"
#include "io.h"
#include <cstring>

#define MBOX_READ         0x00
#define MBOX_READ_STATUS  0x18
#define MBOX_WRITE        0x20
#define MBOX_WRITE_STATUS 0x38

#define MBOX_FULL 0x80000000
#define MBOX_EMPTY 0x40000000

static uintptr_t mbox_base = 0x3F00B880;
void mailbox_init(uintptr_t mmio_base) {
  mbox_base = mmio_base + 0xB880;
}

void mailbox_send(uint8_t port, uint32_t* ptr) {
  uint32_t value = ((uint32_t)(uintptr_t)(ptr) & 0xFFFFFFF0) | (port);
  while (mmio_read<uint32_t>(mbox_base + MBOX_WRITE_STATUS) & MBOX_FULL) {}
  mmio_write(mbox_base + MBOX_WRITE, value);

  while ((mmio_read<uint32_t>(mbox_base + MBOX_READ_STATUS) & MBOX_EMPTY) || mmio_read<uint32_t>(mbox_base + MBOX_READ) != value) {}
}

size_t property_read(MailboxProperty property, uint8_t* buffer, size_t size) {
  alignas(16) uint32_t mbox_buf[] = { 36, 0, (uint32_t)property, (uint32_t)size, 0, 0, 0, 0, 0 };
  mailbox_send(8, mbox_buf);
  memcpy(buffer, (uint8_t*)mbox_buf + 20, size);
  return mbox_buf[4];
}


