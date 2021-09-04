#include "vfs.h"
#include "debug.h"

void RegisterDisk(Disk* disk) {
  debug("[VFS] Found disk {}\n", disk);
}

Disk::~Disk() {

}


