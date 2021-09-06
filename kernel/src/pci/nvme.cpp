#include "pci/nvme.h"
#include "debug.h"
#include "uuid.h"
#include "freepage.h"
#include <cassert>
#include <string>
#include "vfs.h"

struct NvmeCommand {
  uint8_t opcode;
  uint8_t fuse_psdt;
  uint16_t cid;
  uint32_t nsid;
  uint64_t rs;
  uint64_t metadataPtr;
  uint64_t prp1;
  uint64_t prp2;
  uint32_t cdw10;
  uint32_t cdw11;
  uint32_t cdw12;
  uint32_t cdw13;
  uint32_t cdw14;
  uint32_t cdw15;
};

static_assert(sizeof(NvmeCommand) == 64);

struct NvmeCompletion {
  uint32_t dw0;
  uint32_t dw1;
  uint16_t sqhead;
  uint16_t sqid;
  uint16_t cmdid;
  uint16_t status;
};

static_assert(sizeof(NvmeCompletion) == 16);

struct PowerStateDescriptor {
  uint16_t mp;
  uint8_t res;
  uint8_t ps;
  uint32_t enlat;
  uint32_t exlat;
  uint8_t rrt;
  uint8_t rrl;
  uint8_t rwt;
  uint8_t rwl;
  uint16_t idlp;
  uint8_t ips;
  uint8_t res2;
  uint16_t actp;
  uint8_t ap;
  uint8_t res3[9];
};

static_assert(sizeof(PowerStateDescriptor) == 32);

struct IdentifyController {
  uint16_t vendorId;
  uint16_t svid;
  char serialNum[20];
  char modelNo[40];
  char firmwareRev[8];
  uint8_t rab;
  uint8_t ieee_oui[3];
  uint8_t cmic;
  uint8_t mdts;
  uint16_t cntlid;
  uint32_t ver;
  uint32_t rtd3r;
  uint32_t rtd3e;
  uint32_t oaes;
  uint32_t ctratt;
  uint16_t rrls;
  uint8_t res[9];
  uint8_t cntrltype;
  uuid fguid;
  uint16_t crdt1;
  uint16_t crdt2;
  uint16_t crdt3;
  uint8_t res2[106];
  uint8_t dunno[16];
  uint16_t oacs;
  uint8_t acl;
  uint8_t aerl;
  uint8_t rfmw;
  uint8_t lpa;
  uint8_t elpe;
  uint8_t npss;
  uint8_t avscc;
  uint8_t apsta;
  uint16_t wctemp;
  uint16_t cctemp;
  uint16_t mtfa;
  uint32_t hmpre;
  uint32_t hmmin;
  uint128_t tnvmcap;
  uint128_t unvmcap;
  uint32_t rpmbs;
  uint16_t edstt;
  uint8_t dsto;
  uint8_t fwug;
  uint16_t kas;
  uint16_t hctma;
  uint16_t mntmt;
  uint16_t mxtmt;
  uint32_t sanicap;
  uint32_t hmminds;
  uint16_t hmmaxd;
  uint16_t nsetidmax;
  uint16_t endgidmax;
  uint8_t anatt;
  uint8_t anacap;
  uint32_t anagrpmx;
  uint32_t nanagrpid;
  uint32_t pels;
  uint8_t res3[156];
  uint8_t sqes;
  uint8_t cqes;
  uint16_t maxcmd;
  uint32_t nn;
  uint16_t oncs;
  uint16_t fuses;
  uint8_t fna;
  uint8_t vwc;
  uint16_t awun;
  uint16_t awupf;
  uint8_t nvscc;
  uint8_t nwpc;
  uint16_t acwu;
  uint16_t res4;
  uint32_t sgls;
  uint32_t mnan;
  uint8_t res5[224];
  char subnqn[256];
  uint8_t res6[1024];
  PowerStateDescriptor psd[32];
  uint8_t res7[1024];
} __attribute__((packed));
static_assert(sizeof(IdentifyController) == 4096);

