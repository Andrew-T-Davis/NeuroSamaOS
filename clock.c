#include "clock.h"
#include "debug.h"
#include "int.h"
#include "efi.h"

static u64 g_tsc_frequency = 0;
static u64 g_tsc_boot_offset = 0;
static u64 g_tsc_to_ms_mult = 0; 
static u64 g_tsc_to_ns_mult = 0;
static u64 g_tsc_to_us_mult = 0;
static u64 g_tsc_to_s_mult = 0;
static u64 g_tsc_shift = 32;

f64 DeltaMs=0.0;

u64 rdtsc(void) {
    u32 low, high;
    __asm__ __volatile__("rdtsc" : "=a"(low), "=d"(high));
    return ((u64)high << 32) | low;
}

u64 rdtscp(void) {
    u32 low, high, aux;
    __asm__ __volatile__("rdtscp" : "=a"(low), "=d"(high), "=c"(aux));
    return ((u64)high << 32) | low;
}

u64 rdmsr(u32 msr) {
    u32 low, high;
    __asm__ volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
    return ((u64)high << 32) | low;
}

void wrmsr(u32 msr, u64 value) {
    u32 low = value & 0xFFFFFFFF;
    u32 high = value >> 32;
    __asm__ volatile("wrmsr" : : "a"(low), "d"(high), "c"(msr));
}

u64 rdtsc_serialized(void) {
    u32 low, high;
    __asm__ __volatile__(
        "lfence\n\t"
        "rdtsc\n\t"
        : "=a"(low), "=d"(high)
        :
        : "memory"
    );
    return ((u64)high << 32) | low;
}

u64 CalibrateTscUsingStall(void) {
    u32 eax, ebx, ecx, edx;
    u64 freq = 0;

    __asm__ volatile("cpuid" 
                     : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                     : "a"(0), "c"(0));
    
    if (eax >= 0x15) {
        __asm__ volatile("cpuid" 
                         : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                         : "a"(0x15));
        
        if (ecx != 0 && eax != 0) {
            freq = (u64)ecx * ebx / eax;
            if (freq < 500000000 || freq > 6000000000) {
                freq = 0;
            }
        }
    }
    
    if (freq == 0) {
        extern EFI_BOOT_SERVICES* bs;
        u64 start_tsc, end_tsc;
        u32 dummy;

        __asm__ volatile("cpuid" : "=a"(dummy), "=b"(dummy), "=c"(dummy), "=d"(dummy) : "a"(0));
        start_tsc = rdtsc_serialized();
        bs->Stall(1000000);
        end_tsc = rdtsc_serialized();
        __asm__ volatile("cpuid" : "=a"(dummy), "=b"(dummy), "=c"(dummy), "=d"(dummy) : "a"(0));
        
        freq = end_tsc - start_tsc;
    }
    
    return freq;
}

void InitClock(void) {
    if (g_tsc_frequency == 0) {
        g_tsc_frequency = CalibrateTscUsingStall();
    }
    g_tsc_boot_offset = rdtsc_serialized();
    DebugStr("g_tsc_boot_offset=0x");DebugU64(g_tsc_boot_offset);DebugChar('\n');

    g_tsc_to_s_mult  = (1ULL << g_tsc_shift) / g_tsc_frequency;
    g_tsc_to_ms_mult = (1000ULL << g_tsc_shift) / g_tsc_frequency;
    g_tsc_to_us_mult = (1000000ULL << g_tsc_shift) / g_tsc_frequency;
    g_tsc_to_ns_mult = (1000000000ULL << g_tsc_shift) / g_tsc_frequency;
}

u64 SystemGetTime(void) {
    u64 tsc = rdtsc_serialized() - g_tsc_boot_offset;
    return (tsc * g_tsc_to_us_mult) >> g_tsc_shift;
}

u64 SystemGetTimeMillis(void) {
    u64 tsc = rdtsc_serialized() - g_tsc_boot_offset;
    return (tsc * g_tsc_to_ms_mult) >> g_tsc_shift;
}

u64 SystemGetTimeNano(void) {
    u64 tsc = rdtsc_serialized() - g_tsc_boot_offset;
    
    Cu128 temp = (Cu128)tsc * g_tsc_to_ns_mult;
    return (u64)(temp >> g_tsc_shift);
}

u64 SystemGetTimeS(void) {
    u64 tsc = rdtsc_serialized() - g_tsc_boot_offset;
    return (tsc * g_tsc_to_s_mult) >> g_tsc_shift;
}

u64 GetTscFrequency(void) {
    return g_tsc_frequency;
}

void SetTscFrequency(u64 f){
    g_tsc_frequency=f;
}

void SystemBusySleepUs(u64 Time) {
    u64 target = rdtsc_serialized() + Time * g_tsc_frequency / 1000000;
    while (rdtsc_serialized() < target) {
        __asm__("pause");
    }
}

void SystemBusySleepMs(u64 Time) {
    u64 target = rdtsc_serialized() + Time * g_tsc_frequency / 1000;
    while (rdtsc_serialized() < target) {
        __asm__("pause");
    }
}

void SystemBusySleepS(u64 Time) {
    u64 target = rdtsc_serialized() + Time * g_tsc_frequency;
    DebugStr("Rdtsc=0x");DebugU64(rdtsc_serialized());DebugChar('\n');
    while (rdtsc_serialized() < target) {
        __asm__("pause");
    }
}

void SystemBusySleepNano(u64 Time) {
    u64 target = rdtsc_serialized() + Time * g_tsc_frequency / 1000000000;
    while (rdtsc_serialized() < target) {
        __asm__("pause");
    }
}
