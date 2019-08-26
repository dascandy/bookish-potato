#pragma once

#include <cstddef>
#include <cstdint>

void asa_init();
uintptr_t asa_alloc(size_t size);
void asa_free(uintptr_t ptr, size_t size);



