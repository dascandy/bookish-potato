#include "acpi.h"
#include <cstdint>
#include <cstring>
#include "crash.h"

struct RSDPDescriptor {
 char Signature[8];
 uint8_t Checksum;
 char OEMID[6];
 uint8_t Revision;
 uint32_t RsdtAddress;
 uint32_t Length;
 uint32_t XsdtAddress_high, XsdtAddress_low;
 uint8_t ExtendedChecksum;
};

struct ACPIHeader {
  char Signature[4];
  uint32_t Length;
  uint8_t Revision;
  uint8_t Checksum;
  char OEMID[6];
  char OEMTableID[8];
  uint32_t OEMRevision;
  uint32_t CreatorID;
  uint32_t CreatorRevision;
};
static_assert(sizeof(ACPIHeader) == 36);

void parseRsdt(const ACPIHeader* table) {
  crash_hexdump_region(10, (const uint8_t*)table, 128, 7);
  // TODO: map this region before reading it
  return;

  const uint32_t* start = (const uint32_t*)(table+1);
  const uint32_t* end = start + (table->Length - sizeof(ACPIHeader)) / sizeof(uint32_t);
  size_t n = 0;
  for (; start != end; ++start) {
    const ACPIHeader* t = (const ACPIHeader*)((uintptr_t)*start);
    char name[5];
    name[0] = t->Signature[0];
    name[1] = t->Signature[1];
    name[2] = t->Signature[2];
    name[3] = t->Signature[3];
    name[4] = 0;
    crash_text(0, n, name, 7);
    n++;
  }
}

void parseXsdt(const ACPIHeader* table) {
  const uint64_t* start = (const uint64_t*)(table+1);
  const uint64_t* end = start + (table->Length - sizeof(ACPIHeader)) / sizeof(uint64_t);
  size_t n = 0;
  for (; start != end; ++start) {
    const ACPIHeader* t = (const ACPIHeader*)(*start);
    char name[5];
    name[0] = t->Signature[0];
    name[1] = t->Signature[1];
    name[2] = t->Signature[2];
    name[3] = t->Signature[3];
    name[4] = 0;
    crash_text(0, n, name, 7);
    n++;
  }
}

bool check_rsd_ptr(const uint8_t* ptr) {
  uint8_t rsdptr[] = { 'R', 'S', 'D', ' ', 'P', 'T', 'R', ' ' };
  if (memcmp(ptr, rsdptr, 8) != 0) return false;
  const RSDPDescriptor* descriptor = (const RSDPDescriptor*)ptr;
  crash_hexdump_region(0, (const uint8_t*)descriptor, 64, 7);
  uint8_t csum = 0;
  for (size_t n = 0; n < 24; n++) csum += ptr[n];
  if (csum != 0) return false;
  bool validXsdt = true;
  for (size_t n = 0; n < 40; n++) csum += ptr[n];
  if (csum != 0) validXsdt = false;
  if (descriptor->Length < 40) validXsdt = false;
  if (validXsdt) {
    uintptr_t xsdt = ((uintptr_t)descriptor->XsdtAddress_high << 32) | (descriptor->XsdtAddress_low);
    parseXsdt((const ACPIHeader*)xsdt);
  } else {
    uintptr_t rsdt = ((uintptr_t)descriptor->RsdtAddress);
    parseRsdt((const ACPIHeader*)rsdt);
  }
  return true;
}

void acpi_init() {
  const uint8_t* start = (uint8_t*)0xe0000, *end = (uint8_t*)0x100000;
  for (; start != end; start += 16) {
    check_rsd_ptr(start);
  }
}

