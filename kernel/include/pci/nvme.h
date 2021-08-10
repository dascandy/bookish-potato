#pragma once

#include "pci/PciCore.h"

class NvmeDevice : public PciDevice {
public:
  NvmeDevice(uintptr_t confSpaceAddr);
};

