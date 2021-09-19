#include "apic.h"
#include "interrupt.h"
#include "io.h"
#include "debug.h"
#include "ui.h"

#ifdef __x86_64__
void SetIsaInterrupt(size_t isaInterrupt, s2::function<void()> handler);

uint8_t ps2_mode1_to_hid[257] = {
  0x00, 0x29, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x2d, 0x2e, 0x2a, 0x2b, 
  0x14, 0x1a, 0x08, 0x15, 0x17, 0x1c, 0x18, 0x0c, 0x12, 0x13, 0x2f, 0x30, 0x28, 0xe0, 0x04, 0x16, 
  0x07, 0x09, 0x0a, 0x0b, 0x0d, 0x0e, 0x0f, 0x33, 0x34, 0x35, 0xe1, 0x31, 0x1d, 0x1b, 0x06, 0x19, 
  0x05, 0x11, 0x10, 0x36, 0x37, 0x38, 0xe5, 0x55, 0xe2, 0x2c, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 
  0x3f, 0x40, 0x41, 0x42, 0x43, 0x53, 0x47, 0x5f, 0x60, 0x61, 0x56, 0x5c, 0x5d, 0x5e, 0x57, 0x59, 
  0x5a, 0x5b, 0x62, 0x63, 0x00, 0x00, 0x64, 0x44, 0x45, 0x67, 0x00, 0x00, 0x8c, 0x68, 0x69, 0x6a, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x88, 0x00, 0x00, 0x87, 0x00, 0x00, 0x94, 0x93, 0x92, 0x8a, 0x00, 0x8b, 0x00, 0x89, 0x85, 0x00, 
  // 0xe0 prefixed
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x58, 0xe4, 0x00, 0x00,  
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x54, 0x00, 0x46, 0xe6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0x4a, 0x52, 0x4b, 0x00, 0x50, 0x00, 0x4f, 0x00, 0x4d, 
  0x51, 0x4e, 0x49, 0x4c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe3, 0xe7, 0x65, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  // pause
  0x48
};

static void ps2_write_command(uint8_t command) {
  while (inb(0x64) & 0x2) asm("pause");
  outb(0x64, command);
}

static void ps2_write_data(uint8_t data) {
  while (inb(0x64) & 0x2) asm("pause");
  return outb(0x60, data);
}

static uint8_t ps2_read_data() {
  while ((inb(0x64) & 0x1) == 0) asm("pause");
  return inb(0x60);
}

static struct Ps2Keyboard : HidDevice {
  Ps2Keyboard() {
    Ui::Compositor::Instance().RegisterHidDevice(*this);
  }
  ~Ps2Keyboard() {
    Ui::Compositor::Instance().UnregisterHidDevice(*this);
  }
  void HandleInterrupt() {
    debug("[PS2] Keyboard interrupt\n");
    while ((inb(0x64) & 0x1) == 1) {
      buffer[offset++] = inb(0x60);
      bool sendReport = false;
      bool make = false;
      uint8_t code;
      switch(buffer[0]) {
      case 0xE0:
        if (offset == 2) {
          make = (buffer[1] & 0x80);
          code = ps2_mode1_to_hid[buffer[1] | 0x80];
          sendReport = true;
        }
        break;
      case 0xE1:
        if (offset == 3) {
          make = (buffer[1] & 0x80);
          code = ps2_mode1_to_hid[0x100];
          sendReport = true;
        }
        break;
      default:
        make = (buffer[0] & 0x80);
        code = ps2_mode1_to_hid[buffer[0] & 0x7F];
        sendReport = true;
        break;
      }
      if (sendReport) {
        if (make) {
          report[code] = 1;
        } else if (report.contains(code)) {
          report.erase(report.find(code));
        }
        for (auto& l : listeners) {
          l->HandleReport(*this, report);
        }
        offset = 0;
      }
    }
  }
  Type getType() override {
    return HidDevice::Type::Keyboard;
  }
  s2::flatmap<uint32_t, int16_t> report;
  uint8_t buffer[3];
  uint8_t offset = 0;
}* _keyb;

static struct Ps2Mouse : HidDevice {
  Ps2Mouse() {
    Ui::Compositor::Instance().RegisterHidDevice(*this);
  }
  ~Ps2Mouse() {
    Ui::Compositor::Instance().UnregisterHidDevice(*this);
  }
  void HandleInterrupt() {
    debug("[PS2] Mouse interrupt\n");
    while ((inb(0x64) & 0x1) == 1) {
      packet[offset++] = inb(0x60);
      if (offset == 3) {
        s2::flatmap<uint32_t, int16_t> report;
        if (packet[1] != 0) {
          report[0x10030] = (packet[0] & 0x10) ? packet[1] - 0x100 : packet[1];
        }
        if (packet[2] != 0) {
          // Inverted because USB expects the inverse of PS2.
          report[0x10031] = -((packet[0] & 0x20) ? packet[2] - 0x100 : packet[2]);
        }
        if (packet[0] & 0x1) {
          report[0x90001] = 1;
        }
        if (packet[0] & 0x2) {
          report[0x90002] = 1;
        }
        if (packet[0] & 0x4) {
          report[0x90003] = 1;
        }

        for (auto& l : listeners) {
          l->HandleReport(*this, report);
        }
        offset = 0;
      }
    }
  }
  Type getType() override {
    return HidDevice::Type::Mouse;
  }
  uint8_t packet[3];
  uint8_t offset = 0;
}* _mouse;

void ps2_keyboard_interrupt() {
  _keyb->HandleInterrupt();
}

void ps2_mouse_interrupt() {
  _mouse->HandleInterrupt();
}

void ps2_init() {
  debug("[PS2] Init start\n");
  ps2_write_command(0xad);
  ps2_write_command(0xa7);
  inb(0x60); // Not using ps2_read_data, because we want to just clear it

  ps2_write_command(0x20);
  uint8_t data = ps2_read_data() & 0xFC;
  ps2_write_command(0x60);
  ps2_write_data(data);

  ps2_write_command(0xaa);
  if (ps2_read_data() != 0x55) {
    // no working ps2 controller
    debug("[PS2] Controller not working\n");
    return;
  }

  bool first = false, second = false;
  ps2_write_command(0xab);
  if (ps2_read_data() == 0) first = true;

  ps2_write_command(0xa9);
  if (ps2_read_data() == 0) second = true;

  if (not first and not second) {
    debug("[PS2] No devices found\n");
    return;
  }

  if (first) {
    debug("[PS2] Found first device, enabling\n");
    _keyb = new Ps2Keyboard();
    data |= 1;
    SetIsaInterrupt(1, ps2_keyboard_interrupt);
  }
  if (second) {
    debug("[PS2] Found second device, enabling\n");
    _mouse = new Ps2Mouse();
    data |= 2;
    SetIsaInterrupt(12, ps2_mouse_interrupt);
  }
  ps2_write_command(0x60);
  ps2_write_data(data);

  if (first) {
    ps2_write_command(0xae);

    ps2_write_data(0xf4);
  }

  if (second) {
    ps2_write_command(0xa8);

    ps2_write_command(0xd4);
    ps2_write_data(0xf4);
  }
  debug("[PS2] Init done\n");
}
#endif