struct IdentifyNamespace {
  uint64_t nsze;
  uint64_t ncap;
  uint64_t nuse;
  uint8_t nsfeat;
  uint8_t nlbaf;
  uint8_t flbas;
  uint8_t mc;
  uint8_t dpc;
  uint8_t dps;
  uint8_t nmic;
  uint8_t rescap;
  uint8_t fpi;
  uint8_t dlfeat;
  uint16_t nawun;
  uint16_t nawupf;
  uint16_t nacwu;
  uint16_t nabsn;
  uint16_t nabo;
  uint16_t nabspf;
  uint16_t noiob;
  uint128_t nvmcap;
  uint16_t npwg;
  uint16_t npwa;
  uint16_t npdg;
  uint16_t npda;
  uint16_t nows;
  uint8_t res[18];
  uint32_t anagrpid;
  uint8_t res2[3];
  uint8_t nsattr;
  uint16_t nvmsetid;
  uint16_t endgid;
  uuid nguid;
  uint64_t eui64;
  uint32_t lbaf[16];
  uint8_t res3[3904];
} __attribute__((packed));
static_assert(sizeof(IdentifyNamespace) == 4096);

NvmeCommand CreateIOCQ(uintptr_t physBuffer, uint16_t qsize, uint16_t qid, uint16_t interruptVector) {
  return { 
    0x05, 
    0,
    0,
    0,
    0,
    0,
    physBuffer,
    0,
    ((uint32_t)qsize << 16) | qid,
    ((uint32_t)interruptVector << 16) | 3,
    0,
    0,
    0,
    0
  };
}
NvmeCommand CreateIOSQ(uintptr_t physBuffer, uint16_t qsize, uint16_t qid, uint16_t cqid, uint16_t qprio, uint16_t nvmsetid) {
  return { 
    0x01, 
    0,
    0,
    0,
    0,
    0,
    physBuffer,
    0,
    ((uint32_t)qsize << 16) | qid,
    ((uint32_t)cqid << 16) | (qprio << 1) | 1,
    nvmsetid,
    0,
    0,
    0
  };
}
NvmeCommand Identify(uintptr_t physBuffer, uint8_t cns, uint16_t cntid, uint32_t nsid, uint16_t nvmsetid, uint8_t uuidindex) {
  return { 
    0x06, 
    0,
    0,
    nsid,
    0,
    0,
    physBuffer,
    0,
    ((uint32_t)cntid << 16) | cns,
    nvmsetid,
    0,
    0,
    uuidindex,
    0
  };
}

NvmeCommand Flush(uint16_t nsid) {
  return { 
    0x00, 
    0,
    0,
    nsid,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0
  };
}
NvmeCommand Write(uint16_t nsid, uintptr_t targetBuffer, uint64_t startLba, uint16_t blockCount) {
  return { 
    0x01, 
    0,
    0,
    nsid,
    0,
    0,
    targetBuffer,
    0,
    (uint32_t)(startLba & 0xFFFFFFFF),
    (uint32_t)(startLba >> 32),
    blockCount,
    0,
    0,
    0
  };
}
NvmeCommand Trim(uint16_t nsid, uint64_t startLba, uint16_t blockCount) {
  return { 
    0x08, 
    0,
    0,
    nsid,
    0,
    0,
    0,
    0,
    (uint32_t)(startLba & 0xFFFFFFFF),
    (uint32_t)(startLba >> 32),
    (uint32_t)0x2000000 | blockCount,
    0,
    0,
    0
  };
}
NvmeCommand Read(uint16_t nsid, uintptr_t targetBuffer, uint64_t startLba, uint16_t blockCount) {
  return { 
    0x02, 
    0,
    0,
    nsid,
    0,
    0,
    targetBuffer,
    0,
    (uint32_t)(startLba & 0xFFFFFFFF),
    (uint32_t)(startLba >> 32),
    blockCount,
    0,
    0,
    0
  };
}

