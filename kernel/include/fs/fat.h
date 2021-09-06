#pragma once

#include "vfs.h"

struct FatFilesystem : Filesystem {
  FatFilesystem(Disk* disk);
  s2::future<bool> load();
  s2::pair<size_t, size_t> size() override;
  s2::future<s2::optional<File>> lookup(s2::span<const s2::string> path) override;
  s2::future<File> create(File& parent, s2::string fileName, FileCreateFlags flags = FileCreateFlags::Regular) override;
  s2::future<bool> remove(File& f) override;
  s2::future<bool> rename(File& f, s2::string newName) override;
  s2::future<bool> resizeFile(File& f, uint64_t newSize) override;
  s2::future<s2::vector<File>> readdir(File& f) override;
  s2::future<s2::vector<uint8_t>> hashFile(File& f) override;
};


