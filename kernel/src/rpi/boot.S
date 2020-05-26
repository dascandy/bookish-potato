#ifndef __x86_64__
.section ".text.boot"
.globl _start
_start:
/*
    // read cpu id
    mrs     x1, mpidr_el1
    and     x1, x1, #3    // 0-3
    // Set up stacks
    add     x1, x1, #1    // 1-4
    lsl     x1, x1, #17   // 0x20000 - 0x80000
    mov     sp, x1
*/

    // Re-read cpu id
    mrs     x1, mpidr_el1
    and     x1, x1, #3    // 0-3
    cbz     x1, 2f // Bounce off only the primary to entrypoint
1:  wfe
    b 1b
// 1:  b       kernel_secondary_cpu
2: 
    ldr x1, =_start
    mov sp, x1
    ldr     x1, =__bss_start
    ldr     w3, =__bss_size
3:  cbz     w3, 4f
    str     xzr, [x1], #8
    sub     w3, w3, #1
    cbnz    w3, 3b
 
4:
    mov     x0, x2
    mov     x1, #0xbeef
    bl      kernel_entry
    b 1b

#endif