NvmeDevice::NvmeDevice(uintptr_t confSpaceAddr)
: PciDevice(confSpaceAddr)
, ctrl(conf, PciBars::Bar0)
, aqs(freepage_get_zeroed(), 0x1000, DeviceMemory)
, aqc(freepage_get_zeroed(), 0x1000, DeviceMemory)
, asq((NvmeCommand*)aqs.get())
, acq((NvmeCompletion*)aqc.get())
, nsMap(freepage_get_zeroed(), 0x1000, DeviceMemory)
, ns((IdentifyNamespace*)nsMap.get())
, idMap(freepage_get_zeroed(), 0x1000, DeviceMemory)
, id((IdentifyController*)idMap.get())
{
  ctrl->cc = 0;
  // TODO: wait for it to come back up
  debug("[NVME] Found NVMe version {}.{}.{}\n", ctrl->vs >> 16, (ctrl->vs >> 8) & 0xFF, ctrl->vs & 0xFF);
  assert((ctrl->cap >> 37) & 0x1);
  doorbellStride = 4 << ((ctrl->cap >> 32) & 0xF);

  ctrl->aqa = 0x1000040;
  ctrl->asq = aqs.to_physical(asq);
  ctrl->acq = aqc.to_physical(acq);

  asqi = 0; 
  acqi = 0;

  RegisterInterruptHandler(0, [this]{ HandleInterrupt(); });
  ctrl->cc = 0x460001;

  start();
}

struct NvmeDisk : Disk {
  NvmeDevice& dev;
  uint16_t nsid, diskid;
  mapping deviceMem;
  IdentifyNamespace* ns;
  NvmeCommand* sq;
  NvmeCompletion* cq;
  uint16_t cqi, sqi;
  s2::vector<s2::pair<uint16_t, s2::promise<uint64_t>>> completions;
  NvmeDisk(NvmeDevice& dev, uint16_t nsid, uint16_t diskid) 
  : dev(dev)
  , nsid(nsid)
  , diskid(diskid)
  , deviceMem(freepage_get_range(6), 0x6000, DeviceMemory)
  , ns((IdentifyNamespace*)deviceMem.get())
  , sq((NvmeCommand*)((uintptr_t)deviceMem.get() + 0x1000))
  , cq((NvmeCompletion*)((uintptr_t)deviceMem.get() + 0x5000))
  {
    dev.RegisterInterruptHandler(diskid, [this]{ HandleInterrupt(); });
    start();
  }
  s2::future<void> start() {
    uint64_t status = co_await dev.RunAdminCommand(Identify(deviceMem.to_physical(ns), 0, 0, nsid, 0, 0));
    (void)status;
    uint32_t lbatype = ns->lbaf[ns->flbas & 0xF];
    if (((lbatype >> 16) > 12) || ((lbatype >> 16) < 9)) {
      debug("[NVME] Found invalid block size {}\n", lbatype >> 16);
    }
    uint32_t sectorSize = 1 << (lbatype >> 16);
    debug("[NVME] LBA block size {}\n", 1 << (lbatype >> 16));
    debug("[NVME] Namespace of {} size with {} capacity, {} in use\n", sectorSize * ns->nsze, sectorSize * ns->ncap, sectorSize * ns->nuse);
    co_await dev.RunAdminCommand(CreateIOCQ(deviceMem.to_physical(cq), 0x100, diskid, diskid));
    co_await dev.RunAdminCommand(CreateIOSQ(deviceMem.to_physical(sq), 0x100, diskid, diskid, 3, 0));
    RegisterDisk(this);
  }
private:
  s2::future<uint64_t> RunCommand(NvmeCommand cmd) {
    uint16_t currentCommand = sqi++;
    if (sqi == 0x200) sqi = 0;
    cmd.cid = currentCommand;
    sq[currentCommand % 0x100] = cmd;
    s2::promise<uint64_t> p;
    s2::future<uint64_t> f = p.get_future();
    completions.push_back({currentCommand, s2::move(p)});
    dev.RingDoorbell(nsid*2, sqi);
    return f;
  }
  // TODO: large reads and writes in multiple commands
  s2::future<IoMemory> read(uint64_t startblock, uint32_t blockCount) override {
    IoMemory buffer(blockCount);
    co_await RunCommand(Read(nsid, buffer.hwaddress(), startblock, blockCount));
    co_return s2::move(buffer);
  }
  s2::future<void> write(uint64_t startblock, uint32_t blockCount, IoMemory& buffer) override {
    co_await RunCommand(Write(nsid, buffer.hwaddress(), startblock, blockCount));
  }
  s2::future<void> trim(uint64_t startblock, uint32_t blockCount) override {
    co_await RunCommand(Trim(nsid, startblock, blockCount));
  }
  s2::future<void> flush() override {
    co_await RunCommand(Flush(nsid));
  }
  void HandleInterrupt() {
    while (true) {
      uint8_t i = cqi % 0x100;
      bool phase = not (cqi & 0x100);
      if ((cq[i].status & 1) != phase) {
        dev.RingDoorbell(nsid*2+1, cqi);
        return;
      }
      for (auto& [id, p] : completions) {
        if (id == cq[i].cmdid) {
          p.set_value((cq[i].dw0 << 16) | (cq[i].status & 0xFFFE));
        }
      }
      cqi++;
    }
  }
};

