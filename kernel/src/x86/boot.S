#ifdef __x86_64__
.section ".text.boot"
.globl _start
.code32  
_start:
  cli
  movl %eax, %ebp
  // build paging tables
  xorl %eax, %eax
  movl $0x1000, %ecx
  movl %ecx, %edi
  rep stosl

  // Identity-map the first 16M with 2M pages
  movl $0x1007, %es:0x1ff8
  movl $0x2007, %es:0x1000
  movl $0x3007, %es:0x2000
  movl $0x00000087, %es:0x3000
  movl $0x00200087, %es:0x3008
  movl $0x00400087, %es:0x3010
  movl $0x00600087, %es:0x3018
  movl $0x00800087, %es:0x3020
  movl $0x00a00087, %es:0x3028
  movl $0x00c00087, %es:0x3030
  movl $0x00e00087, %es:0x3038

  // Trigger switch to LM directly
  lgdt (gdt64_load)
  movl %cr4, %eax
  orl $0x620, %eax // PAE | OSFXSR | OSXMMEXCPT
  movl %eax, %cr4
  movl $0xc0000080, %ecx // EFER
  rdmsr
  orl $0x901, %eax // LME | SCE | NXE
  wrmsr
  movl $0x1000, %eax
  movl %eax, %cr3
  movl $0x80000003, %eax // PG | PE | MP
  movl %eax, %cr0
  jmpl $0x08,$lm_entry // jump required to reload CS & to actually actuate the switch

.code64
    .align 8
    .word 0, 0, 0
gdt64_load:
    .word 0x1F
gdt64:
    .quad gdt64
    .long 0x0FFFF, 0x0002F9800
    .long 0x0FFFF, 0x0002FF800
    .long 0x0FFFF, 0x0002F9200

lm_entry:
    movw $0x18, %ax
    movw %ax, %es
    movw %ax, %ds
    movw %ax, %ss
    movq $0xa0000, %rsp
    movq %rbx, %rdi
    movq %rbp, %rsi
    jmp     kernel_entry

.section ".multiboot"
// Multiboot 2
    .align 8
    .long 0xe85250d6
    .long 0x0
    .long 0x30
    .long 0x17adaefa

    .int 1
    .int 0
    .long 24
    .long 1
    .long 3
    .long 6
    .long 21

    .int 0
    .int 0
    .long 8

// Multiboot 1
    .long 0x1BADB002
    .long 0x3
    .long 0xE4524FFB
#endif

