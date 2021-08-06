#include "nvme.h"

NvmeDevice::NvmeDevice(uintptr_t confSpaceAddr)
: PciDevice(confSpaceAddr)
{
}


