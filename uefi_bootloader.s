.code64
.global efi_main
.extern Cefi_main

.section .text
efi_main:
    pushq %rsi
    leaq Welcome(%rip), %rsi
    call .print_string_asm
    popq %rsi

    pushq %rcx
    pushq %rdx

    movq %cr0, %rax
    andq $0xfffffffffffffff1, %rax
    orq $0x22, %rax
    movq %rax, %cr0

    movq %cr4, %rax
    orq $0x40600, %rax
    movq %rax, %cr4

    xorl %ecx, %ecx
    xgetbv
    orl $0x6, %eax
    xorl %edx, %edx
    xsetbv

    popq %rdx
    popq %rcx

    mov %rsp, %rax
    and $-16, %rsp
    push %rax
    leaq Cefi_main(%rip), %rax
    jmp *%rax
.print_string_asm:
    pushq %rax
    pushq %rdx
    movq 0x3F8, %rdx
    1: movb (%rsi), %al
    testb %al, %al
    jz 2f
    outb %al, %dx
    incq %rsi
    jmp 1b
    2: popq %rdx
    popq %rax
    ret

Welcome: .asciz "Welcom To NeuroSamaOS\n"
