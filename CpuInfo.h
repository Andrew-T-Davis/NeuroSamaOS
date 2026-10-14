#pragma once

#include "int.h"

typedef struct {
    u32 Eax;
    u32 Ebx;
    u32 Ecx;
    u32 Edx;
} __attribute__((packed)) CpuInfoResult;

__attribute__((used)) static inline CpuInfoResult CpuInfo(u32 Leaf, u32 SubLeaf){
    CpuInfoResult Res;
    __asm__ volatile (
        "cpuid"
        : "=a" (Res.Eax),
          "=b" (Res.Ebx),
          "=c" (Res.Ecx),
          "=d" (Res.Edx)
        : "a" (Leaf), 
          "c" (SubLeaf)
    );
    return Res;
}