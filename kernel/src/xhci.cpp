#include "xhci.h"
#include "pci.h"
#include "debug.h"
#include "io.h"
#include "map.h"
#include <cstring>
#include "freepage.h"

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

uint64_t XhciDevice::CreateContext(uint32_t scratchcount) {
  uint64_t page = freepage_get();
  mapping dcb(page, 0x1000, DeviceMemory);
  memset(dcb.get(), 0, 0x1000);

  uint64_t scratchpad_index = freepage_get();
  mmio_write<uint64_t>((uintptr_t)dcb.get(), scratchpad_index);

  mapping index(scratchpad_index, 0x1000, DeviceMemory);
  memset(index.get(), 0, 0x1000);
  uint64_t* scratchpads = (uint64_t*)index.get();
  for (size_t n = 0; n < scratchcount; n++) {
    scratchpads[n] = freepage_get_zeroed();
  }
  return page;
}

XhciDevice::XhciDevice(pcidevice dev)
: dev(dev)
{
  uint8_t version = pciread8(dev, 0x60);
  uint8_t major = (version >> 4), minor = version & 0xF;
  uint64_t ptr = (((uint64_t)pciread32(dev, 0x14) << 32) | pciread32(dev, 0x10)) & 0xFFFFFFFFFFFFFFF8ULL;
  debug("xhci for version {}.{} found at {x}\n", major, minor, ptr);

  mapping bar1(ptr, 0x10000, DeviceRegisters);

  cr = (uintptr_t)bar1.get();
  uint64_t opregs = cr + (mmio_read<uint32_t>(cr + CR_CAPLENGTH) & 0xFF);
  rr = cr + mmio_read<uint32_t>(cr + CR_RTSOFF);
  doorbell = cr + mmio_read<uint32_t>(cr + CR_DBOFF);

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

  mmio_write<uint32_t>(opregs + RT_USBCMD, mmio_read<uint32_t>(opregs + RT_USBCMD) & ~RT_USBCMD_RUN);
  while ((mmio_read<uint32_t>(opregs + RT_USBSTS) & RT_USBSTS_HCH) == 0) {}

  mmio_write<uint32_t>(opregs + RT_USBCMD, mmio_read<uint32_t>(opregs + RT_USBCMD) | RT_USBCMD_RESET);
  // TODO: sleep(1us)
  while ((mmio_read<uint32_t>(opregs + RT_USBCMD) & RT_USBCMD_RESET) != 0) {}


  for (size_t n = 0; n < maxports; n++) {
    uint32_t sc = mmio_read<uint32_t>(opregs + 0x400 + n * 16 + P_SC);
    uint32_t pmsc = mmio_read<uint32_t>(opregs + 0x400 + n * 16 + P_PMSC);
    uint32_t li = mmio_read<uint32_t>(opregs + 0x400 + n * 16 + P_LI);
    debug("port {} {} {} ", n, (sc & 1 ? "connected" : "no device"), sc & 2 ? "enabled " : "disabled");
    debug("{}\n", plstab[(sc >> 5) & 0xF]);
    vtbl[(sc >> 10) & 0xF].debugprint();
    debug("port {} {8x} {8x} {8x}\n", n, sc, pmsc, li);
  }

  mmio_write<uint32_t>(opregs + RT_DNCTRL, 0x02);

  uint32_t hcsparams2 = mmio_read<uint32_t>(cr + CR_HCSPARAMS2);
  uint32_t scratchcount = ((hcsparams2 >> 27) & 0x1F) | ((hcsparams2 >> 16) & 0x3E0);
  if (scratchcount > 512)
    debug("XHCI: requires more scratchpads than fit the index?\n");
  uint64_t dcbPage = CreateContext(scratchcount);
  mmio_write<uint64_t>(opregs + RT_DCBAAP, dcbPage);
  mmio_write<uint32_t>(opregs + RT_CONFIG, mmio_read<uint32_t>(opregs + RT_CONFIG) | maxslots);

  uint64_t cr_page = freepage_get_zeroed();
  mapping cr(cr_page, 0x1000, DeviceMemory); // or registers?
  
  uint64_t primaryEventRing = freepage_get_zeroed();
  mapping pev(primaryEventRing, 0x1000, DeviceMemory);

  mmio_write<uint64_t>(opregs + RT_CRCR, cr_page + 1);

  uint64_t evtSeg = freepage_get_zeroed();
  mapping pev(evtSeg, 0x1000, DeviceMemory);


  uint64_t primaryEventRing = freepage_get_zeroed();
  mapping pev(primaryEventRing, 0x1000, DeviceMemory);

  paddr_t evtseg = pmmngr_allocate(1);
  void* evtringseg = find_free_paging(PAGESIZE);
  paging_map(evtringseg, evtseg, PAGESIZE, PAGE_ATTRIBUTE_WRITABLE | PAGE_ATTRIBUTE_NO_CACHING);
  memset(evtringseg, 0, PAGESIZE);
  //Create one event ring segment
  paddr_t evt = pmmngr_allocate(1);
  void* evtring = find_free_paging(PAGESIZE);
  void* evtdp = evtring;
  paging_map(evtring, evt, PAGESIZE, PAGE_ATTRIBUTE_WRITABLE | PAGE_ATTRIBUTE_NO_CACHING);
  memset(evtring, 0, PAGESIZE);
  //Write it to the segment table
  *raw_offset<volatile uint64_t*>(evtringseg, 0) = evt;
  *raw_offset<volatile uint64_t*>(evtringseg, 8) = PAGESIZE / 0x10;
  //Write the event ring info
  primaryevt.dequeueptr = evtdp;
  primaryevt.ringbase = evtring;
  //Set segment table size
  Runtime.Interrupter(0).ERSTSZ.Size = 1;
  //Set dequeue pointer
  Runtime.Interrupter(0).ERDP.update(evt, false);
  //Enable event ring
  Runtime.Interrupter(0).ERSTBA.SegTableBase = evtseg;
  Runtime.Interrupter(0).IMAN = Runtime.Interrupter(0).IMAN;


  mmio_write<uint32_t>(opregs + RT_USBCMD, mmio_read<uint32_t>(opregs + RT_USBCMD) | RT_USBCMD_RUN);

    /*
		Runtime.Interrupter(0).IMAN.InterruptEnable = 1;
		Operational.USBCMD.INTE = 1;
		//Just make sure we don't miss interrupts
		Runtime.Interrupter(0).IMAN = Runtime.Interrupter(0).IMAN;
		Runtime.Interrupter(0).IMOD.InterruptInterval = 4000;
		Operational.USBSTS = Operational.USBSTS;
		//Work on ports
		kprintf(u"XHCI controller enabled, %d ports\n", getMaxPorts());
		for (size_t n = 1; n <= getMaxPorts(); ++n)
		{
			xhci_thread_port_startup* sup = new xhci_thread_port_startup;
			sup->cinfo = this;
			sup->port = n;
			//create_thread(&xhci_port_startup, sup);
			xhci_port_startup(sup);
		}
    */
/*
  // Write the USBCMD (5.4.1) to turn the host controller ON via setting the Run/Stop (R/S) bit to ���� This operation allows the xHC to begin accepting doorbell references.
  x->OpRegs->command |= CMD_RUN; // set Run/Stop bit
  sleepMilliSeconds(100); //  IMPORTANT
*/
}

