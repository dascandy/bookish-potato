#include "mailbox.h"

struct mailbox_regs {
  uint32_t read;
  uint32_t pad[3];
  uint32_t poll;
  uint32_t sender;
  uint32_t status;
  uint32_t config;
  uint32_t write;
};

static volatile mailbox_regs *regs = (mailbox_regs *)0x3F00B880;

void mailbox_send(uint8_t port, uint32_t* ptr) {
  while (regs->status & 0x80000000U) {}
  regs->write = uint32_t(uintptr_t(ptr)) | port;

  while ((regs->status & 0x40000000) == 0) {
    uint32_t val = regs->read;
    if ((uint8_t)(val & 0xF) == port) return;
  }
}

void mailbox_send(uint8_t port, uint32_t value) {
  while (regs->status & 0x80000000U) {}
  regs->write = value | port;

  while ((regs->status & 0x40000000) == 0) {
    uint32_t val = regs->read;
    if ((uint8_t)(val & 0xF) == port) return;
  }
}




