#include "pci/hda.h"
#include "debug.h"
#include "uuid.h"
#include "freepage.h"
#include <cassert>
#include <string>
#include "vfs.h"

enum HdaCommand {
  GetParameter = 0xf0000,
  GetSelectedInput = 0xf0100,
  SetSelectedInput = 0x70100,
  GetStreamChannel = 0xf0600,
  SetStreamChannel = 0x70600,
  GetPinWidgetControl = 0xf0700,
  SetPinWidgetControl = 0x70700,
  GetVolumeControl = 0xf0f00,
  SetVolumeControl = 0x70f00,
  GetConfigurationDefault = 0xf1c00,
  GetConverterChannelCount = 0xf2d00,
  SetConverterChannelCount = 0x72d00,
  FunctionReset = 0x7ff00,
  SetAmplifierGain = 0x30000,
};

enum HdaParams {
  VendorId = 0,
  RevisionId = 2,
  NodeCount = 4,
  FunctionGroupType = 5,
  AudioGroupCaps = 8,
  AudioWidgetCaps = 9,
  SupportedPcmRates = 10,
  SupportedFormats = 11,
  PinCapabilities = 12,
  InputAmplifierCapabilities = 13,
  ConnectionListLength = 14,
  SupportedPowerStates = 15,
  ProcessingCapabilities = 16,
  GpioCount = 17,
  OutputAmplifierCapabilities = 18,
  VolumeCapabilities = 19,
};

HdaDevice::HdaDevice(uintptr_t confSpaceAddr)
: PciDevice(confSpaceAddr)
, ctrl(conf, PciBars::Bar0)
, aqs(freepage_get_zeroed(), 0x1000, DeviceMemory)
, aqc(freepage_get_zeroed(), 0x1000, DeviceMemory)
, asq((uint32_t*)aqs.get())
, acq((uint32_t*)aqc.get())
{
  debug("[HDA] Found HDA version {}.{}\n", ctrl->vmaj, ctrl->vmin);

  ctrl->corbbase = aqs.to_physical(asq);
  ctrl->rirbbase = aqc.to_physical(acq);

  ctrl->corbwp = asqi = 0; 
  ctrl->rirbwp = acqi = 0;

  RegisterInterruptHandler(0, [this]{ HandleInterrupt(); });

  start();
}

s2::future<void> HdaDevice::start() {
/*
  ctrl->cc = 0x460001;
  auto ctrlRes = RunAdminCommand(Identify(idMap.to_physical(id), 1, 0, 0, 0, 0));
  uint64_t status = co_await RunAdminCommand(Identify(nsMap.to_physical(ns), 2, 0, 0, 0, 0));
  (void)status;
  uint32_t* nsids = (uint32_t*)ns;
  while (*nsids != 0) nsids++;
  s2::vector<uint32_t> validNsids((uint32_t*)ns, nsids);
  assert(validNsids.size() == 1);
  assert(validNsids.back() == 1);

  for (auto& nsid : validNsids) {
    disks.push_back(new HdaDisk(*this, nsid, disks.size() + 1));
  }
  uint64_t ctrlStatus = co_await ctrlRes;
  (void)ctrlStatus;
  const char* snstart = id->serialNum, *snend = id->modelNo;
  const char* mstart = id->modelNo, *mend = id->firmwareRev;
  const char* fwstart = id->firmwareRev, *fwend = id->firmwareRev + 8;
  while (snstart != snend && snend[-1] == ' ') snend--;
  while (mstart != mend && mend[-1] == ' ') mend--;
  while (fwstart != fwend && fwend[-1] == ' ') fwend--;
  s2::string sn(snstart, snend);
  s2::string model(mstart, mend);
  s2::string fw(fwstart, fwend);
  debug("[NVME] Found {s} version {s} (SN# {s})\n", model, fw, sn);
*/
  co_return;
}

void HdaDevice::HandleInterrupt() {
/*
  while (true) {
    uint8_t i = acqi % 0x100;
    bool phase = not (acqi & 0x100);
    if ((acq[i].status & 1) != phase) {
      return;
    }
    for (auto& c : completions) {
      if (c.id == acq[i].cmdid) {
        c.p.set_value((acq[i].dw0 << 16) | (acq[i].status & 0xFFFE));
      }
    }
    acqi++;
  }
*/
}


