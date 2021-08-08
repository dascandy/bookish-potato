#ifdef __x86_64__
.section ".text"

.globl _Z15interrupt_checkv
.globl _Z20unhandled_interrupt_PK9registers
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


.globl _Z12interrupt000v
_Z12interrupt000v:
    pushq $0
    pushq $0
    jmp _Z19unhandled_interruptv

.globl _Z12interrupt001v
_Z12interrupt001v:
    pushq $0
    pushq $1
    jmp _Z19unhandled_interruptv

.globl _Z12interrupt002v
_Z12interrupt002v:
    pushq $0
    pushq $2
    jmp _Z19unhandled_interruptv

.globl _Z12interrupt003v
_Z12interrupt003v:
    pushq $0
    pushq $3
    jmp _Z19unhandled_interruptv

.globl _Z12interrupt004v
_Z12interrupt004v:
    pushq $0
    pushq $4
    jmp _Z19unhandled_interruptv

.globl _Z12interrupt005v
_Z12interrupt005v:
    pushq $0
    pushq $5
    jmp _Z19unhandled_interruptv

.globl _Z12interrupt006v
_Z12interrupt006v:
    pushq $0
    pushq $6
    jmp _Z19unhandled_interruptv

.globl _Z12interrupt007v
_Z12interrupt007v:
    pushq $0
    pushq $7
    jmp _Z19unhandled_interruptv

.globl _Z12interrupt008v
_Z12interrupt008v:
    pushq $8
    jmp _Z19unhandled_interruptv

.globl _Z12interrupt009v
_Z12interrupt009v:
    pushq $0
    pushq $9
    jmp _Z19unhandled_interruptv

.globl _Z12interrupt010v
_Z12interrupt010v:
    pushq $0
    pushq $10
    jmp _Z19unhandled_interruptv

.globl _Z12interrupt011v
_Z12interrupt011v:
    pushq $11
    jmp _Z19unhandled_interruptv

.globl _Z12interrupt012v
_Z12interrupt012v:
    pushq $12
    jmp _Z19unhandled_interruptv

.globl _Z12interrupt013v
_Z12interrupt013v:
    pushq $13
    jmp _Z19unhandled_interruptv

.globl _Z12interrupt014v
_Z12interrupt014v:
    pushq $14
    jmp _Z19unhandled_interruptv

.globl _Z12interrupt015v
_Z12interrupt015v:
    pushq $0
    pushq $15
    jmp _Z19unhandled_interruptv

.globl _Z12interrupt016v
_Z12interrupt016v:
    pushq $0
    pushq $16
    jmp _Z19unhandled_interruptv

.globl _Z12interrupt017v
_Z12interrupt017v:
    pushq $17
    jmp _Z19unhandled_interruptv

.globl _Z12interrupt018v
_Z12interrupt018v:
    pushq $0
    pushq $18
    jmp _Z19unhandled_interruptv

.globl _Z12interrupt019v
_Z12interrupt019v:
    pushq $0
    pushq $19
    jmp _Z19unhandled_interruptv

.globl _Z12interrupt020v
_Z12interrupt020v:
    pushq $0
    pushq $20
    jmp _Z19unhandled_interruptv

.globl _Z12interrupt021v
_Z12interrupt021v:
    pushq $0
    pushq $21
    jmp _Z19unhandled_interruptv

.globl _Z12interrupt022v
_Z12interrupt022v:
    pushq $0
    pushq $22
    jmp _Z19unhandled_interruptv

.globl _Z12interrupt023v
_Z12interrupt023v:
    pushq $0
    pushq $23
    jmp _Z19unhandled_interruptv

.globl _Z12interrupt024v
_Z12interrupt024v:
    pushq $0
    pushq $24
    jmp _Z19unhandled_interruptv

.globl _Z12interrupt025v
_Z12interrupt025v:
    pushq $0
    pushq $25
    jmp _Z19unhandled_interruptv

.globl _Z12interrupt026v
_Z12interrupt026v:
    pushq $0
    pushq $26
    jmp _Z19unhandled_interruptv

.globl _Z12interrupt027v
_Z12interrupt027v:
    pushq $0
    pushq $27
    jmp _Z19unhandled_interruptv

.globl _Z12interrupt028v
_Z12interrupt028v:
    pushq $0
    pushq $28
    jmp _Z19unhandled_interruptv

.globl _Z12interrupt029v
_Z12interrupt029v:
    pushq $0
    pushq $29
    jmp _Z19unhandled_interruptv

.globl _Z12interrupt030v
_Z12interrupt030v:
    pushq $30
    jmp _Z19unhandled_interruptv

.globl _Z12interrupt031v
_Z12interrupt031v:
    pushq $0
    pushq $31
    jmp _Z19unhandled_interruptv

.globl _Z9interrupti
_Z9interrupti:
    push %rax
    push %rbx
    push %rcx
    push %rdx
    push %rsi
    push %rbp
    push %r8
    push %r9
    push %r10
    push %r11
    push %r12
    push %r13
    push %r14
    push %r15
    call _Z15interrupt_checki
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %r11 
    pop %r10 
    pop %r9 
    pop %r8 
    pop %rbp 
    pop %rsi 
    pop %rdx 
    pop %rcx 
    pop %rbx 
    pop %rax 
    pop %rdi 
    iretq

#include "generated_interrupt.S.excl"

#endif

