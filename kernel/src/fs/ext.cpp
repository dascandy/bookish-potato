#include "fs/ext.h"
#include <flatmap>
#include "debug.h"
#include "blockcache.h"
#include <cstring>

struct ExtSuperblock {
  uint32_t 	inodes_count;
  uint32_t 	blocks_count_lo;
  uint32_t 	r_blocks_count_lo;
  uint32_t 	free_blocks_count_lo;
  uint32_t 	free_inodes_count;
  uint32_t 	first_data_block;
  uint32_t 	log_block_size;
  uint32_t 	log_cluster_size;
  uint32_t 	blocks_per_group;
  uint32_t 	clusters_per_group;
  uint32_t 	inodes_per_group;
  uint32_t 	mtime;
  uint32_t 	wtime;
  uint16_t 	mnt_count;
  uint16_t 	max_mnt_count;
  uint16_t 	magic;
  uint16_t 	state;
  uint16_t 	errors;
  uint16_t 	minor_rev_level;
  uint32_t 	lastcheck;
  uint32_t 	checkinterval;
  uint32_t 	creator_os;
  uint32_t 	rev_level;
  uint16_t 	def_resuid;
  uint16_t 	def_resgid;
  uint32_t 	first_ino;
  uint16_t 	inode_size;
  uint16_t 	block_group_nr;
  uint32_t 	feature_compat;
  uint32_t 	feature_incompat;
  uint32_t 	feature_ro_compat;
  uint8_t 	uuid[16];
  char 	volume_name[16];
  char 	last_mounted[64];
  uint32_t 	algorithm_usage_bitmap;
  uint8_t 	prealloc_blocks;
  uint8_t 	prealloc_dir_blocks;
  uint16_t 	reserved_gdt_blocks;
  uint8_t 	journal_uuid[16];
  uint32_t 	journal_inum;
  uint32_t 	journal_dev;
  uint32_t 	last_orphan;
  uint32_t 	hash_seed[4];
  uint8_t 	def_hash_version;
  uint8_t 	jnl_backup_type;
  uint16_t 	desc_size;
  uint32_t 	default_mount_opts;
  uint32_t 	first_meta_bg;
  uint32_t 	mkfs_time;
  uint32_t 	jnl_blocks[17];
  uint32_t 	blocks_count_hi;
  uint32_t 	r_blocks_count_hi;
  uint32_t 	free_blocks_count_hi;
  uint16_t 	min_extra_isize;
  uint16_t 	want_extra_isize;
  uint32_t 	flags;
  uint16_t 	raid_stride;
  uint16_t 	mmp_interval;
  uint64_t 	mmp_block;
  uint32_t 	raid_stripe_width;
  uint8_t 	log_groups_per_flex;
  uint8_t 	checksum_type;
  uint16_t 	reserved_pad;
  uint64_t 	kbytes_written;
  uint32_t 	snapshot_inum;
  uint32_t 	snapshot_id;
  uint64_t 	snapshot_r_blocks_count;
  uint32_t 	snapshot_list;
  uint32_t 	error_count;
  uint32_t 	first_error_time;
  uint32_t 	first_error_ino;
  uint64_t 	first_error_block;
  uint8_t 	first_error_func[32];
  uint32_t 	first_error_line;
  uint32_t 	last_error_time;
  uint32_t 	last_error_ino;
  uint32_t 	last_error_line;
  uint64_t 	last_error_block;
  uint8_t 	last_error_func[32];
  uint8_t 	mount_opts[64];
  uint32_t 	usr_quota_inum;
  uint32_t 	grp_quota_inum;
  uint32_t 	overhead_blocks;
  uint32_t 	backup_bgs[2];
  uint8_t 	encrypt_algos[4];
  uint8_t 	encrypt_pw_salt[16];
  uint32_t 	lpf_ino;
  uint32_t 	prj_quota_inum;
  uint32_t 	checksum_seed;
  uint32_t 	reserved[98];
  uint32_t 	checksum;
};
static_assert(sizeof(ExtSuperblock) == 1024);

struct BlockGroup {
  uint32_t block_bitmap_lo;
  uint32_t inode_bitmap_lo;
  uint32_t inode_table_lo;
  uint16_t free_blocks_lo;
  uint16_t free_inodes_lo;
  uint16_t used_dirs_count_lo;
  uint16_t flags;
  uint32_t exclude_bitmap_lo;
  uint16_t block_bitmap_csum_lo;
  uint16_t inode_bitmap_csum_lo;
  uint16_t inode_table_unused_lo;
  uint16_t checksum;
  uint32_t block_bitmap_hi;
  uint32_t inode_bitmap_hi;
  uint32_t inode_table_hi;
  uint16_t free_blocks_hi;
  uint16_t free_inodes_hi;
  uint16_t used_dirs_count_hi;
  uint16_t inode_table_unused_hi;
  uint32_t exclude_bitmap_hi;
  uint16_t block_bitmap_csum_hi;
  uint16_t inode_bitmap_csum_hi;
  uint32_t reserved;
};
static_assert(sizeof(BlockGroup) == 64);

