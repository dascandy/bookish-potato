#ifdef __x86_64__
.section ".text.boot"

.globl _Z15interrupt_checkv
.globl _Z9interruptv
_Z9interruptv:
    push %rax
    push %rcx
    push %rdx
    push %rsi
    push %rdi
    push %r8
    push %r9
    push %r10
    push %r11
    call _Z15interrupt_checkv
    pop %r11
    pop %r10
    pop %r9
    pop %r8
    pop %rdi
    pop %rsi
    pop %rdx
    pop %rcx
    pop %rax
    iret

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
#endif

