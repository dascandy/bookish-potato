#pragma once

#include "vfs.h"

struct BlockGroup;
struct ExtExtent;
struct ExtIndirectBlocks;

struct ExtFilesystem : Filesystem {
  ExtFilesystem(Disk* disk);
  s2::future<bool> load();
  File getroot() override;
  s2::pair<size_t, size_t> size() override;
  s2::future<File> create(File& parent, s2::string fileName, FileCreateFlags flags = FileCreateFlags::Regular) override;
  s2::future<bool> remove(File& f) override;
  s2::future<bool> rename(File& f, s2::string newName) override;
  s2::future<bool> resizeFile(File& f, uint64_t newSize) override;
  s2::future<s2::vector<File>> readdir(File& f) override;
  s2::future<s2::vector<uint8_t>> hashFile(File& f) override;

private:
  s2::future<s2::vector<Extent>> ReadExtents(ExtExtent* ext);
  s2::future<s2::vector<Extent>> ReadIndirects(ExtIndirectBlocks* ind);
  s2::future<File> readInode(uint64_t ino);

  File root;
  uint64_t byteCount, byteFree;
  uint64_t firstBlock;
  uint32_t blockgroupcount;
  uint32_t blocks_per_group;
  uint32_t clusters_per_group;
  uint32_t inodes_per_group;
  uint32_t inode_size;
  uint32_t desc_size;
  mapping blockgroup;
  BlockGroup* bgs;
};


