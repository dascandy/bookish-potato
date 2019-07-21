#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace Cipher {

class Sha256 {
  uint32_t state[8];
public:
  Sha256();
  void AddBlock(const uint8_t* data, size_t length);
  std::vector<uint8_t> GetHash();
};

}

