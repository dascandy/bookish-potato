
.section ".text.boot"

.globl _Z20unhandled_interrupt_PK9registers
.globl _Z24unhandled_interrupt_noecv
.globl _Z19unhandled_interruptv
_Z24unhandled_interrupt_noecv:
    pushq $0
_Z19unhandled_interruptv:
    push %rax
    push %rbx
    push %rcx
    push %rdx
    push %rsi
    push %rdi
    push %rbp
    push %r8
    push %r9
    push %r10
    push %r11
    push %r12
    push %r13
    push %r14
    push %r15
    mov %cr0, %rax
    push %rax
    mov %cr2, %rax
    push %rax
    mov %cr3, %rax
    push %rax
    mov %cr4, %rax
    push %rax
    movq %rsp, %rdi
    jmp _Z20unhandled_interrupt_PK9registers

