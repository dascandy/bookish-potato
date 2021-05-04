#include "model.h"
#include "mailbox.h"

#ifdef __aarch64__
static rpi_entry models[] = {
  0x0002, "B", "1.0", 256, "Egoman", 0x20000000,
  0x0003, "B", "1.0", 256, "Egoman", 0x20000000,
  0x0004, "B", "2.0", 256, "Sony UK", 0x20000000,
  0x0005, "B", "2.0", 256, "Qisda", 0x20000000,
  0x0006, "B", "2.0", 256, "Egoman", 0x20000000,
  0x0007, "A", "2.0", 256, "Egoman", 0x20000000,
  0x0008, "A", "2.0", 256, "Sony UK", 0x20000000,
  0x0009, "A", "2.0", 256, "Qisda", 0x20000000,
  0x000d, "B", "2.0", 512, "Egoman", 0x20000000,
  0x000e, "B", "2.0", 512, "Sony UK", 0x20000000,
  0x000f, "B", "2.0", 512, "Egoman", 0x20000000,
  0x0010, "B+", "1.2", 512, "Sony UK", 0x20000000,
  0x0011, "CM1", "1.0", 512, "Sony UK", 0x20000000,
  0x0012, "A+", "1.1", 256, "Sony UK", 0x20000000,
  0x0013, "B+", "1.2", 512, "Embest", 0x20000000,
  0x0014, "CM1", "1.0", 512, "Embest", 0x20000000,
  0x0015, "A+", "1.1", 256, "Embest", 0x20000000,
  0x900021, "A+", "1.1", 512, "Sony UK", 0x20000000,
  0x900032, "B+", "1.2", 512, "Sony UK", 0x20000000,
  0x900061, "CM", "1.1", 512, "Sony UK", 0x20000000,
  0x900092, "Zero", "1.2", 512, "Sony UK", 0x20000000,
  0x900093, "Zero", "1.3", 512, "Sony UK", 0x20000000,
  0x9000c1, "Zero W", "1.1", 512, "Sony UK", 0x20000000,
  0x9020e0, "3A+", "1.0", 512, "Sony UK", 0x3F000000,
  0x920092, "Zero", "1.2", 512, "Embest", 0x20000000,
  0x920093, "Zero", "1.3", 512, "Embest", 0x20000000,
  0xa01040, "2B", "1.0", 1024, "Sony UK", 0x3F000000,
  0xa01041, "2B", "1.1", 1024, "Sony UK", 0x3F000000,
  0xa02042, "2B (with BCM2837)", "1.2", 1024, "Sony UK", 0x3F000000,
  0xa02082, "3B", "1.2", 1024, "Sony UK", 0x3F000000,
  0xa020a0, "CM3", "1.0", 1024, "Sony UK", 0x3F000000,
  0xa020d3, "3B+", "1.3", 1024, "Sony UK", 0x3F000000,
  0xa02100, "CM3+", "1.0", 1024, "Sony UK", 0x3F000000,
  0xa03111, "4B", "1.1", 1024, "Sony UK", 0xFE000000,
  0xa21041, "2B", "1.1", 1024, "Embest", 0x3F000000,
  0xa22042, "2B (with BCM2837)", "1.2", 1024, "Embest", 0x3F000000,
  0xa22082, "3B", "1.2", 1024, "Embest", 0x3F000000,
  0xa22083, "3B", "1.3", 1024, "Embest", 0x3F000000,
  0xa220a0, "CM3", "1.0", 1024, "Embest", 0x3F000000,
  0xa32082, "3B", "1.2", 1024, "Sony Japan", 0x3F000000,
  0xa52082, "3B", "1.2", 1024, "Stadium", 0x3F000000,
  0xb03111, "4B", "1.1", 2048, "Sony UK", 0xFE000000,
  0xb03112, "4B", "1.2", 2048, "Sony UK", 0xFE000000,
  0xc03111, "4B", "1.1", 4096, "Sony UK", 0xFE000000,
  0xc03112, "4B", "1.2", 4096, "Sony UK", 0xFE000000,
};

static uintptr_t get_mmio_base() {
    uint64_t reg;
    asm volatile ("mrs %0, midr_el1" : "=r" (reg));
 
    switch ((reg >> 4) & 0xFFF) {
        case 0xB76: 
          return 0x20000000; 
        case 0xC07: 
        case 0xD03: 
          return 0x3F000000;
        case 0xD08: 
        default: 
          return 0xFE000000;
    }
    return 0x3F000000;
}

rpi_entry getModel() {
  uintptr_t mmiobase = get_mmio_base();
  mailbox_init(mmiobase);
  uint32_t modelno;
  property_read(MailboxProperty::HardwareBoardRevision, (uint8_t*)&modelno, sizeof(modelno));
  for (auto& m : models) {
    if (m.modelno == modelno) return m;
  }
  return { 0xdeadbeef, "Unknown", "?.?", 4096, "??", mmiobase, };
}
#endif


