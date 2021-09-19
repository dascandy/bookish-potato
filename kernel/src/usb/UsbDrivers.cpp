#include "usb/UsbDrivers.h"
#include "usb/UsbHid.h"
#include "usb/UsbHub.h"
#include "usb/UsbStorage.h"

void init_usb_drivers() {
  UsbHid::Initialize();
  UsbHub::Initialize();
  UsbStorage::Initialize();
}