struct Inode {
  uint16_t mode;
  uint16_t uid;
  uint32_t size_lo;
  uint32_t atime;
  uint32_t ctime;
  uint32_t mtime;
  uint32_t dtime;
  uint16_t gid;
  uint16_t links_count;
  uint32_t blocks_lo;
  uint32_t flags;
  uint32_t version;
  uint8_t block[60];
  uint32_t generation;
  uint32_t file_acl_lo;
  uint32_t size_hi;
  uint32_t obso_faddr;
  uint16_t blocks_hi;
  uint16_t file_acl_hi;
  uint16_t uid_hi;
  uint16_t gid_hi;
  uint16_t checksum_lo;
  uint16_t reserved;
  uint16_t extra_isize;
  uint16_t checksum_hi;
  uint32_t ctime_extra;
  uint32_t mtime_extra;
  uint32_t atime_extra;
  uint32_t crtime;
  uint32_t crtime_extra;
  uint32_t version_hi;
  uint32_t projid;
};
static_assert(sizeof(Inode) == 160);

enum Ext4Flags {
  EXT4_SYNC_FL = 0x8,
  EXT4_IMMUTABLE_FL = 0x10,
  EXT4_APPEND_FL = 0x20,
  EXT4_NODUMP_FL = 0x40,
  EXT4_NOATIME_FL = 0x80,
  EXT4_DIRTY_FL = 0x100,
  EXT4_NOCOMPR_FL = 0x400,
  EXT4_ENCRYPT_FL = 0x800,
  EXT4_INDEX_FL = 0x1000,
  EXT4_IMAGIC_FL = 0x2000,
  EXT4_JOURNAL_DATA_FL = 0x4000,
  EXT4_DIRSYNC_FL = 0x10000,
  EXT4_TOPDIR_FL = 0x20000,
  EXT4_HUGE_FILE_FL = 0x40000,
  EXT4_EXTENTS_FL = 0x80000,
  EXT4_EA_INODE_FL = 0x200000,
  EXT4_INLINE_DATA_FL = 0x10000000,
  EXT4_PROJINHERIT_FL = 0x20000000,
  EXT4_RESERVED_FL = 0x80000000,
};

ExtFilesystem::ExtFilesystem(Disk* disk) 
: Filesystem(*disk)
{
}

struct ExtExtent {
  struct Header {
    uint16_t magic;
    uint16_t entrycount;
    uint16_t maxentries;
    uint16_t depth;
    uint32_t generation; // unused
  };
  struct Node {
    uint32_t block;
    uint32_t leaf_lo;
    uint16_t leaf_hi;
    uint16_t res;
  };
  struct Leaf {
    uint32_t block;
    uint16_t len;
    uint16_t start_hi;
    uint32_t start_lo;
  };
  Header h;
  union {
    Node node[1];
    Leaf leaf[1];
  };
};

struct ExtIndirectBlocks {
  uint32_t direct[12];
  uint32_t ind1;
  uint32_t ind2;
  uint32_t ind3;
};

s2::future<s2::vector<Extent>> ExtFilesystem::ReadExtents(ExtExtent* ext) {
  s2::vector<Extent> rv;
  assert(ext->h.depth == 0);
  for (size_t n = 0; n < ext->h.entrycount; n++) {
    AddExtent(rv, {firstBlock + ((uint64_t)ext->leaf[n].start_hi << 32) + ext->leaf[n].start_lo, ext->leaf[n].len});
  }
  co_return s2::move(rv);
}

s2::future<s2::vector<Extent>> ExtFilesystem::ReadIndirects(ExtIndirectBlocks* ind) {
  s2::vector<Extent> rv;
  assert(ind->ind1 == 0 && ind->ind2 == 0 && ind->ind3 == 0);
  for (size_t n = 0; n < 12; n++) {
    AddExtent(rv, Extent{ind->direct[n], 1});
  }
  co_return s2::move(rv);
}

