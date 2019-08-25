#include <cstddef>
#include <cstdint>
#include "bookish_potato_version.h"
#include "interrupt.h"
#include "debug.h"
#include "acpi.h"
#include "pci.h"
#include "nvme.h"
#include "xhci.h"
#include "freepage.h"

const char* pci_caps[] = {
  "-",
  "pm",
  "agp",
  "vpd",
  "slot",
  "msi",
  "cpci_hs",
  "pci-x",
  "ht",
  "vendor",
  "debug",
  "ccrc",
  "hotplug",
  "subsys_vid",
  "agp-br",
  "inv-0xF",
  "pci-ex",
  "msi-x",
  "inv-0x12",
  "pciadv",
};

PciDevice* pci_handle_bridge(pcidevice dev, uint8_t subbusid);

PciDevice* pci_find_driver(pcidevice dev, uint16_t vendor, uint16_t device, uint32_t devClass) {
  // 0. if it's a bridge, make it a bridge
  uint8_t header = pciread8(dev, 0x0E);
  if ( (header & 0x7F) == 0x01 )
  {
    uint8_t subbusid = pciread8(dev, 0x19);
    return pci_handle_bridge(dev, subbusid);
  }

  // 1. find match for vendor/device


  // 2. find match for device class
  if (devClass == 0x010802) {
    return new NvmeDevice(dev);
  } else if (devClass == 0x0c0330) {
    return new XhciDevice(dev);
  }

  // 3. give up
  return nullptr;
}

void pci_debug_print_device(uint16_t vendor, uint16_t device, uint32_t devClass) {
  debug("found PCI vid={4x}", vendor);
  if (vendor == 0x8086) debug("(intel)");
  else if (vendor == 0x10EC) debug("(realtek)");
  else if (vendor == 0x1234) debug("(qemu)");
  else if (vendor == 0x1B36) debug("(qemu)");
  debug(" did={4x} class={6x} (", device, devClass);

  if (devClass == 0x010180)
    debug("IDE controller device");
  else if (devClass == 0x010601)
    debug("AHCI device");
  else if (devClass == 0x010801)
    debug("NVMHCI device");
  else if (devClass == 0x010802)
    debug("NVMExpress device");
  else if (devClass == 0x020000)
    debug("Ethernet controller device");
  else if (devClass == 0x030000)
    debug("VGA Graphics device");
  else if (devClass == 0x038000)
    debug("non-VGA graphics device");
  else if (devClass == 0x040300)
    debug("audio device");
  else if (devClass == 0x0c0330)
    debug("XHCI device");
  else if (devClass == 0x060000)
    debug("host bridge device");
  else if (devClass == 0x060100)
    debug("ISA bridge device");
  else if (devClass == 0x060400)
    debug("PCI bridge device");
  else if (devClass == 0x068000)
    debug("some weird bridge device");
  else if (devClass == 0x080501)
    debug("SDHCI device");
  else 
    debug("unknown");
  debug(")");
}

PciDevice* pci_handle_bridge(pcidevice dev, uint8_t subbusid) {
  PciBridge* bridge = new PciBridge(dev, subbusid);
  debug("bridge starting\n");
  pci_detect([](pcidevice dev){
    uint32_t vendor_device = pciread32(dev, 0);
    uint16_t device = vendor_device >> 16;
    uint16_t vendor = vendor_device & 0xFFFF;
    uint32_t devClass = (pciread32(dev, 8) & 0xFFFFFF00) >> 8;
    pci_debug_print_device(vendor, device, devClass);
    uint8_t cap = pciread8(dev, 0x34);
    if (cap) {
      debug(" cap(");
      while (cap) {
        uint16_t val = pciread16(dev, cap);
        uint8_t capid = val & 0xFF;
        debug("{} ", std::string_view(pci_caps[capid]));
        cap = val >> 8;
      }
      debug(")");
    }
    debug("\n");
    return pci_find_driver(dev, vendor, device, devClass);
  }, subbusid, bridge);
  debug("bridge ending\n");
  return bridge;
}

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
    debug("MB1 has memory map\n");
    debug("{x} {x}\n", data->mmap_addr, data->mmap_length);
    size_t stride = *(uint32_t*)((uintptr_t)data->mmap_addr );
    debug("{} stride\n", stride);
    if (stride < 20) stride = 24;
    size_t count = data->mmap_length / stride;
    for(size_t n = 0; n < count; n++) {
      mb1_memory* m = (mb1_memory*)((uintptr_t)data->mmap_addr + 4 + n * (stride + 4));
      debug("{x} {x} {}\n", m->base_addr, m->length, m->type);
      if (m->type == 1) {
        uintptr_t start = m->base_addr;
        size_t length = m->length;
        if ((start & 0xFFF) != 0) {
          size_t toskip = 0x1000 - (start & 0xFFF);
          start += toskip;
          length -= toskip;
        }
        if (length & 0xFFF) length -= (length & 0xFFF);
        debug("{x} {x}\n", start, length);
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
    debug("MB2 {} {}\n", entry->type, entry->size);
    entry = (mb2_entry*)((char*)entry + entry->size);
  }
}

void platform_init(void* platform_data, uint32_t magic) {
  if (magic == 0x2badb002) {
    mb1_init((mb1*)platform_data);
  } else if (magic == 0x36d76289) {
    mb2_init((mb2*)platform_data);
  } else {
    debug("No MB data found, continuing without. This is broken, fyi.\n");
  }
  acpi_init();
  pci_handle_bridge(0, 0);
}

extern "C" void kernel_secondary_cpu() {
  while (1) {}
}

extern "C" void kernel_entry(void* platform_data, uint32_t magic) {
  debug_init();
  debug("magic is {x}\n", magic);
  platform_init(platform_data, magic);
  interrupt_init();
  while(1) {}
}


