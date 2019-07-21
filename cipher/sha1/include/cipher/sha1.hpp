#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace Cipher {

class Sha1 {
  uint32_t state[5];
public:
  Sha1();
  void AddBlock(const uint8_t* data, size_t length);
  std::vector<uint8_t> GetHash();
};

}

