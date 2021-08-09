#include "interrupt.h"
#include "map.h"
#include <cstdint>
#include "crash.h"
#include "io.h"
#include <cstring>
#include "debug.h"
#include "apic.h"

#ifdef __x86_64__
struct gdt {
  uint16_t limit;
  uint16_t base1;
  uint8_t base2;
  uint8_t access;
  uint8_t limit_flags;
  uint8_t base3;
} gdt_[] = {
  { 0, 0, 0, 0, 0, 0 },
  { 0xFFFF, 0x0000, 0x00, 0x98, 0x2F, 0x00 },
  { 0xFFFF, 0x0000, 0x00, 0xF8, 0x2F, 0x00 },
  { 0xFFFF, 0x0000, 0x00, 0x92, 0x2F, 0x00 },
  { 0xFFFF, 0x0000, 0x00, 0x89, 0x1F, 0x00 }, // this and next are TSS locator
  { 0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00 },
};

struct load {
  uint16_t size;
  uint64_t offset;
} __attribute__((packed));

static load _loadgdt = {
  sizeof(gdt_)-1,
  (uint64_t)gdt_
};

struct tss {
  uint32_t reserve1;
  uint64_t rsp0;
  uint64_t rsp1;
  uint64_t rsp2;
  void* ist[8];
  uint64_t reserve3;
  uint16_t reserve4;
  char iobitmap[1];
} __attribute__ ((packed));
static tss _tss = {};

enum class IST {
  Interrupt = 1,
  Unhandled = 7,
};

void tss_set_ist(IST ist, void* stackptr) {
  _tss.ist[(uint8_t)ist] = stackptr;
}

static struct idt_entry {
  uint16_t offset_low;
  uint16_t selector;
  uint8_t ist;
  uint8_t type_attr;
  uint16_t offset_middle;
  uint32_t offset_high;
  uint32_t zero;
} idt[0x100];

extern void x8664_oninterrupt();
static char stack_for_interrupts[4096] __attribute__((aligned(4096)));
static char stack_for_crash_exceptions[4096] __attribute__((aligned(4096)));

static load _loadidt = {
  sizeof(idt), 
  (uint64_t)idt
};

static void interrupt_set_vector(uint8_t vector, void (*function)(), IST ist) {
  auto& entry = idt[vector];
  uintptr_t func = (uintptr_t)function;
  entry.offset_low = func & 0xFFFF;
  entry.offset_middle = (func >> 16) & 0xFFFF;
  entry.offset_high = (func >> 32);
  entry.ist = (uint8_t)ist;
  entry.selector = 0x08;
  entry.type_attr = 0x8E;
  entry.zero = 0;
}

static void disable_pic() {
  outb(0x20, 0x11);
  outb(0xa0, 0x11);
  outb(0x21, 0x20);
  outb(0xa1, 0x28);
  outb(0x21, 4);
  outb(0xa1, 2);
  outb(0x21, 1);
  outb(0xa1, 1);
  outb(0x21, 0xFF);
  outb(0xa1, 0xFF);
}

