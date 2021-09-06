#include "vfs.h"
#include "debug.h"
#include "fs/fat.h"
#include "fs/mbr.h"
//#include "fs/ext.h"
#include "fs/gpt.h"

s2::future<FilesystemType> SniffFilesystem(Disk* disk) {
  IoMemory bootsector = co_await disk->read(0, 1);
  // Check for valid Ext header
  if (*(uint16_t*)(bootsector.get() + 0x38) == 0xEF53)
    co_return FilesystemType::Ext;

  // Check for GPT info
  if (*(uint64_t*)(bootsector.get() + 512) == 0x5452415020494645ULL)
    co_return FilesystemType::Gpt;

  // Check for valid Fat header
  if ((bootsector.get()[0x42] == 0x29 || bootsector.get()[0x42] == 0x28) &&
      bootsector.get()[0x11] == 0 && bootsector.get()[0x12] == 0 &&
      bootsector.get()[0x13] == 0 && bootsector.get()[0x14] == 0)
    co_return FilesystemType::Fat;

  // Check for MBR signature (at least)
  if (bootsector.get()[510] == 0x55 && bootsector.get()[511] == 0xaa)
    co_return FilesystemType::Mbr;

  // No idea.
  co_return FilesystemType::Unknown;
}

s2::future<Filesystem*> TryUse(Disk* disk, FilesystemType type) {
  switch(type) {
  case FilesystemType::Mbr:
    ParseMbrPartitions(disk);
    co_return nullptr;
  case FilesystemType::Gpt:
    ParseGptPartitions(disk);
    co_return nullptr;
  case FilesystemType::Fat:
  {
    FatFilesystem* fs = new FatFilesystem(disk);
    if (co_await fs->load())
      co_return fs;
  }
    break;
/*    
  case FilesystemType::Ext:
    ExtFilesystem* ext = new ExtFilesystem(disk);
    // if failed return nullptr;
    co_return ext;
*/
  default:
    debug("[VFS] Unknown filesystem device type\n");
    break;
  }
  co_return nullptr;
}

s2::future<void> ProbeDisk(Disk* disk, FilesystemType hint) {
  if (hint != FilesystemType::Unknown) {
    Filesystem* fs = co_await TryUse(disk, hint);
    if (fs) {
      RegisterFilesystem(fs);
      co_return;
    }
  }
  hint = co_await SniffFilesystem(disk);
  debug("[VFS] Sniffed filesystem, think it is {}\n", (int)hint);
  Filesystem* fs = co_await TryUse(disk, hint);
  if (fs) {
    RegisterFilesystem(fs);
  }
}

void RegisterDisk(Disk* disk, FilesystemType hint) {
  debug("[VFS] Found disk {x}, probing\n", disk);
  ProbeDisk(disk, hint);
}

void RegisterFilesystem(Filesystem* fs) {
  debug("[VFS] Found filesystem {x}\n", fs);
}

Disk::~Disk() {

}

File::File(Filesystem* fs, s2::string fileName, uint64_t fileSize, Type type, s2::vector<Extent> extents)
: fs(fs)
, fileName(fileName)
, fileSize(fileSize)
, type(type)
, extents(extents)
{}

Partition::Partition(Disk& disk, uint64_t start, uint64_t size) 
: disk(disk)
, start(start)
{
  this->size = size;
}

Partition::~Partition() {
}


