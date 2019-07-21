#include "platform.h"
#include <stdlib.h>
#include "io.h"
#include "pci.h"
#include "bga.h"
#include "xhci.h"
#include "interrupt.h"
#include "apic.h"
#include <cstring>
#include "debug.h"

#ifdef __x86_64__
struct e820map {
    unsigned long long offset;
    unsigned long long size;
    unsigned int type;
    unsigned int pad;
};

void platform_setup_memory(void*) {}

extern "C" char __end;
void platform_setup(void* platform_struct) {
  platform_setup_memory(platform_struct);
  interrupt_init();
  pci_detect([](pcidevice dev){
    uint32_t vendor_device = pciread32(dev, 0);
    bool deviceFound = true;
    switch(vendor_device) {
      case 0x11111234:
      case 0x80EEBEEF:
        new BgaFramebuffer(dev);
        break;
      default:
        deviceFound = false;
        break;
    }
    if (deviceFound) return;
    uint32_t devClass = pciread32(dev, 8);
    /*
    if ((devClass & 0xFFFF0000) == 0x01010000)
      createIdeStorage(dev);
    else if ((devClass & 0xFFFFFF00) == 0x0c033000)
      new Xhci(dev);
      */
  });
}

#endif


