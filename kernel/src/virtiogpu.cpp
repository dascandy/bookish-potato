#include "virtiogpu.h"
#include "io.h"
#include "pci.h"
#include "map.h"
#include "debug.h"

// TODO: be able to read edid before creating screen
VirtioGpu::VirtioGpu(pcidevice dev) 
: screen(dev, {})
{
}

VirtioGpu::VirtioGpuScreen::VirtioGpuScreen(pcidevice dev, s2::span<const uint8_t> edid) 
: Screen(edid)
{
  debug("Found VirtIO graphics adapter\n");
}

bool VirtioGpu::VirtioGpuScreen::SetActiveResolution(const Resolution& res, size_t bufferCount) {
  return true;
}

future<void> VirtioGpu::VirtioGpuScreen::QueueBuffer(void* ptr) {
  return {};
}

void* VirtioGpu::VirtioGpuScreen::GetFreeBuffer() {
  return nullptr;
}


