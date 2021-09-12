#include "usb/UsbHub.h"

void UsbHub::Initialize() {
  UsbCore::Instance().RegisterClassDriver(0x090000, *new UsbHub());
}

UsbHubDevice::UsbHubDevice(UsbDevice& dev) 
: dev(dev)
{
}

s2::future<void> UsbHubDevice::start() {

  co_return;
}

void UsbHub::AddDevice(UsbDevice& dev) {
  (new UsbHubDevice(dev))->start();
}

void UsbHub::AddInterface(UsbInterface&) {}