void interrupt000();
void interrupt001();
void interrupt002();
void interrupt003();
void interrupt004();
void interrupt005();
void interrupt006();
void interrupt007();
void interrupt008();
void interrupt009();
void interrupt010();
void interrupt011();
void interrupt012();
void interrupt013();
void interrupt014();
void interrupt015();
void interrupt016();
void interrupt017();
void interrupt018();
void interrupt019();
void interrupt020();
void interrupt021();
void interrupt022();
void interrupt023();
void interrupt024();
void interrupt025();
void interrupt026();
void interrupt027();
void interrupt028();
void interrupt029();
void interrupt030();
void interrupt031();
void interrupt032();
void interrupt033();
void interrupt034();
void interrupt035();
void interrupt036();
void interrupt037();
void interrupt038();
void interrupt039();
void interrupt040();
void interrupt041();
void interrupt042();
void interrupt043();
void interrupt044();
void interrupt045();
void interrupt046();
void interrupt047();
void interrupt048();
void interrupt049();
void interrupt050();
void interrupt051();
void interrupt052();
void interrupt053();
void interrupt054();
void interrupt055();
void interrupt056();
void interrupt057();
void interrupt058();
void interrupt059();
void interrupt060();
void interrupt061();
void interrupt062();
void interrupt063();
void interrupt064();
void interrupt065();
void interrupt066();
void interrupt067();
void interrupt068();
void interrupt069();
void interrupt070();
void interrupt071();
void interrupt072();
void interrupt073();
void interrupt074();
void interrupt075();
void interrupt076();
void interrupt077();
void interrupt078();
void interrupt079();
void interrupt080();
void interrupt081();
void interrupt082();
void interrupt083();
void interrupt084();
void interrupt085();
void interrupt086();
void interrupt087();
void interrupt088();
void interrupt089();
void interrupt090();
void interrupt091();
void interrupt092();
void interrupt093();
void interrupt094();
void interrupt095();
void interrupt096();
void interrupt097();
void interrupt098();
void interrupt099();
void interrupt100();
void interrupt101();
void interrupt102();
void interrupt103();
void interrupt104();
void interrupt105();
void interrupt106();
void interrupt107();
void interrupt108();
void interrupt109();
void interrupt110();
void interrupt111();
void interrupt112();
void interrupt113();
void interrupt114();
void interrupt115();
void interrupt116();
void interrupt117();
void interrupt118();
void interrupt119();
void interrupt120();
void interrupt121();
void interrupt122();
void interrupt123();
void interrupt124();
void interrupt125();
void interrupt126();
void interrupt127();
void interrupt128();
void interrupt129();
void interrupt130();
void interrupt131();
void interrupt132();
void interrupt133();
void interrupt134();
void interrupt135();
void interrupt136();
void interrupt137();
void interrupt138();
void interrupt139();
void interrupt140();
void interrupt141();
void interrupt142();
void interrupt143();
void interrupt144();
void interrupt145();
void interrupt146();
void interrupt147();
void interrupt148();
void interrupt149();
void interrupt150();
void interrupt151();
void interrupt152();
void interrupt153();
void interrupt154();
void interrupt155();
void interrupt156();
void interrupt157();
void interrupt158();
void interrupt159();
void interrupt160();
void interrupt161();
void interrupt162();
void interrupt163();
void interrupt164();
void interrupt165();
void interrupt166();
void interrupt167();
void interrupt168();
void interrupt169();
void interrupt170();
void interrupt171();
void interrupt172();
void interrupt173();
void interrupt174();
void interrupt175();
void interrupt176();
void interrupt177();
void interrupt178();
void interrupt179();
void interrupt180();
void interrupt181();
void interrupt182();
void interrupt183();
void interrupt184();
void interrupt185();
void interrupt186();
void interrupt187();
void interrupt188();
void interrupt189();
void interrupt190();
void interrupt191();
void interrupt192();
void interrupt193();
void interrupt194();
void interrupt195();
void interrupt196();
void interrupt197();
void interrupt198();
void interrupt199();
void interrupt200();
void interrupt201();
void interrupt202();
void interrupt203();
void interrupt204();
void interrupt205();
void interrupt206();
void interrupt207();
void interrupt208();
void interrupt209();
void interrupt210();
void interrupt211();
void interrupt212();
void interrupt213();
void interrupt214();
void interrupt215();
void interrupt216();
void interrupt217();
void interrupt218();
void interrupt219();
void interrupt220();
void interrupt221();
void interrupt222();
void interrupt223();
void interrupt224();
void interrupt225();
void interrupt226();
void interrupt227();
void interrupt228();
void interrupt229();
void interrupt230();
void interrupt231();
void interrupt232();
void interrupt233();
void interrupt234();
void interrupt235();
void interrupt236();
void interrupt237();
void interrupt238();
void interrupt239();
void interrupt240();
void interrupt241();
void interrupt242();
void interrupt243();
void interrupt244();
void interrupt245();
void interrupt246();
void interrupt247();
void interrupt248();
void interrupt249();
void interrupt250();
void interrupt251();
void interrupt252();
void interrupt253();
void interrupt254();
void interrupt255();

