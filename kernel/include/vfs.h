#pragma once

#include <cstdint>
#include "map.h"
#include <future.h>
#include <string>
#include <span>
#include <optional>

enum class FilesystemType {
  Nothing = 0,
  Unknown = 1,
  Mbr = 2,  // Partitioning
  Gpt = 3,  // Partitioning
  Fat = 4,  // FAT32 or exFAT
  Ext = 5,  // Ext2/3/4
};

struct Disk {
  virtual ~Disk();
  virtual s2::future<PageSGList> read(uint64_t startblock, uint32_t blockCount) = 0;
  virtual s2::future<void> write(uint64_t startblock, uint32_t blockCount, PageSGList buffer) = 0;
  virtual s2::future<void> trim(uint64_t startblock, uint32_t blockCount) = 0;
  virtual s2::future<void> flush() = 0;
  uint64_t size;
};

struct Partition : Disk {
  Partition(Disk& disk, uint64_t start, uint64_t size);
  ~Partition() override;
  s2::future<PageSGList> read(uint64_t startblock, uint32_t blockCount) override {
    // TODO: overflow checks
    uint64_t actualStart = startblock + start;
    return disk.read(actualStart, blockCount);
  }
  s2::future<void> write(uint64_t startblock, uint32_t blockCount, PageSGList buffer) override {
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
    return disk.flush();
  }
  Disk& disk;
  uint64_t start;
};

struct Extent {
  uint64_t offset;
  uint64_t size;
};

struct Filesystem;

struct File {
  Filesystem* fs;
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

  File(Filesystem* fs, s2::string fileName, uint64_t fileSize, Type type, s2::vector<Extent> extents);
  File();
  s2::future<s2::vector<File>> readdir() &;
  s2::future<s2::vector<uint8_t>> getHash();
  s2::future<bool> resize(uint64_t newsize);
  s2::future<void> write(uint64_t offset, PageSGList data);
  s2::future<PageSGList> read(uint64_t offset, uint64_t size);
  s2::future<s2::pair<s2::string, File>> resolve(s2::string_view path);
};

struct Filesystem {
  Disk& disk;
  Filesystem(Disk& disk) : disk(disk) {}
  enum class FileCreateFlags {
    Regular = 0, // Create file if it does not exists, return existing file if it does.
    Exclusive = 1, // Create only if it does not exist. Use this for lock files.
    Replace = 2, // If the file exists, create a blank file instead of what's there. Only replaces regular files.
  };
  virtual s2::pair<size_t, size_t> size() = 0;
  virtual File getroot() = 0;
  // Do not use for exclusive-create because of TOCTTOU
  virtual s2::future<File> create(File& parent, s2::string fileName, FileCreateFlags flags = FileCreateFlags::Regular) = 0;
  virtual s2::future<bool> remove(File& f) = 0;
  virtual s2::future<bool> rename(File& f, s2::string newName) = 0;

  virtual s2::future<bool> resizeFile(File& f, uint64_t newSize) = 0;
  virtual s2::future<s2::vector<File>> readdir(File& f) = 0;
  virtual s2::future<s2::vector<uint8_t>> hashFile(File& f) = 0;
};

void RegisterDisk(Disk* disk, FilesystemType hint);
s2::future<void> RegisterFilesystem(Filesystem* fs);

s2::future<s2::optional<File>> VfsLookup(s2::string_view path);


