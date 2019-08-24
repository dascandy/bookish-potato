#include "acpi.h"
#include <cstdint>
#include <cstring>
#include "debug.h"

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

void tryAcpiTable(const ACPIHeader* table) {
  uint32_t tableId = (table->Signature[0] << 24) | (table->Signature[1] << 16) | (table->Signature[2] << 8) | (table->Signature[3] << 0);
  switch(tableId) {
    case 'APIC':
      parseMadt(table);
      break;
    case 'FACP':
      parseFacp(table);
      break;
    default:
      debug("Found unknown ACPI table {} at {} size {}\n", std::string_view(table->Signature, table->Signature + 4), table, table->Length);
      break;
  }
}

void parseRsdt(const ACPIHeader* table) {
  debug("ACPI RSDT\n");
  const uint32_t* start = (const uint32_t*)(table+1);
  const uint32_t* end = start + (table->Length - sizeof(ACPIHeader)) / sizeof(uint32_t);
  for (; start != end; ++start) {
    tryAcpiTable((const ACPIHeader*)((uintptr_t)*start));
  }
}

void parseXsdt(const ACPIHeader* table) {
  debug("ACPI XSDT\n");
  const uint64_t* start = (const uint64_t*)(table+1);
  const uint64_t* end = start + (table->Length - sizeof(ACPIHeader)) / sizeof(uint64_t);
  size_t n = 1;
  for (; start != end; ++start) {
    tryAcpiTable((const ACPIHeader*)(*start));
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

