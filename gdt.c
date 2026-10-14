#include "gdt.h"
#include "debug.h"
#include "int.h"

struct GDTR {
    u16 limit;
    u64 base;
} __attribute__((packed));

static struct GDTEntry gdt[6] = {
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0x9A, 0x20, 0},
    {0, 0, 0, 0x92, 0, 0},
    {0},
    {0},
    {0}
};

static struct GDTR gdtr;

void LoadGDT() {
    DebugStr("Load GDT\n");
    gdtr.limit = sizeof(gdt) - 1;
    gdtr.base = (u64)gdt;
    
    __asm__ volatile(
        "lgdt %0\n\t"
        "pushq $0x08\n\t"
        "leaq 1f(%%rip), %%rax\n\t"
        "pushq %%rax\n\t"
        "lretq\n\t"
        "1:\n\t"
        "movw $0x10, %%ax\n\t"
        "movw %%ax, %%ds\n\t"
        "movw %%ax, %%es\n\t"
        "movw %%ax, %%ss\n\t"
        "movw %%ax, %%fs\n\t"
        "movw %%ax, %%gs\n\t"
        :
        : "m"(gdtr)
        : "rax", "memory"
    );
}