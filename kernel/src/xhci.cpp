#include "xhci.h"
#include "pci.h"
#include "debug.h"
#include "io.h"

constexpr uint64_t CR_CAPLENGTH = 0;
constexpr uint64_t CR_HCIVERSION = 2;
constexpr uint64_t CR_HCSPARAMS1 = 4;
constexpr uint64_t CR_HCSPARAMS2 = 8;
constexpr uint64_t CR_HCSPARAMS3 = 12;
constexpr uint64_t CR_HCCPARAMS1 = 16;
constexpr uint64_t CR_DBOFF = 20;
constexpr uint64_t CR_RTSOFF = 24;
constexpr uint64_t CR_HCCPARAMS2 = 28;

constexpr uint64_t RT_USBCMD = 0x00;
constexpr uint64_t RT_USBSTS = 0x04;
constexpr uint64_t RT_PAGESIZE = 0x08;
constexpr uint64_t RT_DNCTRL = 0x14;
constexpr uint64_t RT_CRCR = 0x18;
constexpr uint64_t RT_DCBAAP = 0x30;
constexpr uint64_t RT_CONFIG = 0x38;

constexpr uint64_t P_SC = 0x00;
constexpr uint64_t P_PMSC = 0x04;
constexpr uint64_t P_LI = 0x08;

constexpr uint64_t RT_USBSTS_HCH = 0x1;
constexpr uint64_t RT_USBCMD_RUN = 0x00000001;
constexpr uint64_t RT_USBCMD_RESET = 0x00000002;

/*
void disable_efi() {
  uint32_t legacy = findExtendedCap(XHCI_ECAP_LEGSUP);
  if (legacy != 0)
  {
    uint32_t legreg = readCapabilityRegister(legacy, 32);
    legreg |= (1 << 24);
    writeCapabilityRegister(legacy, legreg, 32);
    while (1)
    {
      legreg = readCapabilityRegister(legacy, 32);
      if (((legreg & (1 << 24)) != 0) && ((legreg & (1 << 16)) == 0))
        break;
    }
    kprintf(u"Taken control of XHCI from firmware\n");
  }
  //Disable SMIs
  uint32_t legctlsts = readCapabilityRegister(legacy + 4, 32);
  legctlsts &= XHCI_LEGCTLSTS_DISABLE_SMI;
  legctlsts |= XHCI_LEGCTLSTS_EVENTS_SMI;
  writeCapabilityRegister(legacy + 4, legctlsts, 32);
}
*/

struct xhci_speed {
  void debugprint() {
    if (spd_mantissa == 0) {
      debug("<reserved>");
    } else {
      debug("{} {} ", spd_mantissa, (spd_exponent == '0' ? "bps" : (spd_exponent == '1' ? "Kbps" : (spd_exponent == '2' ? "Mbps" : "Gbps"))));
      if (fullduplex) debug("full duplex ");
    }
  }
  bool fullduplex;
  uint8_t spd_exponent;
  uint16_t spd_mantissa;
  uint8_t linkprot = 0;
  uint8_t symmetric = 0;
  uint8_t slottype = 0;
};

xhci_speed vtbl[16] = {
  {},
  { false, 2, 12 },
  { false, 1, 1500 },
  { false, 2, 480 },
  { true, 3, 5 },
  { true, 3, 10 },
  { true, 3, 10 },
  { true, 3, 20 },
};

static constexpr const char* plstab[16] = {
  "U0",
  "U1",
  "U2",
  "U3",
  "Disabled",
  "RxDetect",
  "Inactive",
  "Polling",
  "Recovery",
  "Hot Reset",
  "Compliance Mode",
  "Test Mode",
  "Reserved 12",
  "Reserved 13",
  "Reserved 14",
  "Resume",
};

void extractPsids(pcidevice dev) {

}

