#pragma once

#include "int.h"

extern f64 DeltaMs;

u64 rdtsc(void);
u64 rdtscp(void);
u64 rdmsr(u32 msr);
void wrmsr(u32 msr, u64 value);
u64 rdtsc_serialized(void);
u64 CalibrateTscUsingStall(void);

void InitClock(void);

u64 SystemGetTime(void);
u64 SystemGetTimeS(void);
u64 SystemGetTimeUs(void);
u64 SystemGetTimeNano(void);
u64 SystemGetTimeMillis(void);
u64 GetTscFrequency(void);
void SetTscFrequency(u64 f);

void SystemBusySleepS(u64 Time);
void SystemBusySleepUs(u64 Time);
void SystemBusySleepNano(u64 Time);
void SystemBusySleepMs(u64 Time);

