#include "usb/UsbStorage.h"
#include "debug.h"
#include "map.h"
#include "scsi.h"

void UsbStorage::Initialize() 
{
  UsbCore::Instance().RegisterClassDriver(0x080650, *new UsbStorage());
}

UsbStorageDevice::UsbStorageDevice(UsbInterface& in) 
: in(in)
{
  debug("[USB] Found Storage device\n");
}

s2::future<void> UsbStorageDevice::start() 
{
  EndpointDescriptor* in_ep = nullptr, *out_ep = nullptr;
  for (auto usbd : in.GetInterfaceDescriptors()) {
    if (usbd->type == (int)DescriptorType::Endpoint) {
      EndpointDescriptor* ep = (EndpointDescriptor*)usbd;
      if (in_ep == nullptr && (ep->address & 0x80) == 0x80) {
        debug("[USBSTORAGE] in Addr {}\n", ep->address & 0x7F);
        in_ep = ep;
      } else if (out_ep == nullptr && (ep->address & 0x80) == 0) {
        debug("[USBSTORAGE] out Addr {}\n", ep->address & 0x7F);
        out_ep = ep;
      } else {
        debug("[USBSTORAGE] Found an endpoint I don't know what to do with. Ignoring.\n");
      }
    }
  }
  assert(in_ep != nullptr);
  assert(out_ep != nullptr);

  ine = co_await in.StartupEndpoint(*in_ep);
  oute = co_await in.StartupEndpoint(*out_ep);

  mapping page(freepage_get_zeroed(), 4096, DeviceMemory);
  PageSGList list;
  list.pages.push_back(page.to_physical(page.get()));
  co_await RunCommand(Scsi::Inquiry(0), false, 0, sizeof(Scsi::InquiryRv), list);
  Scsi::InquiryRv* rv = (Scsi::InquiryRv*)page.get();
  s2::string vendor(rv->vendorId, rv->vendorId+8);
  s2::string product(rv->productId, rv->productId+16);
  s2::string rev(rv->revision, rv->revision+4);
  debug("[USBSTORAGE] {s} {s} {s}\n", vendor, product, rev);
  co_await RunCommand(Scsi::ReadCapacity(0), false, 0, sizeof(Scsi::ReadCapacityRv), list);
  Scsi::ReadCapacityRv* rv2 = (Scsi::ReadCapacityRv*)page.get();
  sectorsize = rv2->size();
  this->size = (uint64_t)rv2->lba() * rv2->size();

  RegisterDisk(this, FilesystemType::Unknown);

  co_return;
}

s2::future<PageSGList> UsbStorageDevice::read(uint64_t startblock, uint32_t blockCount) {
  PageSGList list;
  for (size_t n = 0; n < blockCount; n++) {
    list.pages.push_back(freepage_get());
  }
  co_await RunCommand(Scsi::Read10(0, startblock * 4096 / sectorsize, blockCount * 4096 / sectorsize), false, 0, blockCount * 4096, list);
  co_return list;
}

s2::future<void> UsbStorageDevice::write(uint64_t startblock, uint32_t blockCount, PageSGList buffer) {
  co_await RunCommand(Scsi::Write10(0, startblock * 4096 / sectorsize, blockCount * 4096 / sectorsize), false, 0, blockCount * 4096, buffer);
}

s2::future<void> UsbStorageDevice::trim(uint64_t startblock, uint32_t blockCount) {
  PageSGList list;
  uint64_t zeropage = freepage_get_zeroed();
  for (size_t n = 0; n < blockCount; n++) {
    list.pages.push_back(zeropage);
  }
  co_await RunCommand(Scsi::Write10(0, startblock * 4096 / sectorsize, blockCount * 4096 / sectorsize), false, 0, blockCount * 4096, list);
  // TODO: 
  // freepage_release(zeropage);
}

s2::future<void> UsbStorageDevice::flush() {
  co_return;
}



/*
  s2::vector<uint8_t> Inquiry(uint8_t lun);
  s2::vector<uint8_t> Read10(uint8_t lun, uint32_t lba, uint16_t transferLength);
  s2::vector<uint8_t> RequestSense(uint8_t lun, uint8_t allocLen);
  s2::vector<uint8_t> TestUnitReady(uint8_t lun);
  s2::vector<uint8_t> ReadCapacity(uint8_t lun);
  s2::vector<uint8_t> Write10(uint8_t lun, uint32_t lba, uint16_t transferLength);

  struct ReadCapacityRv {
    uint32_t lastLba;
    uint32_t blockSize;
  };

  struct InquiryRv {
    uint8_t type;
    uint8_t rmb;
    uint8_t res[2];
    uint8_t additionalLength;
    uint8_t res2[3];
    uint8_t vendorId[8];
    uint8_t productId[16];
    uint8_t revision[4];
  };
*/

s2::future<s2::pair<uint8_t, uint32_t>> UsbStorageDevice::RunCommand(s2::span<const uint8_t> command, bool write, uint8_t lun, size_t transferSize, PageSGList buffer)
{
  debug("[USBSTORAGE] tfsize {} bufsize {}\n", transferSize, buffer.pages.size());
  uint32_t tag = 42;
  mapping cbw(freepage_get_zeroed(), 4096, DeviceMemory);
  uint8_t cbw_header[] = { 0x55, 0x53, 0x42, 0x43, (uint8_t)(tag & 0xFF), (uint8_t)((tag >> 8) & 0xFF), (uint8_t)((tag >> 16) & 0xFF), (uint8_t)((tag >> 24) & 0xFF),
                (uint8_t)(transferSize & 0xFF), (uint8_t)((transferSize >> 8) & 0xFF), (uint8_t)((transferSize >> 16) & 0xFF), (uint8_t)((transferSize >> 24) & 0xFF),
                (uint8_t)(write ? 0x0 : 0x80), lun, 
                (uint8_t)command.size() 
                };
  memcpy(cbw.get(), cbw_header, sizeof(cbw_header));
  memcpy(cbw.get() + sizeof(cbw_header), command.data(), command.size());
  co_await oute->WriteData(cbw.to_physical(cbw.get()), 31);
  if (write) {
    size_t writeLeft = transferSize;
    for (auto& page : buffer.pages) {
      size_t toWrite = writeLeft < 4096 ? writeLeft : 4096;
      co_await oute->WriteData(page, toWrite);
      writeLeft -= toWrite;
    }
  } else {
    size_t readLeft = transferSize;
    for (auto& page : buffer.pages) {
      size_t toRead = readLeft < 4096 ? readLeft : 4096;
      co_await ine->ReadData(page, toRead);
      readLeft -= toRead;
    }
  }
  co_await ine->ReadData(cbw.to_physical(cbw.get()), 13);
  uint8_t status = cbw.get()[12];
  uint32_t residue = cbw.get()[8] | (cbw.get()[9] << 8) | (cbw.get()[10] << 16) | ((uint32_t)cbw.get()[11] << 24);
  co_return {status, residue};
}

void UsbStorage::AddDevice(UsbDevice&) {}

void UsbStorage::AddInterface(UsbInterface& in) 
{
  (new UsbStorageDevice(in))->start();
}