XhciDevice::XhciDevice(pcidevice dev)
: dev(dev)
{
  uint8_t version = pciread8(dev, 0x60);
  uint8_t major = (version >> 4), minor = version & 0xF;
  uint64_t ptr = (((uint64_t)pciread32(dev, 0x14) << 32) | pciread32(dev, 0x10)) & 0xFFFFFFFFFFFFFFF8ULL;
  debug("xhci for version {}.{} found at {x}\n", major, minor, ptr);

  cr = ptr;
  uint64_t opregs = ptr + (mmio_read<uint32_t>(cr + CR_CAPLENGTH) & 0xFF);
  rr = ptr + mmio_read<uint32_t>(cr + CR_RTSOFF);
  doorbell = ptr + mmio_read<uint32_t>(cr + CR_DBOFF);

  uint32_t hcsparams1 = mmio_read<uint32_t>(cr + CR_HCSPARAMS1);
  uint32_t maxports = (hcsparams1 >> 24) & 0xFF;
  uint32_t maxinterrupters = (hcsparams1 >> 8) & 0x7FF;
  uint32_t maxslots = (hcsparams1 >> 0) & 0xFF;
  debug("maxports{} maxints{} maxslots{}\n", maxports, maxinterrupters, maxslots);

  uint32_t xecp = (mmio_read<uint32_t>(cr + CR_HCCPARAMS1) >> 16) << 2;

  while (xecp) {
    uint32_t hv = mmio_read<uint32_t>(cr + xecp);
    uint8_t id = hv & 0xFF;
    uint8_t ne = (hv >> 8) & 0xFF;
    debug("id={}\n", id);
    if (ne == 0) break;
    xecp += ne * 4;
  }
  pciwrite16(dev, 0x04, pciread16(dev, 0x04) | 6);

/*
  hc_constructRootPorts(&x->hc, BYTE4(x->CapRegs->hcsparams1), &USB_XHCI);
  printf("\nx->hc.rootPortCount: %u", x->hc.rootPortCount);
*/

  mmio_write<uint32_t>(opregs + RT_USBCMD, mmio_read<uint32_t>(opregs + RT_USBCMD) & ~RT_USBCMD_RUN);
  while ((mmio_read<uint32_t>(opregs + RT_USBSTS) & RT_USBSTS_HCH) == 0) {}

  mmio_write<uint32_t>(opregs + RT_USBCMD, mmio_read<uint32_t>(opregs + RT_USBCMD) | RT_USBCMD_RESET);
  // TODO: sleep(1us)
  while ((mmio_read<uint32_t>(opregs + RT_USBCMD) & RT_USBCMD_RESET) != 0) {}

  mmio_write<uint32_t>(opregs + RT_CONFIG, mmio_read<uint32_t>(opregs + RT_CONFIG) | maxslots);

  for (size_t n = 0; n < maxports; n++) {
    uint32_t sc = mmio_read<uint32_t>(opregs + 0x400 + n * 16 + P_SC);
    uint32_t pmsc = mmio_read<uint32_t>(opregs + 0x400 + n * 16 + P_PMSC);
    uint32_t li = mmio_read<uint32_t>(opregs + 0x400 + n * 16 + P_LI);
    debug("port {} {} {} ", n, (sc & 1 ? "connected" : "no device"), sc & 2 ? "enabled " : "disabled");
    debug("{}\n", plstab[(sc >> 5) & 0xF]);
    vtbl[(sc >> 10) & 0xF].debugprint();
    debug("port {} {8x} {8x} {8x}\n", n, sc, pmsc, li);
  }

/*
  deactivateLegacySupport(x);

  x->OpRegs->devnotifctrl = 0x2;

  // Program the Device Context Base Address Array Pointer (DCBAAP) register (5.4.6) with a 64-bit address pointing to where the Device Context Base Address Array is located.
  x->virt_deviceContextPointerArrayBase = malloc(sizeof(xhci_DeviceContextArray_t), 64 | HEAP_CONTINUOUS, "xhci_DevContextArray"); // Heap is contiguous below 4K. Alignment see Table 54
  x->OpRegs->dcbaap = (uint64_t)paging_getPhysAddr(x->virt_deviceContextPointerArrayBase);

  uint8_t MaxScratchpadBuffers = ((x->CapRegs->hcsparams2 >> 27) & 0x1F) | ((x->CapRegs->hcsparams2 >> 16) & 0xE0);
  if (MaxScratchpadBuffers > 0) // Max Scratchpad Buffers
  {
    printf("\nscratchpad buffer created! Max Scratchpad Buffers = %u", MaxScratchpadBuffers);
    uint64_t* ScratchpadBuffersPtr = malloc(sizeof(uint64_t)*MaxScratchpadBuffers, 64 | HEAP_CONTINUOUS, "xhci_ScratchpadBuffersPtr");
    for (uint8_t i=0; i<MaxScratchpadBuffers; i++)
    {
      ScratchpadBuffersPtr[i] = paging_getPhysAddr(malloc(PAGESIZE, PAGESIZE | HEAP_CONTINUOUS, "xhci_ScratchpadBuffer"));
    }
    x->virt_deviceContextPointerArrayBase->scratchpadBufferArrBase = (uint64_t)paging_getPhysAddr(ScratchpadBuffersPtr); // Ptr to scratchpad buffer array
  }
  else // Max Scratchpad Buffers = 0
  {
    x->virt_deviceContextPointerArrayBase->scratchpadBufferArrBase = 0;
  }

  // Device Contexts
  for (uint16_t i=0; i<MAX_HC_SLOTS; i++)
  {
    x->devContextPtr[i] = malloc(sizeof(xhci_DeviceContext_t), 64 | HEAP_CONTINUOUS, "xhci_DevContext"); // Alignment see Table 54
    memset(x->devContextPtr[i], 0, sizeof(xhci_DeviceContext_t));
    x->virt_deviceContextPointerArrayBase->devContextPtr[i] = (uintptr_t)paging_getPhysAddr(x->devContextPtr[i]);
  }

  // Input Device Contexts
  for (uint16_t i=0; i<MAX_HC_SLOTS; i++)
  {
    x->devInputContextPtr[i] = malloc(sizeof(xhci_InputContext_t), 64 | HEAP_CONTINUOUS, "xhci_DevInputContext"); // Alignment see Table 54
    memset(x->devInputContextPtr[i], 0, sizeof(xhci_InputContext_t));
  }

  // Transfer Rings
  for (uint16_t slotNr=1; slotNr<=MAX_HC_SLOTS; slotNr++)
  {
    x->slots[slotNr-1] = malloc(sizeof(xhci_slot_t), 0, "xhci_slots");

    for (uint16_t i=0; i<NUM_ENDPOINTS; i++)
    {
      xhci_xfer_NormalTRB_t* trb = malloc(256*sizeof(xhci_xfer_NormalTRB_t), PAGESIZE | HEAP_CONTINUOUS, "xhci_transferTRB"); // Alignment see Table 54. Use PAGESIZE to ensure that the memory is within one page.
      memset(trb, 0, 256 * sizeof(xhci_xfer_NormalTRB_t));
      x->trb[slotNr-1][i] = trb;

      // Software uses and maintains private copies of the Enqueue and Dequeue Pointers for each Transfer Ring.
      x->slots[slotNr-1]->endpoints[i].virtEnqTransferRingPtr = x->slots[slotNr-1]->endpoints[i].virtDeqTransferRingPtr = trb;
      x->slots[slotNr-1]->endpoints[i].TransferRingProducerCycleState = true; // PCS
      x->slots[slotNr-1]->endpoints[i].TransferCounter = 0; // Reset Transfer Counter
      x->slots[slotNr-1]->endpoints[i].timeEvent = 0;
      x->slots[slotNr-1]->endpoints[i].TransferRingbase = trb;

      // LinkTRB
      xhci_LinkTRB_t* linkTrb = (xhci_LinkTRB_t*)(trb + 255);
      linkTrb->RingSegmentPtrLo = paging_getPhysAddr(trb); // segment pointer
      linkTrb->IntTarget = 0;                // intr target
      linkTrb->TC = 1;                   // Toggle Cycle !! 4.11.5.1 Link TRB
      linkTrb->TRBtype = TRB_TYPE_LINK;          // ID of link TRB, cf. table 131
    }
  }

  // Command Ring
  // Define the Command Ring Dequeue Pointer by programming the Command Ring Control Register (5.4.5) with a 64-bit address pointing to the starting address of the first TRB of the Command Ring.
  x->CmdRingbase = malloc(256*sizeof(xhci_LinkTRB_t), 64 | HEAP_CONTINUOUS, "xhci_cmdTRB"); // Alignment see Table 54
  x->virtEnqCmdRingPtr = x->CmdRingbase;
  memset(x->CmdRingbase, 0, 256*sizeof(xhci_LinkTRB_t));

  // LinkTRB
  x->CmdRingbase[255].RingSegmentPtrLo = paging_getPhysAddr(x->CmdRingbase); // segment pointer
  x->CmdRingbase[255].TC = 1;                        // Toggle Cycle
  x->CmdRingbase[255].TRBtype = TRB_TYPE_LINK;                 // ID of link TRB, cf. table 131

  // Command Ring Control Register (5.4.5), "crcr"
  x->CmdRingProducerCycleState = true; // PCS
  x->OpRegs->crcr = paging_getPhysAddr(x->CmdRingbase) | x->CmdRingProducerCycleState; // command ring control register, Table 32, 5.4.5, CCS is Bit 0. Bit5:4 are RsvdP, but cannot be read.

  xhci_configureInterrupts(x);
  xhci_prepareEventRing(x);

  // Write the USBCMD (5.4.1) to turn the host controller ON via setting the Run/Stop (R/S) bit to ���� This operation allows the xHC to begin accepting doorbell references.
  x->OpRegs->command |= CMD_RUN; // set Run/Stop bit
  sleepMilliSeconds(100); //  IMPORTANT

  xhci_showStatus(x);

  if (!(x->OpRegs->status & STS_HCH)) // HC not Halted
  {
    xhci_prepareSlotsForControlTransfers(x);
  }
  else // HC Halted
  {
    printfe("\nFatal Error: HCHalted set. Ports cannot be enabled.");
    xhci_showStatus(x);
  }
}





static void xhci_configureInterrupts(xhci_t* x)
{
    // MSI (We do not support MSI-X)
    x->msiCapEnabled = pci_trySetMSIVector(x->PCIdevice, APICIRQ);

    if (x->msiCapEnabled)
        x->irq = APICIRQ;
    else
        x->irq = x->PCIdevice->irq;
    irq_installPCIHandler(x->irq, xhci_handler, x->PCIdevice);
*/
}

