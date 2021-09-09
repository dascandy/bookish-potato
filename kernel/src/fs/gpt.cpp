#include "fs/mbr.h"
#include "vfs.h"
#include "debug.h"
#include "uuid.h"

struct [[gnu::packed]] gpt_header {
  uint64_t magic;
  uint32_t revision;
  uint32_t headersize;
  uint32_t crc32;
  uint32_t res;
  uint64_t thislba;
  uint64_t altlba;
  uint64_t firstblock;
  uint64_t lastblock;
  uuid_t diskid;
  uint64_t partlba;
  uint32_t partitionCount;
  uint32_t partentrysize;
  uint32_t partcrc;
};

struct gpt_entry {
  uuid_t type;
  uuid_t id;
  uint64_t startLba;
  uint64_t endLba;
  uint64_t attr;
  uint8_t partname[72];
};

s2::future<bool> ParseGptPartitions(Disk* disk) {
//  uint64_t size = disk->size;
  mapping bootsector = co_await disk->read(0, 5);

  // Check for GPT header
  gpt_header* header = (gpt_header*)(bootsector.get() + 512);

  if (header->magic != 0x5452415020494645ULL ||
      header->partentrysize < sizeof(gpt_entry) ||
      header->partitionCount > 128) {
    debug("{x} {} {}\n", header->magic, header->partentrysize, header->partitionCount);
    debug("[VFS] Rejecting for GPT\n");
    co_return false;
  }

  gpt_entry* ent = (gpt_entry*)(bootsector.get() + 1024);
  for (size_t n = 0; n < 128; n++) {
    FilesystemType probableType;
    switch(ent[n].type) {
      case uuid(0xEBD0A0A2, 0xB9E5, 0x4433, 0x87C0, 0x68B6B72699C7):
        probableType = FilesystemType::Fat;
        break;
      case uuid(0x0FC63DAF, 0x8483, 0x4772, 0x8E79, 0x3D69D8477DE4):
      case uuid(0x44479540, 0xF297, 0x41B2, 0x9AF7, 0xD131D5F0458A):
      case uuid(0x4F68BCE3, 0xE8CD, 0x4DB1, 0x96E7, 0xFBCAF984B709):
      case uuid(0x69DAD710, 0x2CE4, 0x4E3C, 0xB16C, 0x21A1D49ABED3):
      case uuid(0xB921B045, 0x1DF0, 0x41C3, 0xAF44, 0x4C6F280D3FAE):
      case uuid(0xBC13C2FF, 0x59E6, 0x4262, 0xA352, 0xB275FD6F7172):
      case uuid(0xE6D6D379, 0xF507, 0x44C2, 0xA23C, 0x238F2A3DF928):
      case uuid(0x933AC7E1, 0x2EB4, 0x4F13, 0xB844, 0x0E14E2AEF915):
      case uuid(0x3B8F8425, 0x20E0, 0x4F3B, 0x907F, 0x1A25A76F98E8):
        probableType = FilesystemType::Ext;
        break;
      case uuid(0,0,0,0,0):
        probableType = FilesystemType::Nothing;
        break;
      default:
        debug("{016x} {016x}\n", (uint64_t)(ent[n].type >> 64), (uint64_t)ent[n].type);
        probableType = FilesystemType::Unknown;
        break;
    }
    if (probableType != FilesystemType::Nothing) {
      debug("[VFS] GPT found {} uuid {016x} {016x}\n", n, (uint64_t)(ent[n].type >> 64), (uint64_t)ent[n].type);
      RegisterDisk(new Partition(*disk, ent[n].startLba / 8, (ent[n].endLba - ent[n].startLba) / 8), probableType);
    }
  }
  debug("[VFS] GPT probe done\n");
  co_return true;
}


