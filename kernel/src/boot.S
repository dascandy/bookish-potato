// To keep this in the first portion of the binary.
.section ".text.boot"
 
// Make _start global.
.globl _start

#ifdef __ARM
    .org 0x80000
_start:
    // read cpu id
    mrs     x1, mpidr_el1
    and     x1, x1, #3    // 0-3
    // Set up stacks
    add     x1, x1, #1    // 1-4
    lsl     x1, x1, #17   // 0x20000 - 0x80000
    mov     sp, x1

    // Re-read cpu id
    mrs     x1, mpidr_el1
    and     x1, x1, #3    // 0-3
    cbz     x1, 2f // Bounce off only the primary to entrypoint
1:  b       kernel_secondary_cpu
2:  // clear bss
    ldr     x1, =__bss_start
    ldr     w2, =__bss_size
3:  cbz     w2, 4f
    str     xzr, [x1], #8
    sub     w2, w2, #1
    cbnz    w2, 3b
 
4:
    b       kernel_entry

#endif // __ARM

#ifdef __x86_64__
.code32  
_start:
  cli
  movl %eax, %ebp
  // build paging tables
  xorl %eax, %eax
  movl $0x1000, %ecx
  movl %ecx, %edi
  rep stosl

  // Identity-map the first 4G using 1G pages
  movl $0x1007, %es:0x1ff8
  movl $0x2007, %es:0x1000
  movl $0x00000087, %es:0x2000
  movl $0x40000087, %es:0x2008
  movl $0x80000087, %es:0x2010
  movl $0xc0000087, %es:0x2018

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

