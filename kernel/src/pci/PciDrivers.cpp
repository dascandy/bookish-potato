#include "pci/PciDrivers.h"
#include "pci/PciCore.h"
#include "pci/nvme.h"
#include "usb/xhci.h"
#include "pci/bga.h"
#include "pci/virtiogpu.h"

void init_pci_drivers() {
  PciCore::Instance().RegisterVidPidDriver(0x1234, 0x1111, [](uintptr_t dev) { return new BgaFramebuffer(dev); });
  PciCore::Instance().RegisterVidPidDriver(0x1af4, 0x1050, [](uintptr_t dev) { return new VirtioGpu(dev); });
  
  PciCore::Instance().RegisterClassDriver(0x010802, [](uintptr_t dev) { return new NvmeDevice(dev); });
  PciCore::Instance().RegisterClassDriver(0x0c0330, [](uintptr_t dev) { return new XhciDevice(dev); });

  PciCore::Instance().RegisterClassDriver(0x060400, [](uintptr_t dev) { return new PciBridge(dev, 0); });
}

