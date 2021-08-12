#pragma once

#include <cstdint>
#include "map.h"
#include <future.h>

struct Disk {
  virtual ~Disk() = default;
  virtual s2::future<IoMemory> read(uint64_t startblock, uint32_t blockCount) = 0;
  virtual s2::future<void> write(uint64_t startblock, uint32_t blockCount, IoMemory& buffer) = 0;
  virtual s2::future<void> trim(uint64_t startblock, uint32_t blockCount) = 0;
  virtual s2::future<void> flush() = 0;
};

void RegisterDisk(Disk* disk);


