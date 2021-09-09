#include "fs/fat.h"
#include <flatmap>
#include "debug.h"
#include "blockcache.h"

FatFilesystem::FatFilesystem(Disk* disk) 
: Filesystem(*disk)
{
}

struct [[gnu::packed]] Fat32BootBlock {
  uint8_t jump[3];
  uint8_t oem[8];
  uint16_t bytespersector;
  uint8_t sectorspercluster;
  uint16_t reservedsectors;
  uint8_t fats;
  uint16_t rootdirentries;
  uint16_t sectors;
  uint8_t mdt;
  uint16_t sectorsperfat;
  uint16_t sectorspertrack;
  uint16_t heads;
  uint32_t hiddensectors;
  uint32_t sectorcount;
  uint32_t sectorsperfat2;
  uint16_t flags;
  uint16_t fatversion;
  uint32_t rootdircluster;
  uint16_t fsinfo_sector;
  uint16_t backupsector;
  uint8_t res[12];
  uint8_t driveno;
  uint8_t ntflags;
  uint8_t sig;
  uint32_t serialno;
  uint8_t vollabel[11];
  uint8_t fat32_str[8];
};
static_assert(sizeof(Fat32BootBlock) == 90);

struct ExfatBootBlock {
  uint8_t pad[64];
  uint64_t partitionoffset;
  uint64_t volumelength;
  uint32_t fatoffset;
  uint32_t fatlength;
  uint32_t clusterheapoffset;
  uint32_t clustercount;
  uint32_t rootdircluster;
  uint32_t serialno;
  uint16_t fsrev;
  uint16_t flags;
  uint8_t sectorshift;
  uint8_t clustershift;
  uint8_t fatcount;
  uint8_t driveselect;
  uint8_t percinuse;
  uint8_t reserved[7];
};
static_assert(sizeof(ExfatBootBlock) == 120);

s2::future<uint64_t> FatFilesystem::getNextCluster(uint64_t currentCluster) {
  uint64_t offset = fatOffset + currentCluster * 4;
  mapping sector = co_await disk.read(offset / 4096, 1);
  uint32_t* p = (uint32_t*)sector.get();
  co_return p[offset % 4096];
}

s2::future<s2::vector<Extent>> FatFilesystem::readFatChain(uint64_t cluster) {
  s2::vector<Extent> extents;
  uint64_t currentCluster = cluster;
  uint64_t currentCount = 1;
  while (cluster < (byteCount / clusterSize)) {
    uint64_t next = co_await(getNextCluster(cluster));
    if (cluster + 1 != next) {
      extents.push_back({currentCluster * clusterSize, currentCount * clusterSize});
      currentCount = 1;
    }
    cluster = next;
  }
  co_return s2::move(extents);
}

s2::future<bool> FatFilesystem::load() {
  mapping bootsector = co_await disk.read(0, 1);
  Fat32BootBlock* bb2 = (Fat32BootBlock*)bootsector.get();
  for (size_t n = 0; n < 512; n++) {
    debug("{02x} ", bootsector.get()[n]);
    if (n % 16 == 15) debug("\n");
  }

  uint64_t rootdircluster;
  if (bb2->bytespersector != 0) {
    clusterSize = bb2->bytespersector * bb2->sectorspercluster;
    clusterOffset = ((bb2->reservedsectors + (bb2->sectorsperfat + bb2->sectorsperfat2) * bb2->fats) * bb2->bytespersector) - 2 * clusterSize;
    byteCount = (uint64_t)bb2->sectorcount * bb2->bytespersector;
    fatOffset = bb2->reservedsectors * bb2->bytespersector;
    rootdircluster = bb2->rootdircluster;
    debug("[FAT] Found Fat32\n");
  } else {
    ExfatBootBlock* bb1 = (ExfatBootBlock*)bootsector.get();
    byteCount = (uint64_t)bb1->clustercount << bb1->clustershift;
    clusterSize = 1 << (bb1->clustershift);
    clusterOffset = (bb1->clusterheapoffset << bb1->clustershift) - 2 * clusterSize;
    fatOffset = bb1->fatoffset << bb1->sectorshift;
    rootdircluster = bb1->rootdircluster;
    debug("[FAT] Found exFat\n");
  }
  debug("[FAT] {x} {x} {x} {x}\n", clusterSize, clusterOffset, byteCount, fatOffset);
  byteFree = 0; // to determine from fat table?
  root.fs = this;
  root.fileName = "/";
  root.extents = co_await readFatChain(rootdircluster);
  root.type = File::Type::Directory;
  root.fileSize = 0;
  for (auto& e : root.extents) {
    root.fileSize += e.size * 4096;
  }
  co_return true;
}

s2::future<s2::vector<File>> FatFilesystem::readdir(File& f) {

  (void)f;
  co_return {};
}

s2::pair<size_t, size_t> FatFilesystem::size() {
  return {byteCount, byteFree};
}

File FatFilesystem::getroot() {
  return root;
}

// WRITE FUTURE

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

s2::future<s2::vector<uint8_t>> FatFilesystem::hashFile(File& f) {
  co_return {};
}


