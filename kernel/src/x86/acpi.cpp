#include "acpi.h"
#include <cstdint>
#include <cstring>
#include "debug.h"
#include "map.h"

#ifdef __x86_64__
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

void parseMadt(const ACPIHeader* table) {
  debug("Found MADT at {} size {}\n", table, table->Length);
}

void parseFacp(const ACPIHeader* table) {
  debug("Found FACP at {} size {}\n", table, table->Length);
}

void parseHpet(const ACPIHeader* table) {

}

void tryAcpiTable(uintptr_t address) {
  size_t length = 0;
  {
    mapping firstpage(address, 0x1000, ReadOnlyMemory);
    ACPIHeader* table = (ACPIHeader*)firstpage.get();
    length = table->Length;
  }
  mapping full(address, length, ReadOnlyMemory);
  ACPIHeader* table = (ACPIHeader*)full.get();
  uint32_t tableId = (table->Signature[0] << 24) | (table->Signature[1] << 16) | (table->Signature[2] << 8) | (table->Signature[3] << 0);
  switch(tableId) {
    case 'APIC':
      parseMadt(table);
      break;
    case 'FACP':
      parseFacp(table);
      break;
    default:
      debug("Found unknown ACPI table {} at {} size {}\n", s2::string_view(table->Signature, table->Signature + 4), table, table->Length);
      break;
  }
}

template <typename T>
void parseSdt(uintptr_t address) {
  mapping sdt(address, 0x2000, ReadOnlyMemory);
  ACPIHeader* table = (ACPIHeader*)sdt.get();
  const T* start = (const T*)(table+1);
  const T* end = start + (table->Length - sizeof(ACPIHeader)) / sizeof(T);
  for (; start != end; ++start) {
    tryAcpiTable((uintptr_t)*start);
  }
}

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

bool check_rsd_ptr(const uint8_t* ptr) {
  uint8_t rsdptr[] = { 'R', 'S', 'D', ' ', 'P', 'T', 'R', ' ' };
  if (memcmp(ptr, rsdptr, 8) != 0) return false;
  const RSDPDescriptor* descriptor = (const RSDPDescriptor*)ptr;
  uint8_t csum = 0;
  for (size_t n = 0; n < 24; n++) csum += ptr[n];
  if (csum != 0) return false;

  bool validXsdt = true;
  for (size_t n = 0; n < 40; n++) csum += ptr[n];
  if (csum != 0) validXsdt = false;
  if (descriptor->Length < 40) validXsdt = false;
  if (validXsdt) {
    parseSdt<uint64_t>(((uintptr_t)descriptor->XsdtAddress_high << 32) | (descriptor->XsdtAddress_low));
  } else {
    parseSdt<uint32_t>((uintptr_t)descriptor->RsdtAddress);
  }
  return true;
}

void acpi_init() {
  debug("ACPI dance\n");
  mapping bios(0xE0000, 0x20000, ReadOnlyMemory);
  const uint8_t* start = (const uint8_t*)bios.get(), *end = (const uint8_t*)bios.get() + 0x20000;
  for (; start != end; start += 16) {
    check_rsd_ptr(start);
  }
}
#endif