void interrupt_init(uintptr_t) {
  disable_pic();
  
  debug("[INT] stack for interrupts = {}\n", (void*)stack_for_interrupts);
  debug("[INT] stack for crash exceptions = {}\n", (void*)stack_for_crash_exceptions);
  debug("[INT] tss = {}\n", &_tss);
  tss_set_ist(IST::Interrupt, stack_for_interrupts + sizeof(stack_for_interrupts));
  tss_set_ist(IST::Unhandled, stack_for_crash_exceptions + sizeof(stack_for_crash_exceptions));

  interrupt_set_vector(0, interrupt000, IST::Unhandled);
  interrupt_set_vector(1, interrupt001, IST::Unhandled);
  interrupt_set_vector(2, interrupt002, IST::Unhandled);
  interrupt_set_vector(3, interrupt003, IST::Unhandled);
  interrupt_set_vector(4, interrupt004, IST::Unhandled);
  interrupt_set_vector(5, interrupt005, IST::Unhandled);
  interrupt_set_vector(6, interrupt006, IST::Unhandled);
  interrupt_set_vector(7, interrupt007, IST::Unhandled);
  interrupt_set_vector(8, interrupt008, IST::Unhandled);
  interrupt_set_vector(9, interrupt009, IST::Unhandled);
  interrupt_set_vector(10, interrupt010, IST::Unhandled);
  interrupt_set_vector(11, interrupt011, IST::Unhandled);
  interrupt_set_vector(12, interrupt012, IST::Unhandled);
  interrupt_set_vector(13, interrupt013, IST::Unhandled);
  interrupt_set_vector(14, interrupt014, IST::Unhandled);
  interrupt_set_vector(15, interrupt015, IST::Unhandled);
  interrupt_set_vector(16, interrupt016, IST::Unhandled);
  interrupt_set_vector(17, interrupt017, IST::Unhandled);
  interrupt_set_vector(18, interrupt018, IST::Unhandled);
  interrupt_set_vector(19, interrupt019, IST::Unhandled);
  interrupt_set_vector(20, interrupt020, IST::Unhandled);
  interrupt_set_vector(21, interrupt021, IST::Unhandled);
  interrupt_set_vector(22, interrupt022, IST::Unhandled);
  interrupt_set_vector(23, interrupt023, IST::Unhandled);
  interrupt_set_vector(24, interrupt024, IST::Unhandled);
  interrupt_set_vector(25, interrupt025, IST::Unhandled);
  interrupt_set_vector(26, interrupt026, IST::Unhandled);
  interrupt_set_vector(27, interrupt027, IST::Unhandled);
  interrupt_set_vector(28, interrupt028, IST::Unhandled);
  interrupt_set_vector(29, interrupt029, IST::Unhandled);
  interrupt_set_vector(30, interrupt030, IST::Unhandled);
  interrupt_set_vector(31, interrupt031, IST::Unhandled);
  interrupt_set_vector(32, interrupt032, IST::Interrupt);
  interrupt_set_vector(33, interrupt033, IST::Interrupt);
  interrupt_set_vector(34, interrupt034, IST::Interrupt);
  interrupt_set_vector(35, interrupt035, IST::Interrupt);
  interrupt_set_vector(36, interrupt036, IST::Interrupt);
  interrupt_set_vector(37, interrupt037, IST::Interrupt);
  interrupt_set_vector(38, interrupt038, IST::Interrupt);
  interrupt_set_vector(39, interrupt039, IST::Interrupt);
  interrupt_set_vector(40, interrupt040, IST::Interrupt);
  interrupt_set_vector(41, interrupt041, IST::Interrupt);
  interrupt_set_vector(42, interrupt042, IST::Interrupt);
  interrupt_set_vector(43, interrupt043, IST::Interrupt);
  interrupt_set_vector(44, interrupt044, IST::Interrupt);
  interrupt_set_vector(45, interrupt045, IST::Interrupt);
  interrupt_set_vector(46, interrupt046, IST::Interrupt);
  interrupt_set_vector(47, interrupt047, IST::Interrupt);
  interrupt_set_vector(48, interrupt048, IST::Interrupt);
  interrupt_set_vector(49, interrupt049, IST::Interrupt);
  interrupt_set_vector(50, interrupt050, IST::Interrupt);
  interrupt_set_vector(51, interrupt051, IST::Interrupt);
  interrupt_set_vector(52, interrupt052, IST::Interrupt);
  interrupt_set_vector(53, interrupt053, IST::Interrupt);
  interrupt_set_vector(54, interrupt054, IST::Interrupt);
  interrupt_set_vector(55, interrupt055, IST::Interrupt);
  interrupt_set_vector(56, interrupt056, IST::Interrupt);
  interrupt_set_vector(57, interrupt057, IST::Interrupt);
  interrupt_set_vector(58, interrupt058, IST::Interrupt);
  interrupt_set_vector(59, interrupt059, IST::Interrupt);
  interrupt_set_vector(60, interrupt060, IST::Interrupt);
  interrupt_set_vector(61, interrupt061, IST::Interrupt);
  interrupt_set_vector(62, interrupt062, IST::Interrupt);
  interrupt_set_vector(63, interrupt063, IST::Interrupt);
  interrupt_set_vector(64, interrupt064, IST::Interrupt);
  interrupt_set_vector(65, interrupt065, IST::Interrupt);
  interrupt_set_vector(66, interrupt066, IST::Interrupt);
  interrupt_set_vector(67, interrupt067, IST::Interrupt);
  interrupt_set_vector(68, interrupt068, IST::Interrupt);
  interrupt_set_vector(69, interrupt069, IST::Interrupt);
  interrupt_set_vector(70, interrupt070, IST::Interrupt);
  interrupt_set_vector(71, interrupt071, IST::Interrupt);
  interrupt_set_vector(72, interrupt072, IST::Interrupt);
  interrupt_set_vector(73, interrupt073, IST::Interrupt);
  interrupt_set_vector(74, interrupt074, IST::Interrupt);
  interrupt_set_vector(75, interrupt075, IST::Interrupt);
  interrupt_set_vector(76, interrupt076, IST::Interrupt);
  interrupt_set_vector(77, interrupt077, IST::Interrupt);
  interrupt_set_vector(78, interrupt078, IST::Interrupt);
  interrupt_set_vector(79, interrupt079, IST::Interrupt);
  interrupt_set_vector(80, interrupt080, IST::Interrupt);
  interrupt_set_vector(81, interrupt081, IST::Interrupt);
  interrupt_set_vector(82, interrupt082, IST::Interrupt);
  interrupt_set_vector(83, interrupt083, IST::Interrupt);
  interrupt_set_vector(84, interrupt084, IST::Interrupt);
  interrupt_set_vector(85, interrupt085, IST::Interrupt);
  interrupt_set_vector(86, interrupt086, IST::Interrupt);
  interrupt_set_vector(87, interrupt087, IST::Interrupt);
  interrupt_set_vector(88, interrupt088, IST::Interrupt);
  interrupt_set_vector(89, interrupt089, IST::Interrupt);
  interrupt_set_vector(90, interrupt090, IST::Interrupt);
  interrupt_set_vector(91, interrupt091, IST::Interrupt);
  interrupt_set_vector(92, interrupt092, IST::Interrupt);
  interrupt_set_vector(93, interrupt093, IST::Interrupt);
  interrupt_set_vector(94, interrupt094, IST::Interrupt);
  interrupt_set_vector(95, interrupt095, IST::Interrupt);
  interrupt_set_vector(96, interrupt096, IST::Interrupt);
  interrupt_set_vector(97, interrupt097, IST::Interrupt);
  interrupt_set_vector(98, interrupt098, IST::Interrupt);
  interrupt_set_vector(99, interrupt099, IST::Interrupt);
  interrupt_set_vector(100, interrupt100, IST::Interrupt);
  interrupt_set_vector(101, interrupt101, IST::Interrupt);
  interrupt_set_vector(102, interrupt102, IST::Interrupt);
  interrupt_set_vector(103, interrupt103, IST::Interrupt);
  interrupt_set_vector(104, interrupt104, IST::Interrupt);
  interrupt_set_vector(105, interrupt105, IST::Interrupt);
  interrupt_set_vector(106, interrupt106, IST::Interrupt);
  interrupt_set_vector(107, interrupt107, IST::Interrupt);
  interrupt_set_vector(108, interrupt108, IST::Interrupt);
  interrupt_set_vector(109, interrupt109, IST::Interrupt);
  interrupt_set_vector(110, interrupt110, IST::Interrupt);
  interrupt_set_vector(111, interrupt111, IST::Interrupt);
  interrupt_set_vector(112, interrupt112, IST::Interrupt);
  interrupt_set_vector(113, interrupt113, IST::Interrupt);
  interrupt_set_vector(114, interrupt114, IST::Interrupt);
  interrupt_set_vector(115, interrupt115, IST::Interrupt);
  interrupt_set_vector(116, interrupt116, IST::Interrupt);
  interrupt_set_vector(117, interrupt117, IST::Interrupt);
  interrupt_set_vector(118, interrupt118, IST::Interrupt);
  interrupt_set_vector(119, interrupt119, IST::Interrupt);
  interrupt_set_vector(120, interrupt120, IST::Interrupt);
  interrupt_set_vector(121, interrupt121, IST::Interrupt);
  interrupt_set_vector(122, interrupt122, IST::Interrupt);
  interrupt_set_vector(123, interrupt123, IST::Interrupt);
  interrupt_set_vector(124, interrupt124, IST::Interrupt);
  interrupt_set_vector(125, interrupt125, IST::Interrupt);
  interrupt_set_vector(126, interrupt126, IST::Interrupt);
  interrupt_set_vector(127, interrupt127, IST::Interrupt);
  interrupt_set_vector(128, interrupt128, IST::Interrupt);
  interrupt_set_vector(129, interrupt129, IST::Interrupt);
  interrupt_set_vector(130, interrupt130, IST::Interrupt);
  interrupt_set_vector(131, interrupt131, IST::Interrupt);
  interrupt_set_vector(132, interrupt132, IST::Interrupt);
  interrupt_set_vector(133, interrupt133, IST::Interrupt);
  interrupt_set_vector(134, interrupt134, IST::Interrupt);
  interrupt_set_vector(135, interrupt135, IST::Interrupt);
  interrupt_set_vector(136, interrupt136, IST::Interrupt);
  interrupt_set_vector(137, interrupt137, IST::Interrupt);
  interrupt_set_vector(138, interrupt138, IST::Interrupt);
  interrupt_set_vector(139, interrupt139, IST::Interrupt);
  interrupt_set_vector(140, interrupt140, IST::Interrupt);
  interrupt_set_vector(141, interrupt141, IST::Interrupt);
  interrupt_set_vector(142, interrupt142, IST::Interrupt);
  interrupt_set_vector(143, interrupt143, IST::Interrupt);
  interrupt_set_vector(144, interrupt144, IST::Interrupt);
  interrupt_set_vector(145, interrupt145, IST::Interrupt);
  interrupt_set_vector(146, interrupt146, IST::Interrupt);
  interrupt_set_vector(147, interrupt147, IST::Interrupt);
  interrupt_set_vector(148, interrupt148, IST::Interrupt);
  interrupt_set_vector(149, interrupt149, IST::Interrupt);
  interrupt_set_vector(150, interrupt150, IST::Interrupt);
  interrupt_set_vector(151, interrupt151, IST::Interrupt);
  interrupt_set_vector(152, interrupt152, IST::Interrupt);
  interrupt_set_vector(153, interrupt153, IST::Interrupt);
  interrupt_set_vector(154, interrupt154, IST::Interrupt);
  interrupt_set_vector(155, interrupt155, IST::Interrupt);
  interrupt_set_vector(156, interrupt156, IST::Interrupt);
  interrupt_set_vector(157, interrupt157, IST::Interrupt);
  interrupt_set_vector(158, interrupt158, IST::Interrupt);
  interrupt_set_vector(159, interrupt159, IST::Interrupt);
  interrupt_set_vector(160, interrupt160, IST::Interrupt);
  interrupt_set_vector(161, interrupt161, IST::Interrupt);
  interrupt_set_vector(162, interrupt162, IST::Interrupt);
  interrupt_set_vector(163, interrupt163, IST::Interrupt);
  interrupt_set_vector(164, interrupt164, IST::Interrupt);
  interrupt_set_vector(165, interrupt165, IST::Interrupt);
  interrupt_set_vector(166, interrupt166, IST::Interrupt);
  interrupt_set_vector(167, interrupt167, IST::Interrupt);
  interrupt_set_vector(168, interrupt168, IST::Interrupt);
  interrupt_set_vector(169, interrupt169, IST::Interrupt);
  interrupt_set_vector(170, interrupt170, IST::Interrupt);
  interrupt_set_vector(171, interrupt171, IST::Interrupt);
  interrupt_set_vector(172, interrupt172, IST::Interrupt);
  interrupt_set_vector(173, interrupt173, IST::Interrupt);
  interrupt_set_vector(174, interrupt174, IST::Interrupt);
  interrupt_set_vector(175, interrupt175, IST::Interrupt);
  interrupt_set_vector(176, interrupt176, IST::Interrupt);
  interrupt_set_vector(177, interrupt177, IST::Interrupt);
  interrupt_set_vector(178, interrupt178, IST::Interrupt);
  interrupt_set_vector(179, interrupt179, IST::Interrupt);
  interrupt_set_vector(180, interrupt180, IST::Interrupt);
  interrupt_set_vector(181, interrupt181, IST::Interrupt);
  interrupt_set_vector(182, interrupt182, IST::Interrupt);
  interrupt_set_vector(183, interrupt183, IST::Interrupt);
  interrupt_set_vector(184, interrupt184, IST::Interrupt);
  interrupt_set_vector(185, interrupt185, IST::Interrupt);
  interrupt_set_vector(186, interrupt186, IST::Interrupt);
  interrupt_set_vector(187, interrupt187, IST::Interrupt);
  interrupt_set_vector(188, interrupt188, IST::Interrupt);
  interrupt_set_vector(189, interrupt189, IST::Interrupt);
  interrupt_set_vector(190, interrupt190, IST::Interrupt);
  interrupt_set_vector(191, interrupt191, IST::Interrupt);
  interrupt_set_vector(192, interrupt192, IST::Interrupt);
  interrupt_set_vector(193, interrupt193, IST::Interrupt);
  interrupt_set_vector(194, interrupt194, IST::Interrupt);
  interrupt_set_vector(195, interrupt195, IST::Interrupt);
  interrupt_set_vector(196, interrupt196, IST::Interrupt);
  interrupt_set_vector(197, interrupt197, IST::Interrupt);
  interrupt_set_vector(198, interrupt198, IST::Interrupt);
  interrupt_set_vector(199, interrupt199, IST::Interrupt);
  interrupt_set_vector(200, interrupt200, IST::Interrupt);
  interrupt_set_vector(201, interrupt201, IST::Interrupt);
  interrupt_set_vector(202, interrupt202, IST::Interrupt);
  interrupt_set_vector(203, interrupt203, IST::Interrupt);
  interrupt_set_vector(204, interrupt204, IST::Interrupt);
  interrupt_set_vector(205, interrupt205, IST::Interrupt);
  interrupt_set_vector(206, interrupt206, IST::Interrupt);
  interrupt_set_vector(207, interrupt207, IST::Interrupt);
  interrupt_set_vector(208, interrupt208, IST::Interrupt);
  interrupt_set_vector(209, interrupt209, IST::Interrupt);
  interrupt_set_vector(210, interrupt210, IST::Interrupt);
  interrupt_set_vector(211, interrupt211, IST::Interrupt);
  interrupt_set_vector(212, interrupt212, IST::Interrupt);
  interrupt_set_vector(213, interrupt213, IST::Interrupt);
  interrupt_set_vector(214, interrupt214, IST::Interrupt);
  interrupt_set_vector(215, interrupt215, IST::Interrupt);
  interrupt_set_vector(216, interrupt216, IST::Interrupt);
  interrupt_set_vector(217, interrupt217, IST::Interrupt);
  interrupt_set_vector(218, interrupt218, IST::Interrupt);
  interrupt_set_vector(219, interrupt219, IST::Interrupt);
  interrupt_set_vector(220, interrupt220, IST::Interrupt);
  interrupt_set_vector(221, interrupt221, IST::Interrupt);
  interrupt_set_vector(222, interrupt222, IST::Interrupt);
  interrupt_set_vector(223, interrupt223, IST::Interrupt);
  interrupt_set_vector(224, interrupt224, IST::Interrupt);
  interrupt_set_vector(225, interrupt225, IST::Interrupt);
  interrupt_set_vector(226, interrupt226, IST::Interrupt);
  interrupt_set_vector(227, interrupt227, IST::Interrupt);
  interrupt_set_vector(228, interrupt228, IST::Interrupt);
  interrupt_set_vector(229, interrupt229, IST::Interrupt);
  interrupt_set_vector(230, interrupt230, IST::Interrupt);
  interrupt_set_vector(231, interrupt231, IST::Interrupt);
  interrupt_set_vector(232, interrupt232, IST::Interrupt);
  interrupt_set_vector(233, interrupt233, IST::Interrupt);
  interrupt_set_vector(234, interrupt234, IST::Interrupt);
  interrupt_set_vector(235, interrupt235, IST::Interrupt);
  interrupt_set_vector(236, interrupt236, IST::Interrupt);
  interrupt_set_vector(237, interrupt237, IST::Interrupt);
  interrupt_set_vector(238, interrupt238, IST::Interrupt);
  interrupt_set_vector(239, interrupt239, IST::Interrupt);
  interrupt_set_vector(240, interrupt240, IST::Interrupt);
  interrupt_set_vector(241, interrupt241, IST::Interrupt);
  interrupt_set_vector(242, interrupt242, IST::Interrupt);
  interrupt_set_vector(243, interrupt243, IST::Interrupt);
  interrupt_set_vector(244, interrupt244, IST::Interrupt);
  interrupt_set_vector(245, interrupt245, IST::Interrupt);
  interrupt_set_vector(246, interrupt246, IST::Interrupt);
  interrupt_set_vector(247, interrupt247, IST::Interrupt);
  interrupt_set_vector(248, interrupt248, IST::Interrupt);
  interrupt_set_vector(249, interrupt249, IST::Interrupt);
  interrupt_set_vector(250, interrupt250, IST::Interrupt);
  interrupt_set_vector(251, interrupt251, IST::Interrupt);
  interrupt_set_vector(252, interrupt252, IST::Interrupt);
  interrupt_set_vector(253, interrupt253, IST::Interrupt);
  interrupt_set_vector(254, interrupt254, IST::Interrupt);
  interrupt_set_vector(255, interrupt255, IST::Interrupt);

  uint64_t p = (uint64_t)&_tss;
  uint32_t size = sizeof(_tss);
  gdt_[4].access = 0x89;
  gdt_[4].limit_flags = ((size & 0xF0000) >> 16);
  gdt_[4].limit = (size & 0xFFFF);
  gdt_[4].base1 = p & 0xFFFF;
  gdt_[4].base2 = (p >> 16) & 0xFF;
  gdt_[4].base3 = (p >> 24) & 0xFF;
  gdt_[5].limit = (p >> 32) & 0xFFFF;
  gdt_[5].base1 = (p >> 48) & 0xFFFF;
  gdt_[5].base2 = 0;
  gdt_[5].limit_flags = 0;
  gdt_[5].access = 0;
  gdt_[5].base3 = 0;

  asm volatile ("lidt (%%rax)" :: "a"(&_loadidt));
  asm volatile ("lgdt (%%rax)" :: "a"(&_loadgdt));
  asm volatile ("ltr %%ax\n" :: "a"(0x20));
}

void halt_for_interrupts() {
  asm volatile ("sti; hlt; cli");
}

void platform_enable_interrupts() {
  asm volatile ("sti");
}

void platform_disable_interrupts() {
  asm volatile ("cli");
}

#endif


