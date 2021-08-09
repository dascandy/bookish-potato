#include <cstddef>
#include <cstdint>
#include <experimental/coroutine>
#include "bookish_potato_version.h"
#include "interrupt.h"
#include "debug.h"
#include "x86/acpi.h"
#include "x86/apic.h"
#include "pci.h"
#include "future.h"
#include "freepage.h"
#include "asa.h"
#include "event.h"
#include "timer.h"
#include "io.h"
#include "usb/UsbDrivers.h"

#ifdef __x86_64__
#include "x86/cpu.h"
struct mb1 {
  uint32_t flags;
  uint32_t memlower;
  uint32_t memhigher;
  uint32_t bootdevice;
  uint32_t cmdline;
  uint32_t mods_count;
  uint32_t mods_addr;
  uint32_t sym_num;
  uint32_t sym_size;
  uint32_t sym_addr;
  uint32_t sym_shndx;
  uint32_t mmap_length;
  uint32_t mmap_addr;
  uint32_t drives_length;
  uint32_t drives_addr;
  uint32_t config_table;
  uint32_t boot_loader_name;
  uint32_t apm_table;
  // legacy graphics stuff below, don't care
};

struct mb1_memory {
  uint64_t base_addr;
  uint64_t length;
  uint32_t type;
};

void mb1_init(mb1* data) {
  if (data->flags & 0x40) {
    debug("[PLAT] MB1 has memory map\n");
    size_t stride = *(uint32_t*)((uintptr_t)data->mmap_addr );
    if (stride < 20) stride = 24;
    size_t count = data->mmap_length / stride;
    for(size_t n = 0; n < count; n++) {
      mb1_memory* m = (mb1_memory*)((uintptr_t)data->mmap_addr + 4 + n * (stride + 4));
      if (m->type == 1) {
        uintptr_t start = m->base_addr;
        size_t length = m->length;
        if ((start & 0xFFF) != 0) {
          size_t toskip = 0x1000 - (start & 0xFFF);
          start += toskip;
          length -= toskip;
        }
        if (length & 0xFFF) length -= (length & 0xFFF);
        debug("[PLAT] Found {} pages at {x}\n", length / 4096, start);
        freepage_add_region(m->base_addr, m->length);
      }
    }
  }
}

struct mb2_entry {
  uint32_t type;
  uint32_t size;
};

struct mb2 {
  uint32_t total_size;
  uint32_t reserved;
};

void mb2_init(mb2* data) {
  mb2_entry* entry = (mb2_entry*)((char*)data + 8);
  while (entry->type != 0) {
    debug("[PLAT] MB2 {} {}\n", entry->type, entry->size);
    entry = (mb2_entry*)((char*)entry + entry->size);
  }
}

void platform_init(void* platform_data, uint32_t magic) {
  cpu_init();
  Apic::init();
  interrupt_init();
  if (magic == 0x2badb002) {
    mb1_init((mb1*)platform_data);
  } else if (magic == 0x36d76289) {
    mb2_init((mb2*)platform_data);
  } else {
    debug("[PLAT] No MB data found, continuing without. This is broken, fyi.\n");
  }
//  init_pci_drivers();
  init_usb_drivers();
  acpi_init();
//  pci_handle_bridge(0, 0);
}
#else
#include "rpi/property_print.h"
#include "rpi/model.h"
#include "rpi/sd.h"
#include "rpi/framebuffer.h"
#include "rpi/mailbox.h"
#include "gic.h"

void timer_init(uintptr_t base);

void interrupt_check();

void platform_init(void* platform_data, uint32_t magic) {
  rpi_entry model = getModel();
  debug_init(model.mmio_base);
  if (model.mmio_base == 0x3F000000) {
    interrupt_init(model.mmio_base);
  } else {
    gic_init(model.mmio_base);
  }
  timer_init(model.mmio_base);
  debug("[PLAT] Found Raspberry Pi {s} (rev {s} with {}MB RAM, modelno {x} manufactured by {s}\n", model.modelname, model.revision, model.memory, model.modelno, model.manufacturer);

  RpiFramebuffer* fb = RpiFramebuffer::Create();
  debug("[PLAT] Found monitor {s} {s} serial# {s} at resolution {}x{}\n", fb->m.brand, fb->m.name, fb->m.serial, fb->m.width, fb->m.height);

  init_usb_drivers();
//  sd_init(model.mmio_base + 0x00300000);
}

#endif

extern "C" void kernel_secondary_cpu() {
  while (1) {}
}

s2::future<void> f() {
  // Automatic after integrating network and ntp
  uint64_t currentTime = 1591308000000000ULL;
  set_utc_offset(currentTime - get_timer_value());
  debug("[PLAT] set utc offset\n");
  while (true) {
    currentTime += 1000000;
    co_await wait_until(currentTime);
    uint64_t time = (currentTime - 1591308000000000ULL) / 1000000;
    debug("[PLAT] {}:{}:{}     \r", time / 3600, (time / 60) % 60, time % 60);
  }
}

extern "C" void kernel_entry(void* platform_data, uint32_t magic) {
  asa_init();
  platform_init(platform_data, magic);
  set_utc_offset(1591473338000000 - get_timer_value());
  debug("[ENTRY] End of platform init\n");
  platform_enable_interrupts();  
  f();
/*
  while (1) {
    interrupt_check();
    debug("current timer is {}\r", get_timer_value());
  }
  */
  while(1) {}
}


