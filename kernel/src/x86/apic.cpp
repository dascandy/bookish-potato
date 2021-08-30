#include "apic.h"
#include "io.h"
#include "map.h"
#include "cpuid.h"
#include <cassert>
#include "debug.h"
#include <vector>
#include <functional>
#include "interrupt.h"

#ifdef __x86_64__

namespace Apic 
{
  static const uint32_t IA32_APIC_BASE = 0x1B;
  static bool useMsr = false;
  static mapping mmio;

  enum {
    LocalAPIC_ID = 2,
    LocalAPIC_Version = 3,
    TPR = 8,
    PPR = 0x0A,
    EOI = 0x0B,
    LDR = 0x0D,
    Spurious = 0x0F,
    ISRBASE = 0x10,
    TMRBASE = 0x18,
    IRRBASE = 0x20,
    ESR = 0x28,
    ICR = 0x30,
    ICRHigh = 0x31,
    LVT_Timer = 0x32,
    LVT_Thermal = 0x33,
    LVT_Perfmon = 0x34,
    LVT_LINT0 = 0x35,
    LVT_LINT1 = 0x36,
    LVT_Error = 0x37,
    TimerInitialCount = 0x38,
    TimerCurrentCount = 0x39,
    TimerDivideConfig = 0x3E,
    SelfIPI = 0x3F,
  };

  static bool isX2() {
    return (cpuid(1).ecx & 0x200000) == 0x200000;
  }

  void init() {
    if (isX2()) {
      useMsr = true;
      wrmsr(IA32_APIC_BASE, rdmsr(IA32_APIC_BASE) | 0xC00);
    } else {
      useMsr = false;
      mmio = mapping(0xFEE00000, 0x1000, DeviceRegisters);
    }

    write(Spurious, 0x130);
  }

  void write(uint8_t reg, uint64_t value)
  {
    if (useMsr) {
      wrmsr(0x800 + reg, value);
    } else {
      mmio_write<uint32_t>((uintptr_t)mmio.get() + reg * 0x10, value);
    }
  }

  uint64_t read(uint8_t reg)
  {
    if (useMsr) {
      return rdmsr(0x800 + reg);
    } else {
      return mmio_read<uint32_t>((uintptr_t)mmio.get() + reg * 0x10);
    }
  }

  struct MadtHeader {
    uint32_t lapicAddr;
    uint32_t flags;
  };
  enum class MadtType : uint8_t {
    Lapic = 0,
    Ioapic = 1,
    IoapicIntSource = 2,
    IoapicNmi = 3,
    LapicNmi = 4,
    LapicAddr = 5,
    X2apic = 9,
  };
  struct MadtEntry {
    MadtType entrytype;
    uint8_t recordLength;
  };
  struct MadtLapic {
    uint8_t entrytype;
    uint8_t recordLength;
    uint8_t acpiId;
    uint8_t apicId;
    uint32_t flags;
  };
  struct MadtX2Apic {
    uint8_t entrytype;
    uint8_t recordLength;
    uint8_t pad[2];
    uint32_t apicId;
    uint32_t flags;
    uint32_t acpiId;
  };
  struct MadtIoapic {
    uint8_t entrytype;
    uint8_t recordLength;
    uint8_t ioapicId;
    uint8_t pad;
    uint32_t ioapicAddr;
    uint32_t gsib;
  };
  struct MadtIoapicEntry {
    uint8_t entrytype;
    uint8_t recordLength;
    uint8_t busSource;
    uint8_t irqSource;
    uint32_t gsi;
    uint16_t flags;
  };

  struct IntOverride {
    uint8_t irqSource;
    uint16_t flags;
    uint32_t gsi;
  };