s2::future<void> NvmeDevice::start() {
  auto ctrlRes = RunAdminCommand(Identify(idMap.to_physical(id), 1, 0, 0, 0, 0));
  uint64_t status = co_await RunAdminCommand(Identify(nsMap.to_physical(ns), 2, 0, 0, 0, 0));
  (void)status;
  uint32_t* nsids = (uint32_t*)ns;
  while (*nsids != 0) nsids++;
  s2::vector<uint32_t> validNsids((uint32_t*)ns, nsids);
  assert(validNsids.size() == 1);
  assert(validNsids.back() == 1);

  for (auto& nsid : validNsids) {
    disks.push_back(new NvmeDisk(*this, nsid, disks.size() + 1));
  }
  uint64_t ctrlStatus = co_await ctrlRes;
  (void)ctrlStatus;
  const char* snstart = id->serialNum, *snend = id->modelNo;
  const char* mstart = id->modelNo, *mend = id->firmwareRev;
  const char* fwstart = id->firmwareRev, *fwend = id->firmwareRev + 8;
  while (snstart != snend && snend[-1] == ' ') snend--;
  while (mstart != mend && mend[-1] == ' ') mend--;
  while (fwstart != fwend && fwend[-1] == ' ') fwend--;
  s2::string sn(snstart, snend);
  s2::string model(mstart, mend);
  s2::string fw(fwstart, fwend);
  debug("[NVME] Found {s} version {s} (SN# {s})\n", model, fw, sn);
}

void NvmeDevice::RingDoorbell(uint32_t doorbell, uint16_t index) {
  uint32_t* p = (uint32_t*)((uintptr_t)ctrl.get() + 0x1000 + doorbellStride * doorbell);
  *p = index;
}

s2::future<uint64_t> NvmeDevice::RunAdminCommand(NvmeCommand cmd) {
  uint16_t currentCommand = asqi++;
  if (asqi == 0x80) asqi = 0;
  cmd.cid = currentCommand;
  asq[currentCommand % 0x40] = cmd;
  s2::promise<uint64_t> p;
  s2::future<uint64_t> f = p.get_future();
  completions.push_back({currentCommand, s2::move(p)});
  RingDoorbell(0, asqi);
  return f;
}

void NvmeDevice::HandleInterrupt() {
  while (true) {
    uint8_t i = acqi % 0x100;
    bool phase = not (acqi & 0x100);
    if ((acq[i].status & 1) != phase) {
      return;
    }
    for (auto& c : completions) {
      if (c.id == acq[i].cmdid) {
        c.p.set_value((acq[i].dw0 << 16) | (acq[i].status & 0xFFFE));
      }
    }
    acqi++;
  }
}


