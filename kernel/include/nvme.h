#pragma once

#include "pci.h"

class NvmeDevice : public PciDevice {
public:
  NvmeDevice(uintptr_t confSpaceAddr);
};

