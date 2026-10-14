#include "ap.h"
#include "int.h"
#include "CpuInfo.h"
#include "clock.h"

volatile u64 APStackSPs[288];
volatile u64 APStackBPs[288];
volatile u32 APAPICIDs[288];
__attribute__((aligned(8))) volatile u8 APAPICIDsBusy[288];

void* (*APCallFunc[288])(void* ArgList) = {NULL};
void* APCallFuncArg[288] = {NULL};
void** APCallFuncTarget[288] = {NULL};

__attribute__((naked)) void GlobalAPDoTaskFunc(){
    __asm__ volatile(
        "pushq %%rax\n\t" "pushq %%rcx\n\t" "pushq %%rdx\n\t" "pushq %%rbx\n\t"
        "pushq %%rbp\n\t" "pushq %%rsi\n\t" "pushq %%rdi\n\t"
        "pushq %%r8\n\t"  "pushq %%r9\n\t"  "pushq %%r10\n\t" "pushq %%r11\n\t"
        "pushq %%r12\n\t" "pushq %%r13\n\t" "pushq %%r14\n\t" "pushq %%r15\n\t"
        "call GlobalAPDoTaskFuncDo\n\t"
        "popq %%r15\n\t" "popq %%r14\n\t" "popq %%r13\n\t" "popq %%r12\n\t"
        "popq %%r11\n\t" "popq %%r10\n\t" "popq %%r9\n\t"  "popq %%r8\n\t"
        "popq %%rdi\n\t" "popq %%rsi\n\t" "popq %%rbp\n\t" "popq %%rbx\n\t"
        "popq %%rdx\n\t" "popq %%rcx\n\t" "popq %%rax\n\t"
        "iretq\n\t"
        ::: "memory"
    );
}

void GlobalAPDoTaskFuncDo(){
    u32 core = GetCurrentCoreId();
    u64 apic_base = rdmsr(0x1B);
    if ((apic_base & (3ULL << 10)) == (3ULL << 10)) {
        wrmsr(0x80B, 0);
    } else {
        u64 apic_addr = apic_base & ~0xFFFULL;
        volatile u32* eoi = (volatile u32*)(apic_addr + 0xB0);
        *eoi = 0;
    }
    if (core < 288 && APAPICIDsBusy[core] == 0 && APCallFunc[core] != NULL){
        __asm__ volatile("cli");
        APAPICIDsBusy[core] = 1;
        if (APCallFuncTarget[core] != NULL) {
            *APCallFuncTarget[core] = APCallFunc[core](APCallFuncArg[core]);
        } else {
            APCallFunc[core](APCallFuncArg[core]);
        }
        APCallFunc[core] = NULL;
        APCallFuncArg[core] = NULL;
        __asm__ volatile("sti");
        APAPICIDsBusy[core] = 0;
        __asm__ volatile("mfence" ::: "memory");
    }
}

u64 GetActualCoreCount(){
    CpuInfoResult Res = CpuInfo(4, 0);
    if (Res.Eax != 0) {
        return ((Res.Eax >> 26) & 0x3F) + 1;
    }
    return 1;
}

u32 GetCurrentApicId(void) {
    u32 eax, ebx, ecx, edx;

    __asm__ volatile("cpuid"
                     : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                     : "a"(0x00));

    if (eax >= 0x0B) {
        __asm__ volatile("cpuid"
                         : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                         : "a"(0x0B), "c"(0));
        if (edx != 0) {
            return edx;
        }
    }

    __asm__ volatile("cpuid"
                     : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                     : "a"(0x01));
    return (ebx >> 24) & 0xFF;
}

u32 GetCurrentCoreId(void) {
    u32 ecx;
    __asm__ volatile("rdtscp" : "=c"(ecx) : : "eax", "edx");
    return ecx;
}

void SetCurrentCoreId(u32 CoreId) {
    __asm__ volatile("wrmsr" : : "c"(0xC0000103), "a"(CoreId), "d"(0));
}

void WaitForAllAPNotBusy(){
    volatile u64* busy_u64 = (volatile u64*)APAPICIDsBusy;
    u32 bsp_core = GetCurrentCoreId();

    u32 bsp_u64_idx = bsp_core / 8;
    u32 bsp_byte_offset = bsp_core % 8;

    u64 bsp_mask = ~(0xFFULL << (bsp_byte_offset * 8));

    #pragma unroll
    for (u32 i = 0; i < (288 / 8); i++) {
        u64 mask = (i == bsp_u64_idx) ? bsp_mask : 0xFFFFFFFFFFFFFFFFULL;

        while ((busy_u64[i] & mask) != 0) {
            __asm__ volatile("pause" ::: "memory");
        }
    }
    __asm__ volatile("mfence" ::: "memory");
}