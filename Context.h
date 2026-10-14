#pragma once

#include "int.h"

typedef struct {
    u64 Reg0t15[16];
    u64 cs, ds, es, fs, gs, ss;
    u64 rflags;
    u64 rip;
    u8 SIMDBuffer[2048] __attribute__((aligned(64)));
} __attribute__((aligned(64))) RegContext;

void SaveContext(RegContext *ctx);
void LoadContext(RegContext *ctx);
