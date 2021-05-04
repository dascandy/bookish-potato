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


    // set up EL1
    mrs     x0, CurrentEL
    and     x0, x0, #12 // clear reserved bits

    // running at EL3?
    cmp     x0, #12
    bne     5f
    // should never be executed, just for completeness
    mov     x2, #0x5b1
    msr     scr_el3, x2
    mov     x2, #0x3c9
    msr     spsr_el3, x2
    adr     x2, 5f
    msr     elr_el3, x2
    eret

5:  cmp     x0, #4
    beq     5f
    msr     sp_el1, x1
    // enable CNTP for EL1
    mrs     x0, cnthctl_el2
    orr     x0, x0, #3
    msr     cnthctl_el2, x0
    msr     cntvoff_el2, xzr
    // enable AArch64 in EL1
    mov     x0, #(1 << 31)      // AArch64
    orr     x0, x0, #(1 << 1)   // SWIO hardwired on Pi3
    msr     hcr_el2, x0
    mrs     x0, hcr_el2
    // Setup SCTLR access
    mov     x2, #0x0800
    movk    x2, #0x30d0, lsl #16
    msr     sctlr_el1, x2
    // change execution level to EL1
    mov     x2, #0x3c4
    msr     spsr_el2, x2
    adr     x2, 5f
    msr     elr_el2, x2
    eret

5:  mov     sp, x1
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


