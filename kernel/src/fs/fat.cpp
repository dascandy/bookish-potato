#include "fs/fat.h"

FatFilesystem::FatFilesystem(Disk* disk) {
  (void)disk;
}

s2::future<bool> FatFilesystem::load() {
  co_return true;
}

s2::pair<size_t, size_t> FatFilesystem::size() {
  return {0, 0};
}

s2::future<s2::optional<File>> FatFilesystem::lookup(s2::span<const s2::string> path) {
  co_return {};
}

s2::future<File> FatFilesystem::create(File& parent, s2::string fileName, FileCreateFlags flags) {
  co_return File(this, fileName, 0, File::Type::Normal, {});
}

s2::future<bool> FatFilesystem::remove(File& f) {
  co_return false;
}

s2::future<bool> FatFilesystem::rename(File& f, s2::string newName) {
  co_return false;
}

s2::future<bool> FatFilesystem::resizeFile(File& f, uint64_t newSize) {
  co_return true;
}

s2::future<s2::vector<File>> FatFilesystem::readdir(File& f) {
  co_return {};
}

s2::future<s2::vector<uint8_t>> FatFilesystem::hashFile(File& f) {
  co_return {};
}


