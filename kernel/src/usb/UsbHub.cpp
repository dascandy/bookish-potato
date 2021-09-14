#include "usb/UsbHub.h"
#include "debug.h"

void UsbHub::Initialize() {
  UsbCore::Instance().RegisterClassDriver(0x090000, *new UsbHub());
}

UsbHubDevice::UsbHubDevice(UsbInterface& in) 
: in(in)
{
  debug("[USB] Found HUB device\n");
}

s2::future<void> UsbHubDevice::start() {

  co_return;
}

void UsbHub::AddDevice(UsbDevice&) {}

void UsbHub::AddInterface(UsbInterface& in) {
  (new UsbHubDevice(in))->start();
}

