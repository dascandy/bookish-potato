#include "virtiogpu.h"
#include "io.h"
#include "pci.h"
#include "map.h"
#include "debug.h"

// TODO: be able to read edid before creating screen
VirtioGpu::VirtioGpu(uintptr_t confSpaceAddr)
: PciDevice(confSpaceAddr)
, screen({})
{
}

VirtioGpu::VirtioGpuScreen::VirtioGpuScreen(s2::span<const uint8_t> edid) 
: Screen(edid)
{
  debug("Found VirtIO graphics adapter\n");
}

bool VirtioGpu::VirtioGpuScreen::SetActiveResolution(const Resolution& res, size_t bufferCount) {
  return true;
}

s2::future<void> VirtioGpu::VirtioGpuScreen::QueueBuffer(void* ptr) {
  return {};
}

void* VirtioGpu::VirtioGpuScreen::GetFreeBuffer() {
  return nullptr;
}


