#pragma once

#include "vfs.h"

struct FatFilesystem : Filesystem {
  FatFilesystem(Disk* disk);
  s2::future<bool> load();
  File getroot() override;
  s2::future<uint64_t> getNextCluster(uint64_t currentCluster);
  s2::future<s2::vector<Extent>> readFatChain(uint64_t cluster);
  s2::pair<size_t, size_t> size() override;
  s2::future<File> create(File& parent, s2::string fileName, FileCreateFlags flags = FileCreateFlags::Regular) override;
  s2::future<bool> remove(File& f) override;
  s2::future<bool> rename(File& f, s2::string newName) override;
  s2::future<bool> resizeFile(File& f, uint64_t newSize) override;
  s2::future<s2::vector<File>> readdir(File& f) override;
  s2::future<s2::vector<uint8_t>> hashFile(File& f) override;
  bool exfat = false;
  File root;
  uint64_t byteCount;
  uint64_t byteFree;

  uint64_t clusterOffset;
  uint32_t clusterSize;
  uint64_t fatOffset;
};


