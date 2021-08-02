#include "UsbDrivers.h"
#include "UsbHid.h"

void init_usb_drivers() {
  UsbHid::Initialize();
}


