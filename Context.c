#include "Context.h"

void SaveContext(RegContext *ctx) {
    __asm__ volatile(
        "mov %%rax, 0x00(%0)\n"
        "mov %%rcx, 0x08(%0)\n"
        "mov %%rdx, 0x10(%0)\n"
        "mov %%rbx, 0x18(%0)\n"
        "mov %%rsp, 0x20(%0)\n"
        "mov %%rbp, 0x28(%0)\n"
        "mov %%rsi, 0x30(%0)\n"
        "mov %%rdi, 0x38(%0)\n"
        "mov %%r8,  0x40(%0)\n"
        "mov %%r9,  0x48(%0)\n"
        "mov %%r10, 0x50(%0)\n"
        "mov %%r11, 0x58(%0)\n"
        "mov %%r12, 0x60(%0)\n"
        "mov %%r13, 0x68(%0)\n"
        "mov %%r14, 0x70(%0)\n"
        "mov %%r15, 0x78(%0)\n"
        "mov (%%rsp), %%rax\n"
        "mov %%rax, 0x80(%0)\n"
        "mov %%cs,  0x88(%0)\n"
        "mov %%ds,  0x90(%0)\n"
        "mov %%es,  0x98(%0)\n"
        "mov %%fs,  0xA0(%0)\n"
        "mov %%gs,  0xA8(%0)\n"
        "mov %%ss,  0xB0(%0)\n"
        "pushfq\n"
        "popq 0xB8(%0)\n"
        "mov $0xFFFFFFFF, %%eax\n"
        "mov $0xFFFFFFFF, %%edx\n"
        "xsave 0xC0(%0)\n"
        :
        : "r"(ctx)
        : "memory", "cc"
    );
}

void LoadContext(RegContext *ctx) {
    __asm__ volatile(
        "mov 0x00(%0), %%rax\n"
        "mov 0x08(%0), %%rcx\n"
        "mov 0x10(%0), %%rdx\n"
        "mov 0x18(%0), %%rbx\n"
        "mov 0x20(%0), %%rsp\n"
        "mov 0x28(%0), %%rbp\n"
        "mov 0x30(%0), %%rsi\n"
        "mov 0x38(%0), %%rdi\n"
        "mov 0x40(%0), %%r8\n"
        "mov 0x48(%0), %%r9\n"
        "mov 0x50(%0), %%r10\n"
        "mov 0x58(%0), %%r11\n"
        "mov 0x60(%0), %%r12\n"
        "mov 0x68(%0), %%r13\n"
        "mov 0x70(%0), %%r14\n"
        "mov 0x78(%0), %%r15\n"
        "mov 0x90(%0), %%ds\n"
        "mov 0x98(%0), %%es\n"
        "mov 0xA0(%0), %%fs\n"
        "mov 0xA8(%0), %%gs\n"
        "mov 0xB0(%0), %%ss\n"
        "mov $0xFFFFFFFF, %%eax\n"
        "mov $0xFFFFFFFF, %%edx\n"
        "xrstor 0xC0(%0)\n"
        "pushq 0x88(%0)\n"
        "pushq 0x80(%0)\n"
        "pushq 0xB8(%0)\n"
        "iretq\n"
        :
        : "r"(ctx)
        : "memory"
    );
}