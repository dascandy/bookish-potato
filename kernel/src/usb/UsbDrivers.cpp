#include "usb/UsbDrivers.h"
#include "usb/UsbHid.h"

void init_usb_drivers() {
  UsbHid::Initialize();
}


