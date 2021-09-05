#pragma once

#include <cstdint>
#include "map.h"
#include <future.h>

enum class HashType {
  SHA1,
  SHA256,
  SHA512,
};

struct Disk {
  virtual ~Disk();
  virtual s2::future<IoMemory> read(uint64_t startblock, uint32_t blockCount) = 0;
  virtual s2::future<void> write(uint64_t startblock, uint32_t blockCount, IoMemory& buffer) = 0;
  virtual s2::future<void> trim(uint64_t startblock, uint32_t blockCount) = 0;
  virtual s2::future<void> flush() = 0;
};

struct Partition : Disk {
  s2::future<IoMemory> read(uint64_t startblock, uint32_t blockCount) override {
    // TODO: overflow checks
    uint64_t actualStart = startblock + start;
    return disk.read(actualStart, blockCount);
  }
  s2::future<void> write(uint64_t startblock, uint32_t blockCount, IoMemory& buffer) override {
    // TODO: overflow checks
    uint64_t actualStart = startblock + start;
    return disk.write(actualStart, blockCount, buffer);
  }
  s2::future<void> trim(uint64_t startblock, uint32_t blockCount) override {
    // TODO: overflow checks
    uint64_t actualStart = startblock + start;
    return disk.trim(actualStart, blockCount);
  }
  s2::future<void> flush() override {
    disk.flush();
  }
  Disk& disk;
  uint64_t start, size;
};

struct Extent {
  uint64_t offset;
  uint64_t size;
};

struct File {
  Disk& disk;
  s2::string fileName;
  uint64_t fileSize;
  enum class Type : uint8_t {
    Normal,
    Directory,
    Symlink,
    UnixFifo,
    UnixSocket,
    UnixCharNode,
    UnixBlockNode,

    Unknown = 255,
  } type = Type::Normal;
  s2::vector<Extent> extents;

  virtual s2::future<s2::vector<File>> readdir() = 0;
  virtual s2::future<s2::vector<uint8_t>> getHash(HashType type) = 0;
  virtual s2::future<bool> resize(uint64_t newsize) = 0;
  virtual s2::future<void> write(uint64_t offset, s2::span<IoMemory> data) = 0;
  virtual s2::future<s2::vector<IoMemory>> read(uint64_t offset, uint64_t size) = 0;
};

struct Filesystem {
  virtual s2::future<s2::optional<File>> getPath(s2::span<const s2::string> path) = 0;
  virtual s2::vector<Extent> emptySpace() = 0;
  virtual s2::pair<size_t, size_t> size() = 0;
  virtual s2::future<File> create(File& parent, s2::string fileName) = 0;
};

void RegisterDisk(Disk* disk);


