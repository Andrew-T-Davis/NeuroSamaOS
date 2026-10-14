#pragma once

#include "int.h"
#include "clock.h"

extern volatile u64 APStackSPs[288];
extern volatile u64 APStackBPs[288];
extern volatile u32 APAPICIDs[288];
extern volatile u8 APAPICIDsBusy[288];

extern void* (*APCallFunc[288])(void* ArgList);
extern void* APCallFuncArg[288];
extern void** APCallFuncTarget[288];

__attribute__((used)) static inline void SendIPI(u64 ApicId, u8 Vector) {
    u64 apic_base = rdmsr(0x1B);
    Bool is_x2apic = ((apic_base & (1ULL << 10)) != 0) && 
                              ((apic_base & (1ULL << 11)) != 0);
    
    if (is_x2apic) {
        u64 icr_val = ((u64)ApicId << 32) | 0x00004000ULL | Vector;
        wrmsr(0x830, icr_val);
        
        while (rdmsr(0x830) & (1ULL << 12)) {
            __asm__ volatile("pause" ::: "memory");
        }
    } else {
        u64 apic_base_mmio = apic_base & ~0xFFFULL;
        volatile u32* IcrLow  = (volatile u32*)(apic_base_mmio + 0x300);
        volatile u32* IcrHigh = (volatile u32*)(apic_base_mmio + 0x310);
        
        while (*IcrLow & (1 << 12)) {
            __asm__ volatile("pause" ::: "memory");
        }
        
        *IcrHigh = (u32)(ApicId << 24);
        __asm__ volatile("mfence" ::: "memory");
        *IcrLow = 0x00004000 | Vector;
        while (*IcrLow & (1 << 12)) {
            __asm__ volatile("pause" ::: "memory");
        }
    }
}

void GlobalAPDoTaskFunc();
u64 GetActualCoreCount();
u32 GetCurrentApicId(void);
u32 GetCurrentCoreId(void);
void WaitForAllAPNotBusy();
void SetCurrentCoreId(u32 CoreId);