#pragma once

#include "pci.h"

class Xhci {
public:
  Xhci(pcidevice dev);
private:
  void start();
  pcidevice dev;
};


