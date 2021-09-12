#include "fs/fat.h"
#include <flatmap>
#include "debug.h"
#include "blockcache.h"
#include <cstring>

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
  mapping sector = co_await Blockcache::Instance().read(&disk, offset / 4096, 1);
  uint32_t* p = (uint32_t*)sector.get();
  co_return p[(offset % 4096) / 4];
}

s2::future<s2::vector<Extent>> FatFilesystem::readFatChain(uint64_t cluster) {
  s2::vector<Extent> extents;
  uint64_t currentCluster = cluster;
  uint64_t currentCount = 1;
  while (cluster < (byteCount / clusterSize)) {
    uint64_t next = co_await(getNextCluster(cluster));
    if (cluster + 1 != next) {
      extents.push_back({(clusterOffset + currentCluster * clusterSize) / 4096, (currentCount * clusterSize) / 4096});
      currentCount = 1;
    }
    cluster = next;
  }
  co_return s2::move(extents);
}

s2::future<bool> FatFilesystem::load() {
  mapping bootsector = co_await Blockcache::Instance().read(&disk, 0, 1);
  Fat32BootBlock* bb2 = (Fat32BootBlock*)bootsector.get();

  uint64_t rootdircluster;
  if (bb2->bytespersector != 0) {
    clusterSize = bb2->bytespersector * bb2->sectorspercluster;
    clusterOffset = ((bb2->reservedsectors + (bb2->sectorsperfat + bb2->sectorsperfat2) * bb2->fats) * bb2->bytespersector) - 2 * clusterSize;
    byteCount = (uint64_t)bb2->sectorcount * bb2->bytespersector;
    fatOffset = bb2->reservedsectors * bb2->bytespersector;
    rootdircluster = bb2->rootdircluster;
    debug("[FAT] Found Fat32\n");
    exfat = false;
  } else {
    ExfatBootBlock* bb1 = (ExfatBootBlock*)bootsector.get();
    byteCount = (uint64_t)bb1->clustercount << (bb1->clustershift + bb1->sectorshift);
    clusterSize = 1 << (bb1->clustershift + bb1->sectorshift);
    clusterOffset = (bb1->clusterheapoffset << (bb1->sectorshift)) - 2 * clusterSize;
    fatOffset = bb1->fatoffset << bb1->sectorshift;
    rootdircluster = bb1->rootdircluster;
    debug("[FAT] Found exFat\n");
    exfat = true;
  }
  byteFree = 0; // to determine from fat table?
  root.fs = this;
  root.fileName = "/";
  root.extents = co_await readFatChain(rootdircluster);
  root.type = File::Type::Directory;
  root.fileSize = 0;
  for (auto& e : root.extents) {
    root.fileSize += e.size * 4096;
    debug("{} {} {}\n", e.size, e.offset, root.fileSize);
  }
  co_return true;
}

struct Fat32DirEntry {
  uint8_t shortname[11]; // irrelevant
  uint8_t attributes;
  uint8_t winnt;
  uint8_t ctime_tenths;
  uint16_t ctime;
  uint16_t cdate;
  uint16_t adate;
  uint16_t clusterhigh;
  uint16_t mtime;
  uint16_t mdate;
  uint16_t clusterlow;
  uint32_t size;
};

struct Fat32LfnEntry {
  uint8_t order;
  uint8_t lfn1[10];
  uint8_t attr;
  uint8_t longtype;
  uint8_t checksum;
  uint8_t lfn2[12];
  uint8_t pad[2];
  uint8_t lfn3[4];
};

enum class ExfatEntryType {
  EndOfDirectory = 0x00,
  AllocationBitmap = 0x81,
  UpcaseTable = 0x82,
  VolumeLabel = 0x83,
  File = 0x85,
  StreamExtension = 0xc0,
  FileName = 0xc1,
};

struct ExfatBaseEntry {
  uint8_t type;
  uint8_t custom[19];
  uint32_t firstCluster;
  uint64_t dataLength;
};

struct ExfatAllocationBitmapEntry {
  uint8_t type;
  uint8_t flags;
  uint8_t custom[18];
  uint32_t firstCluster;
  uint64_t dataLength;
};

struct ExfatUpcaseTableEntry {
  uint8_t type;
  uint8_t res1[3];
  uint32_t tablechecksum;
  uint8_t custom[12];
  uint32_t firstCluster;
  uint64_t dataLength;
};

struct ExfatVolumeLabelEntry {
  uint8_t type;
  uint8_t charcount;
  uint8_t volname[22];
  uint64_t reserved;
};

struct ExfatFileEntry {
  uint8_t type;
  uint8_t secondaryCount;
  uint16_t setChecksum;
  uint16_t fileAttrs;
  uint16_t res;
  uint32_t cTime;
  uint32_t mTime;
  uint32_t aTime;
  uint8_t cMs;
  uint8_t mMs;
  uint8_t cUtcOffset;
  uint8_t mUtcOffset;
  uint8_t aUtcOffset;
  uint8_t res2[7];
};

struct ExfatStreamExtensionEntry {
  uint8_t type;
  uint8_t secFlags;
  uint8_t res;
  uint8_t nameLen;
  uint16_t nameHash;
  uint16_t res2;
  uint64_t validDataLength;
  uint32_t res3;
  uint32_t firstCluster;
  uint64_t dataLength;
};

struct ExfatFileNameEntry {
  uint8_t type;
  uint8_t flags;
  uint8_t name[30];
};

