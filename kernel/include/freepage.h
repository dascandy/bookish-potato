#pragma once

#include <cstddef>
#include <cstdint>

uint64_t freepage_get();
uint64_t freepage_get_range(size_t pagecount);
uint64_t freepage_get_zeroed();
void freepage_add_region(uint64_t start, size_t length);