  struct Ioapic {
    mapping map;
    uint32_t gsiBase;
    Ioapic(uint64_t addr, uint32_t gsiBase)
    : map(addr, 0x1000, DeviceRegisters)
    , gsiBase(gsiBase)
    {}
    void setInterrupt(IntOverride* ov, uint32_t cpuId, uint32_t gsi, uint8_t vector) {
      uint64_t value = ((uint64_t)cpuId << 56) | vector;
      if (ov) {
        gsi = ov->gsi;
        value |= (ov->flags & 0xA) << 12;
      }
      uint32_t entryId = gsi - gsiBase;
      mmio_write<uint32_t>((uintptr_t)map.get(), 0x10 + entryId * 2);
      mmio_write<uint32_t>((uintptr_t)map.get() + 0x10, (uint32_t)value);
      mmio_write<uint32_t>((uintptr_t)map.get(), 0x11 + entryId * 2);
      mmio_write<uint32_t>((uintptr_t)map.get() + 0x10, (value >> 32));
    }
  };

  static s2::vector<IntOverride> overrides;
  static s2::vector<Ioapic> ioapics;

  void addSecondaryCpu(uint32_t apicId, uint32_t acpiId) {
    debug("[APIC] Found CPU APIC {} ACPI {}\n", apicId, acpiId);
/*

    uint64_t icrvalue = 0x600;
    if (useMsr) {
      IcrValue |= (apicId << 32);
      Apic::write(Apic::ICR, IcrValue);
    } else {
      IcrValue |= (apicId << 56);
      Apic::write(Apic::ICRHigh, (uint32_t)(IcrValue >> 32));
      Apic::write(Apic::ICR, (uint32_t)IcrValue);
    }
*/
  }
  
  void HandleMadt(s2::span<const uint8_t> madt) {
    const MadtEntry* start = (const MadtEntry*)(madt.data() + sizeof(MadtHeader));
    const MadtEntry* end = (const MadtEntry*)(madt.data() + madt.size());

    for (; start < end; start = (const MadtEntry*)((const uint8_t*)start + start->recordLength)) {
      switch(start->entrytype) {
        case MadtType::Lapic:
        {
          const MadtLapic* lapic = (const MadtLapic*)start;
          if (lapic->flags == 0) addSecondaryCpu(lapic->apicId, lapic->acpiId);
        }
          break;
        case MadtType::X2apic:
        {
          const MadtX2Apic* x2apic = (const MadtX2Apic*)start;
          if (x2apic->flags == 0) addSecondaryCpu(x2apic->apicId, x2apic->acpiId);
        }
          break;
        case MadtType::Ioapic:
        {
          const MadtIoapic* ioapic = (const MadtIoapic*)start;
          debug("[APIC] ioapic {} addr {x} gsib {}\n", ioapic->ioapicId, ioapic->ioapicAddr, ioapic->gsib);
          ioapics.push_back({ioapic->ioapicAddr, ioapic->gsib});
        }
          break;
        case MadtType::IoapicIntSource:
        {
          const MadtIoapicEntry* entry = (const MadtIoapicEntry*)start;
          debug("[APIC] ioapicentry bus {} irqs {} gsi {} flags {x}\n", entry->busSource, entry->irqSource, entry->gsi, entry->flags);
          overrides.push_back({entry->irqSource, entry->flags, entry->gsi});
        }
          break;
        case MadtType::IoapicNmi:
        case MadtType::LapicNmi:
        case MadtType::LapicAddr:
          debug("[APIC] Found unused record {}, skipping\n", (uint32_t)start->entrytype);
          break;
      }
    }
  }

  void start_secondary_cpus() {
  }
}

void SetIsaInterrupt(size_t isaInterrupt, s2::function<void()> handler) {
  uint32_t vector = get_empty_interrupt_vector();
  debug("[APIC] Setting ISA interrupt {} to vector {}\n", isaInterrupt, vector);
  Apic::Ioapic* apic = nullptr;
  Apic::IntOverride* ov = nullptr;
  uint32_t gsi = isaInterrupt;
  for (auto& o : Apic::overrides) {
    if (o.irqSource == isaInterrupt) {
      gsi = o.gsi;
    }
  }
  for (auto& a : Apic::ioapics) {
    if (isaInterrupt > a.gsiBase &&
        (apic == nullptr || (apic->gsiBase < a.gsiBase))) {
      apic = &a;
    }
  }

  // TODO: get local Apic ID
  apic->setInterrupt(ov, 0, gsi, vector);
  interrupt_register(vector, s2::move(handler));
}

void plat_endofinterrupt() {
  Apic::write(Apic::EOI, 0);
}

#endif


