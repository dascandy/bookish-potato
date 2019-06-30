// AArch64 mode
 
// To keep this in the first portion of the binary.
.section ".text.boot"
 
// Make _start global.
.globl _start
 
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
    
