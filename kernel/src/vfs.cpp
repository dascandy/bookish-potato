#include "vfs.h"
#include "debug.h"
#include <flatmap>
#include "fs/fat.h"
#include "fs/mbr.h"
//#include "fs/ext.h"
#include "fs/gpt.h"
#include "blockcache.h"

s2::future<FilesystemType> SniffFilesystem(Disk* disk) {
  mapping bootsector = co_await Blockcache::Instance().read(disk, 0, 1);

  // Filesystem checks in order of reliability

  // Check for GPT info
  if (*(uint64_t*)(bootsector.get() + 512) == 0x5452415020494645ULL)
    co_return FilesystemType::Gpt;

  // Check for valid Ext header
  if (*(uint16_t*)(bootsector.get() + 0x38) == 0xEF53)
    co_return FilesystemType::Ext;

  // Check for valid Fat header
  if (bootsector.get()[0] == 0xEB &&
      bootsector.get()[0x11] == 0 && bootsector.get()[0x12] == 0 &&
      bootsector.get()[0x13] == 0 && bootsector.get()[0x14] == 0) {
    // Might be some form of FAT32/exfat
    if (bootsector.get()[3] == 'E' && 
        bootsector.get()[4] == 'X' &&
        bootsector.get()[5] == 'F' &&
        bootsector.get()[6] == 'A' &&
        bootsector.get()[7] == 'T' &&
        bootsector.get()[8] == ' ' &&
        bootsector.get()[9] == ' ' &&
        bootsector.get()[10] == ' ') {
      co_return FilesystemType::Fat;
    } else if (bootsector.get()[0x42] == 0x29 || bootsector.get()[0x42] == 0x28){
      co_return FilesystemType::Fat;
    }
  }

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

Disk::~Disk() {

}

File::File() 
: fs(nullptr)
, type(File::Type::Unknown)
{
}

File::File(Filesystem* fs, s2::string fileName, uint64_t fileSize, Type type, s2::vector<Extent> extents)
: fs(fs)
, fileName(fileName)
, fileSize(fileSize)
, type(type)
, extents(extents)
{}

s2::future<s2::vector<File>> File::readdir() & {
  return fs->readdir(*this);
}

s2::future<PageSGList> File::read(uint64_t offset, uint64_t size) {
  PageSGList list;
  for (auto& e : extents) {
    if (offset < e.size) {
      uint64_t start = e.offset + offset;
      uint64_t extsize = (e.size < size ? e.size : size);
      list.append(co_await Blockcache::Instance().read(&fs->disk, start, extsize));
      size -= extsize;
    }
    offset += e.size;
    if (size == 0) break;
  }
  if (size != 0) {
    // Attempted to read beyond the end of the file
    debug("[FAT] Somebody tried to read beyond the end of the file\n");
  } 
  co_return list;
}

Partition::Partition(Disk& disk, uint64_t start, uint64_t size) 
: disk(disk)
, start(start)
{
  this->size = size;
}

Partition::~Partition() {

}

s2::flatmap<s2::string, File> dentrycache;

s2::future<void> RegisterFilesystem(Filesystem* fs) {
  debug("[VFS] Found filesystem of {} GiB\n", ((fs->size().first / 1024 / 1024) + 512) / 1024);

  // TODO: more intelligent vfs stuff
  dentrycache["/"] = fs->getroot();
/*
  auto files = co_await dentrycache["/"].readdir();
  for (auto& f : files) {
    debug("{s} {s} {} {}\n", f.fileName, f.type == File::Type::Normal ? "File" : "Directory", f.fileSize, f.extents.size());
  }
  auto cstddef = files[4];
  mapping file = co_await cstddef.read(0, (cstddef.fileSize + 4095) / 4096);
  s2::string data{(const char*)file.get(), cstddef.fileSize};
  debug("Cstddef:\n{s}\n", data);
  */
  co_return;
}

bool isAutomountable(const File& file) {
  (void)file;
  return false;
}

// lookup with a slash at the end to open a folder, or open as a folder. Open without to open a file or open as a file.
// no difference, except for automounting things
// TODO: Limit symlink following count to remove chance for DoS of kernel
s2::future<s2::optional<File>> VfsLookup(s2::string_view path) {
  size_t currentLength = 0;
  File currentEntry;
  for (const auto& [entrypath, entry] : dentrycache) {
    if (entrypath.size() > currentLength && path.startsWith(entrypath)) {
      currentLength = entrypath.size();
      currentEntry = entry;
    }
  }
  path = path.subview(currentLength);
  while (not path.empty()) {
    if (currentEntry.type == File::Type::Symlink) {
      // read file
      debug("[VFS] TODO: implement symlink support\n");
      while(1) {}
    } else {
      if (currentEntry.type == File::Type::Normal &&
        isAutomountable(currentEntry)) {
        debug("[VFS] TODO: implement automount support\n");
        while(1) {}
        // Automount subfilesystem
      }
      auto [newpath, newentry] = co_await currentEntry.resolve(path);
      if (newentry.type == File::Type::Unknown) {
        co_return s2::nullopt;
      }
      path = newpath;
      currentEntry = newentry;
    }
  }
  co_return currentEntry;
}


