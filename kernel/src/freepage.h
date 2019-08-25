#pragma once

#include <cstddef>
#include <cstdint>

uint64_t freepage_get();
void freepage_add_region(uint64_t start, size_t length);


