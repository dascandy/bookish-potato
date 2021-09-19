#pragma once

#include <cstdint>
#include <vector>

namespace Scsi {
  s2::vector<uint8_t> Inquiry(uint8_t lun);
  s2::vector<uint8_t> Read10(uint8_t lun, uint32_t lba, uint16_t transferLength);
  s2::vector<uint8_t> RequestSense(uint8_t lun, uint8_t allocLen);
  s2::vector<uint8_t> TestUnitReady(uint8_t lun);
  s2::vector<uint8_t> ReadCapacity(uint8_t lun);
  s2::vector<uint8_t> Write10(uint8_t lun, uint32_t lba, uint16_t transferLength);

  struct ReadCapacityRv {
    uint32_t lba() {
      return ((lastLba >> 24) & 0xFF) |
             ((lastLba >> 8) & 0xFF00) |
             ((lastLba << 8) & 0xFF0000) |
             ((lastLba << 24) & 0xFF000000);
    }
    uint32_t size() {
      return ((blockSize >> 24) & 0xFF) |
             ((blockSize >> 8) & 0xFF00) |
             ((blockSize << 8) & 0xFF0000) |
             ((blockSize << 24) & 0xFF000000);
    }
  private:
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
};