s2::future<s2::vector<File>> FatFilesystem::readdir(File& d) {
  mapping directory = co_await d.read(0, (d.fileSize + 4095) / 4096);
  for (size_t n = 0; n < d.fileSize; n++) {
    if (n % 16 == 0) debug("\n{06x}  ", n);
    debug("{02x} ", directory.get()[n]);
  }
  debug("\n");
  s2::vector<File> files;
  s2::vector<uint8_t> lfncache;
  uint8_t lastLfn = 0;
  lfncache.reserve(104);
  if (exfat) {
    ExfatFileEntry* fileEntry = nullptr;
    ExfatStreamExtensionEntry* seEntry = nullptr;

    ExfatBaseEntry* entry = (ExfatBaseEntry*)directory.get();
    for (size_t n = 0; n < d.fileSize / 32; n++) {
      switch((ExfatEntryType)entry[n].type) {
      case ExfatEntryType::EndOfDirectory:
        co_return s2::move(files);
      case ExfatEntryType::File:
        fileEntry = (ExfatFileEntry*)(entry + n);
        break;
      case ExfatEntryType::StreamExtension:
        seEntry = (ExfatStreamExtensionEntry*)(entry + n);
        break;
      case ExfatEntryType::FileName:
      {
        ExfatFileNameEntry* fn = (ExfatFileNameEntry*)(entry + n);
        lfncache.append(s2::span<const uint8_t>{fn->name, fn->name + 30});
        if ((lfncache.size() / 30) < fileEntry->secondaryCount - 1) {
          break;
        }
        File f;
        f.fs = this;
        f.fileSize = seEntry->validDataLength;
        if (seEntry->secFlags & 2) { // no fat chain
          f.extents.push_back({(clusterOffset + clusterSize * seEntry->firstCluster) / 4096, (f.fileSize + 4095) / 4096});
        } else {
          f.extents = co_await readFatChain(seEntry->firstCluster);
        }
        size_t offset = 0;
        while (offset < lfncache.size() && (lfncache[offset] != 0 || lfncache[offset + 1] != 0)) offset += 2;
        lfncache.resize(offset);
        f.fileName = s2::string::from_utf16le(lfncache);
        f.type = fileEntry->fileAttrs & 0x10 ? File::Type::Directory : File::Type::Normal;
        files.push_back(s2::move(f));
        lfncache.clear();
      }
        break;
      default:
        debug("unk {} {}\n", n, entry[n].type);
        break;
      }
    }
  } else {
    Fat32DirEntry* entry = (Fat32DirEntry*)directory.get();
    Fat32LfnEntry* lfn = (Fat32LfnEntry*)directory.get();
    for (size_t n = 0; n < d.fileSize / sizeof(Fat32DirEntry); n++) {
      if (entry[n].attributes == 0xF) {
        if (lastLfn == 0) {
          if ((lfn[n].order & 0x40) == 0x40) {
            lastLfn = lfn[n].order & 0x3F;
            lfncache.resize(lastLfn * 26);
            memcpy(lfncache.data() + (lastLfn - 1) * 26, lfn[n].lfn1, 10);
            memcpy(lfncache.data() + (lastLfn - 1) * 26 + 10, lfn[n].lfn2, 12);
            memcpy(lfncache.data() + (lastLfn - 1) * 26 + 22, lfn[n].lfn3, 4);
          } else {
            debug("[FAT] Ignoring LFN entry {}\n", n);
            lfncache.clear();
            lastLfn = 0;
          }
        } else if (lfn[n].order != lastLfn - 1) {
          debug("[FAT] Found invalid LFN continuation\n");
          lfncache.clear();
          lastLfn = 0;
        } else {
          lastLfn--;
          memcpy(lfncache.data() + (lastLfn - 1) * 26, lfn[n].lfn1, 10);
          memcpy(lfncache.data() + (lastLfn - 1) * 26 + 10, lfn[n].lfn2, 12);
          memcpy(lfncache.data() + (lastLfn - 1) * 26 + 22, lfn[n].lfn3, 4);
        }
      } else if (entry[n].shortname[0] == 0) {
        break;
      } else {
        if (lastLfn != 1) {
          debug("[FAT] Incomplete LFN {} {x}\n", n, lastLfn);
          lfncache.clear();
        }
        
        uint32_t startcluster = ((uint32_t)entry[n].clusterhigh << 16) | (entry[n].clusterlow);
        File f;
        f.fs = this;
        f.extents = co_await readFatChain(startcluster);
        f.fileSize = entry[n].size;
        if (lfncache.empty()) {
          s2::string s = s2::string::from_cp437(s2::span<uint8_t>(entry[n].shortname));
          s2::string name = s.substr(0, 8), ext = s.substr(8, 3);
          while (not name.empty() && name.back() == ' ') name.pop_back();
          if (ext.empty()) {
            f.fileName = name;
          } else {
            f.fileName = name + "." + ext;
          }
        } else {
          size_t offset = 0;
          while (offset < lfncache.size() && (lfncache[offset] != 0 || lfncache[offset + 1] != 0)) offset += 2;
          lfncache.resize(offset);
          for (auto& c : lfncache) {
            debug("{02x} ", c);
          }
          debug("\n");
          f.fileName = s2::string::from_utf16le(lfncache);
        }
        f.type = entry[n].attributes & 0x10 ? File::Type::Directory : File::Type::Normal;
        files.push_back(s2::move(f));
        lfncache.clear();
        lastLfn = 0;
      }
    }
  }
  co_return s2::move(files);
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


