#include "mailbox.h"
#include "io.h"

#define MBOX_READ         0x3F00B880
#define MBOX_READ_STATUS  0x3F00B898
#define MBOX_WRITE        0x3F00B8A0
#define MBOX_WRITE_STATUS 0x3F00B8B8

#define MBOX_FULL 0x80000000
#define MBOX_EMPTY 0x40000000

void mailbox_send(uint8_t port, uint32_t* ptr) {
  uint32_t value = ((uint32_t)(uintptr_t)(ptr) & 0xFFFFFFF0) | (port);
  while (mmio_read<uint32_t>(MBOX_WRITE_STATUS) & MBOX_FULL) {}
  mmio_write(MBOX_WRITE, value);

  while ((mmio_read<uint32_t>(MBOX_READ_STATUS) & MBOX_EMPTY) || mmio_read<uint32_t>(MBOX_READ) != value) {}
}



