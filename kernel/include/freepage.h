#pragma once

#include <cstddef>
#include <cstdint>

enum class DmaType {
  Unrestricted,
  Bit32,
};

uint64_t freepage_get();
uint64_t freepage_get_range(size_t pagecount, DmaType = DmaType::Unrestricted);
uint64_t freepage_get_zeroed();
void freepage_add_region(uint64_t start, size_t length);


