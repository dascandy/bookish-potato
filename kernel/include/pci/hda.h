#pragma once

#include "pci/PciCore.h"
#include <future.h>

struct HdaControllerRegs {
  uint16_t gcap;
  uint8_t vmin;
  uint8_t vmaj;
  uint16_t outpay;
  uint16_t inpay;
  uint16_t gctl;
  uint16_t wakeen;
  uint16_t statests;
  uint64_t gsts;
  uint16_t outstrmpay;
  uint16_t instrmpay;
  uint32_t res;
  uint32_t intctl;
  uint32_t intsts;
  uint64_t res2;
  uint32_t counter;
  uint32_t ssync;
  uint64_t res3;
  uint64_t corbbase;
  uint16_t corbwp;
  uint16_t corbrp;
  uint8_t corbctl;
  uint8_t corbsts;
  uint16_t corbsize;
  uint64_t rirbbase;
  uint16_t rirbwp;
  uint16_t rirbrp;
  uint8_t rirbctl;
  uint8_t rirbsts;
  uint16_t rirbsize;
  uint64_t res4[2];
  uint64_t dpbase;
  uint64_t res5;
};

class HdaDevice : public PciDevice {
  struct PendingCallback {
    uint16_t id;
    s2::promise<uint64_t> p;
  };
public:
  HdaDevice(uintptr_t confSpaceAddr);
  RegisterMapping<HdaControllerRegs> ctrl;
  mapping aqs, aqc;
  uint32_t* asq;
  uint32_t* acq;
  uint16_t asqi, acqi;
  s2::vector<PendingCallback> completions;
  s2::future<void> start();
  void HandleInterrupt();
};

