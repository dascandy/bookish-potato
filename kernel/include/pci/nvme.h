#pragma once

#include "pci/PciCore.h"
#include <future.h>

struct ControllerRegs {
  uint64_t cap;
  uint32_t vs;
  uint32_t intms;
  uint32_t intmc;
  uint32_t cc;
  uint32_t res;
  uint32_t csts;
  uint32_t nssr;
  uint32_t aqa;
  uint64_t asq;
  uint64_t acq;
  uint32_t cmbloc;
  uint32_t cmbsz;
  uint32_t bpinfo;
  uint32_t bprsel;
  uint64_t bpmbl;
  uint64_t cmbmsc;
  uint32_t cmbsts;
};

struct NvmeCommand;
struct NvmeCompletion;
struct IdentifyController;
struct IdentifyNamespace;

struct NvmeDisk;

class NvmeDevice : public PciDevice {
  struct PendingCallback {
    uint16_t id;
    s2::promise<uint64_t> p;
  };
public:
  NvmeDevice(uintptr_t confSpaceAddr);
  RegisterMapping<ControllerRegs> ctrl;
  mapping aqs, aqc;
  NvmeCommand* asq;
  NvmeCompletion* acq;
  uint16_t asqi, acqi;
  mapping nsMap;
  IdentifyNamespace* ns;
  mapping idMap;
  IdentifyController* id;
  uint8_t doorbellStride;
  void RingDoorbell(uint32_t doorbell, uint16_t index);
  s2::vector<PendingCallback> completions;
  s2::future<uint64_t> RunAdminCommand(NvmeCommand cmd);
  s2::future<void> start();
  s2::vector<NvmeDisk*> disks;
  void HandleInterrupt();
};

