#ifdef __aarch64__

.globl Vector_table_el1
.balign 0x800
Vector_table_el1:
curr_el_sp0_sync: 
    mov     x0, #0
    mrs     x1, esr_el1
    mrs     x2, elr_el1
    mrs     x3, spsr_el1
    mrs     x4, far_el1
    b       sync_handler
.balign 0x80
curr_el_sp0_irq: // THIS IS THE IRQ HANDLER THINGUMS
  sub lr, lr, #4
  sub sp, sp, #256
	stp	x0, x1, [sp, #16 * 0]
	stp	x2, x3, [sp, #16 * 1]
	stp	x4, x5, [sp, #16 * 2]
	stp	x6, x7, [sp, #16 * 3]
	stp	x8, x9, [sp, #16 * 4]
	stp	x10, x11, [sp, #16 * 5]
	stp	x12, x13, [sp, #16 * 6]
	stp	x14, x15, [sp, #16 * 7]
	stp	x16, x17, [sp, #16 * 8]
	stp	x18, x19, [sp, #16 * 9]
	stp	x20, x21, [sp, #16 * 10]
	stp	x22, x23, [sp, #16 * 11]
	stp	x24, x25, [sp, #16 * 12]
	stp	x26, x27, [sp, #16 * 13]
	stp	x28, x29, [sp, #16 * 14]
  bl       irq_handler
  ldp     x0, x1, [sp, #16 * 0]
  ldp     x2, x3, [sp, #16 * 1]
  ldp     x4, x5, [sp, #16 * 2]
  ldp     x6, x7, [sp, #16 * 3]
  ldp     x8, x9, [sp, #16 * 4]
  ldp     x10, x11, [sp, #16 * 5]
  ldp     x12, x13, [sp, #16 * 6]
  ldp     x14, x15, [sp, #16 * 7]
  ldp     x16, x17, [sp, #16 * 8]
  ldp     x18, x19, [sp, #16 * 9]
  ldp     x20, x21, [sp, #16 * 10]
  ldp     x22, x23, [sp, #16 * 11]
  ldp     x24, x25, [sp, #16 * 12]
  ldp     x26, x27, [sp, #16 * 13]
  ldp     x28, x29, [sp, #16 * 14]
  add sp, sp, #256
  eret

.balign 0x80
curr_el_sp0_fiq:       
    mov     x0, #2
    mrs     x1, esr_el1
    mrs     x2, elr_el1
    mrs     x3, spsr_el1
    mrs     x4, far_el1
    b       fiq_handler
.balign 0x80
curr_el_sp0_serror:    
    mov     x0, #3
    mrs     x1, esr_el1
    mrs     x2, elr_el1
    mrs     x3, spsr_el1
    mrs     x4, far_el1
    b       serror_handler
.balign 0x80
curr_el_spx_sync:       
    mov     x0, #4
    mrs     x1, esr_el1
    mrs     x2, elr_el1
    mrs     x3, spsr_el1
    mrs     x4, far_el1
    b       sync_handler
.balign 0x80
curr_el_spx_irq:    
    mov     x0, #5
    mrs     x1, esr_el1
    mrs     x2, elr_el1
    mrs     x3, spsr_el1
    mrs     x4, far_el1
    b       irq_handler
.balign 0x80
curr_el_spx_fiq:           
    mov     x0, #6
    mrs     x1, esr_el1
    mrs     x2, elr_el1
    mrs     x3, spsr_el1
    mrs     x4, far_el1
    b       fiq_handler
.balign 0x80
curr_el_spx_serror:      
    mov     x0, #7
    mrs     x1, esr_el1
    mrs     x2, elr_el1
    mrs     x3, spsr_el1
    mrs     x4, far_el1
    b       serror_handler
.balign 0x80
lower_el_aarch64_sync:    
    mov     x0, #8
    mrs     x1, esr_el1
    mrs     x2, elr_el1
    mrs     x3, spsr_el1
    mrs     x4, far_el1
    b       sync_handler
.balign 0x80
lower_el_aarch64_irq:    
    mov     x0, #9
    mrs     x1, esr_el1
    mrs     x2, elr_el1
    mrs     x3, spsr_el1
    mrs     x4, far_el1
    b       irq_handler
.balign 0x80
lower_el_aarch64_fiq:      
    mov     x0, #10
    mrs     x1, esr_el1
    mrs     x2, elr_el1
    mrs     x3, spsr_el1
    mrs     x4, far_el1
    b       fiq_handler
.balign 0x80
lower_el_aarch64_serror: 
    mov     x0, #11
    mrs     x1, esr_el1
    mrs     x2, elr_el1
    mrs     x3, spsr_el1
    mrs     x4, far_el1
    b       serror_handler
.balign 0x80
lower_el_aarch32_sync:   
    mov     x0, #12
    mrs     x1, esr_el1
    mrs     x2, elr_el1
    mrs     x3, spsr_el1
    mrs     x4, far_el1
    b       sync_handler
.balign 0x80
lower_el_aarch32_irq:    
    mov     x0, #13
    mrs     x1, esr_el1
    mrs     x2, elr_el1
    mrs     x3, spsr_el1
    mrs     x4, far_el1
    b       irq_handler
.balign 0x80
lower_el_aarch32_fiq:    
    mov     x0, #14
    mrs     x1, esr_el1
    mrs     x2, elr_el1
    mrs     x3, spsr_el1
    mrs     x4, far_el1
    b       fiq_handler
.balign 0x80
lower_el_aarch32_serror: 
    mov     x0, #15
    mrs     x1, esr_el1
    mrs     x2, elr_el1
    mrs     x3, spsr_el1
    mrs     x4, far_el1
    b       serror_handler

#endif