s2::future<File> ExtFilesystem::readInode(uint64_t ino) {
  // Notably, Ext filesystems don't have filenames in the inode; files don't have names, file entries have names.
  uint64_t blockgroup = (ino - 1) / inodes_per_group;
  uint64_t offset = ((ino - 1) % inodes_per_group) * inode_size;
  uint64_t inode_table_root = bgs[blockgroup].inode_table_lo;
  if (desc_size > 32) {
    inode_table_root += ((uint64_t)bgs[blockgroup].inode_table_hi << 32);
  }

  debug("{x} {x} {x}\n", blockgroup, inode_table_root + (offset / 4096), offset % 4096);
  mapping inodeblock = co_await Blockcache::Instance().read(&disk, inode_table_root + (offset / 4096), 1);
  Inode* ip = (Inode*)(inodeblock.get() + (offset % 4096));
  for (size_t n = 0; n < inode_size; n++) {
    debug("{02x} ", ((uint8_t*)ip)[n]);
    if (n % 16 == 15) debug("\n");
  }
  File f;
  f.fs = this;
  f.fileSize = ((uint64_t)ip->size_hi << 32) + ip->size_lo;
  switch(ip->mode & 0xF000) {
    case 0x1000: f.type = File::Type::UnixFifo; break;
    case 0x2000: f.type = File::Type::UnixCharNode; break;
    case 0x4000: f.type = File::Type::Directory; break;
    case 0x6000: f.type = File::Type::UnixBlockNode; break;
    case 0x8000: f.type = File::Type::Normal; break;
    case 0xA000: f.type = File::Type::Symlink; break;
    case 0xC000: f.type = File::Type::UnixSocket; break;
    default: f.type = File::Type::Unknown; break;
  }
  if (ip->flags & EXT4_EXTENTS_FL) {
    f.extents = co_await ReadExtents((ExtExtent*)(ip->block));
  } else {
    f.extents = co_await ReadIndirects((ExtIndirectBlocks*)(ip->block));
  }
  co_return s2::move(f);
}

s2::future<bool> ExtFilesystem::load() {
  mapping bootsector = co_await Blockcache::Instance().read(&disk, 0, 1);
  ExtSuperblock* bb = (ExtSuperblock*)(bootsector.get() + 0x400);
  uint64_t blocksize = (1024 << bb->log_block_size);
  uint64_t blockCount = (((uint64_t)bb->blocks_count_hi << 32) + bb->blocks_count_lo);
  byteCount = blocksize * blockCount;
  byteFree = blocksize * (((uint64_t)bb->free_blocks_count_hi << 32) + bb->free_blocks_count_lo);
  firstBlock = bb->first_data_block;
  blockgroupcount = (blockCount + bb->blocks_per_group - 1) / bb->blocks_per_group;
  blockgroup = co_await Blockcache::Instance().read(&disk, blocksize / 4096, (blockgroupcount + 127) / 128);
  bgs = (BlockGroup*)blockgroup.get();
  blocks_per_group = bb->blocks_per_group;
  clusters_per_group = bb->clusters_per_group;
  inodes_per_group = bb->inodes_per_group;
  inode_size = bb->inode_size;
  desc_size = bb->desc_size;
  debug("Found ExtN {x} {x} {x} {x}\n", firstBlock, blocks_per_group, inodes_per_group, inode_size);

  root = co_await readInode(2);
  root.fileName = "/";
  co_return true;
}

struct ExtDirEntry {
  uint32_t inode;
  uint16_t reclen;
  uint8_t namelen;
  uint8_t filetype;
  uint8_t filename[1];
};

s2::future<s2::vector<File>> ExtFilesystem::readdir(File& d) {
  debug("Reading rootdir {}\n", d.fileSize);
  mapping directory = co_await d.read(0, (d.fileSize + 4095) / 4096);
  for (size_t n = 0; n < d.fileSize; n++) {
    if (n % 16 == 0) debug("\n{06x}  ", n);
    debug("{02x} ", directory.get()[n]);
  }
  debug("\n");

  s2::vector<File> files;
  ExtDirEntry* dirEnt = (ExtDirEntry*)directory.get();
  while ((uint8_t*)dirEnt < directory.get() + d.fileSize) {
    if (dirEnt->inode != 0) {
      File f = co_await readInode(dirEnt->inode);
      f.fileName = s2::string(dirEnt->filename, dirEnt->filename + dirEnt->namelen);
      files.push_back(s2::move(f));
    }
    if (dirEnt->reclen < 9) { // Ended up desynced (corrupted), just stop here.
      break;
    }
    dirEnt = (ExtDirEntry*)((uint8_t*)dirEnt + dirEnt->reclen);
  }
  co_return s2::move(files);
}

s2::pair<size_t, size_t> ExtFilesystem::size() {
  return {byteCount, byteFree};
}

File ExtFilesystem::getroot() {
  return root;
}

// WRITE FUTURE

s2::future<File> ExtFilesystem::create(File& parent, s2::string fileName, FileCreateFlags flags) {
  co_return File(this, fileName, 0, File::Type::Normal, {});
}

s2::future<bool> ExtFilesystem::remove(File& f) {
  co_return false;
}

s2::future<bool> ExtFilesystem::rename(File& f, s2::string newName) {
  co_return false;
}

s2::future<bool> ExtFilesystem::resizeFile(File& f, uint64_t newSize) {
  co_return true;
}

s2::future<s2::vector<uint8_t>> ExtFilesystem::hashFile(File& f) {
  co_return {};
}


