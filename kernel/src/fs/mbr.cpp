#include "fs/mbr.h"
#include "vfs.h"
#include "debug.h"

struct [[gnu::packed]] mbr_entry {
  uint8_t bootable;
  uint8_t chs_start[3];
  uint8_t id;
  uint8_t chs_end[3];
  uint32_t start;
  uint32_t length;
};

s2::future<bool> ParseMbrPartitions(Disk* disk) {
  // 4k blocks
  uint64_t size = disk->size;
  debug("Loading boot sector\n");
  mapping bootsector = co_await disk->read(0, 1);
  if (bootsector.get()[510] != 0x55 && bootsector.get()[511] != 0xaa) {
    co_return false;
  }
  
  mbr_entry* ent = (mbr_entry*)(bootsector.get() + 0x1be);
  for (size_t n = 0; n < 4; n++) {
    if ((ent[n].start & 0x7) ||
        (ent[n].length & 0x7) ||
        (ent[n].length < 262144) ||
        (ent[n].start + ent[n].length > 8*size)) {
      debug("Invalid partition found, ignoring\n");
    } else {
      debug("partition at {} length {}\n", ent[n].start, ent[n].length);
      FilesystemType probableType;
      switch(ent[n].id) {
        case 0x01:
        case 0x04:
        case 0x06:
        case 0x07:
        case 0x0b:
        case 0x0c:
        case 0x0e:
          // Microsoft made a mess of this
          probableType = FilesystemType::Fat;
          break;
        case 0x83:
          probableType = FilesystemType::Ext;
          break;
        case 0x0f:
          probableType = FilesystemType::Mbr;
          break;
        default:
          probableType = FilesystemType::Unknown;
          break;
      }
      RegisterDisk(new Partition(*disk, ent[n].start / 8, ent[n].length / 8), probableType);
    }
  }
  co_return true;
}


