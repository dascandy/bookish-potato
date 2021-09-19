#include "scsi.h"
#include <span>
#include "debug.h"

s2::vector<uint8_t> Scsi::Inquiry(uint8_t lun) {
  uint8_t rv[] = {
    0x12,
    (uint8_t)(lun << 5),
    0,
    0,
    0x24,
    0,
  };
  return rv;
}

s2::vector<uint8_t> Scsi::Read10(uint8_t lun, uint32_t lba, uint16_t transferLength) {
  debug("[SCSI] READ10 {} {} {}\n", lun, lba, transferLength);
  uint8_t rv[] = {
    0x28,
    (uint8_t)(lun << 5),
    (uint8_t)(lba >> 24),
    (uint8_t)((lba >> 16) & 0xFF),
    (uint8_t)((lba >> 8) & 0xFF),
    (uint8_t)(lba & 0xFF),
    0,
    (uint8_t)(transferLength >> 8),
    (uint8_t)(transferLength & 0xFF),
    0,
    0,
    0
  };
  return rv;
}

s2::vector<uint8_t> Scsi::RequestSense(uint8_t lun, uint8_t allocLen) {
  uint8_t rv[] = {
    0x03,
    (uint8_t)(lun << 5),
    0,
    0,
    (uint8_t)(allocLen),
    0,
  };
  return rv;
}

s2::vector<uint8_t> Scsi::TestUnitReady(uint8_t lun) {
  uint8_t rv[] = {
    0x00,
    (uint8_t)(lun << 5),
    0,
    0,
    0,
    0,
  };
  return rv;
}

s2::vector<uint8_t> Scsi::ReadCapacity(uint8_t lun) {
  uint8_t rv[] = {
    0x25,
    (uint8_t)(lun << 5),
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
  };
  return rv;
}

s2::vector<uint8_t> Scsi::Write10(uint8_t lun, uint32_t lba, uint16_t transferLength) {
  debug("[SCSI] Write10 {} {} {}\n", lun, lba, transferLength);
  uint8_t rv[] = {
    0x2a,
    (uint8_t)(lun << 5),
    (uint8_t)(lba >> 24),
    (uint8_t)((lba >> 16) & 0xFF),
    (uint8_t)((lba >> 8) & 0xFF),
    (uint8_t)(lba & 0xFF),
    0,
    (uint8_t)(transferLength >> 8),
    (uint8_t)(transferLength & 0xFF),
    0,
    0,
    0
  };
  return rv;
}


