#pragma once

#include "int.h"

typedef struct {
    union {
        u8 U;
        struct {
            u8 Mantissa : 4;
            u8 Point    : 3;
            u8 MSB      : 1;
        };
    };
} FP8;

typedef struct {
    union {
        u16 U;
        struct {
            u16 Mantissa : 11;
            u16 Point    : 4;
            u16 MSB      : 1;
        };
    };
} FP16;

typedef struct {
    union {
        u32 U;
        struct {
            u32 Mantissa : 26;
            u32 Point    : 5;
            u32 MSB      : 1;
        };
    };
} FP32;

typedef struct {
    union {
        u64 U;
        struct {
            u64 Mantissa : 57;
            u64 Point    : 6;
            u64 MSB      : 1;
        };
    };
} FP64;

typedef struct {
    union {
        u64 U;
        struct {
            u64 Float    : 59;
            u64 Int      : 4;
            u64 MSB      : 1;
        };
    };
} S1Q4F59;

typedef struct {
    union {
        u64 U;
        struct {
            u64 Float    : 60;
            u64 Int      : 4;
        };
    };
} Q4F59;

typedef struct {
    union {
        u64 U;
        struct {
            u64 Float    : 58;
            u64 Int      : 5;
            u64 MSB      : 1;
        };
    };
} S1Q5F58;

typedef struct {
    union {
        u64 U;
        struct {
            u64 Float    : 59;
            u64 Int      : 5;
        };
    };
} Q5F59;

void FP8Align(FP8* F1, FP8* F2);
void FP16Align(FP16* F1, FP16* F2);
void FP32Align(FP32* F1, FP32* F2);
void FP64Align(FP64* F1, FP64* F2);

Bool FP8PrecisionIs(FP8 F1, FP8 F2);
Bool FP16PrecisionIs(FP16 F1, FP16 F2);
Bool FP32PrecisionIs(FP32 F1, FP32 F2);
Bool FP64PrecisionIs(FP64 F1, FP64 F2);
Bool S1Q4F59PrecisionIs(S1Q4F59 F1, S1Q4F59 F2);
Bool S1Q5F58PrecisionIs(S1Q4F59 F1, S1Q4F59 F2);

Bool FP8Is(FP8 F1, FP8 F2);
Bool FP16Is(FP16 F1, FP16 F2);
Bool FP32Is(FP32 F1, FP32 F2);
Bool FP64Is(FP64 F1, FP64 F2);
Bool S1Q4F59Is(S1Q4F59 F1, S1Q4F59 F2);
Bool S1Q5F58Is(S1Q5F58 F1, S1Q5F58 F2);

FP8 FP8Add(FP8 F1, FP8 F2);
FP16 FP16Add(FP16 F1, FP16 F2);
FP32 FP32Add(FP32 F1, FP32 F2);
FP64 FP64Add(FP64 F1, FP64 F2);
S1Q4F59 S1Q4F59Add(S1Q4F59 F1, S1Q4F59 F2);
S1Q5F58 S1Q5F58Add(S1Q5F58 F1, S1Q5F58 F2);

void FP8Split(FP8 F, u8* integer, u8* fraction);
void FP16Split(FP16 F, u16* integer, u16* fraction);
void FP32Split(FP32 F, u32* integer, u32* fraction);
void FP64Split(FP64 F, u64* integer, u64* fraction);
void S1Q4F59Split(S1Q4F59 F, u64* integer, u64* fraction);
void S1Q5F58Split(S1Q5F58 F, u64* integer, u64* fraction);

FP8 FP8Sub(FP8 F1, FP8 F2);
FP16 FP16Sub(FP16 F1, FP16 F2);
FP32 FP32Sub(FP32 F1, FP32 F2);
FP64 FP64Sub(FP64 F1, FP64 F2);
S1Q4F59 S1Q4F59Sub(S1Q4F59 F1, S1Q4F59 F2);
S1Q5F58 S1Q5F58Sub(S1Q5F58 F1, S1Q5F58 F2);

Bool FP8IsExact(FP8 F1, FP8 F2);
Bool FP16IsExact(FP16 F1, FP16 F2);
Bool FP32IsExact(FP32 F1, FP32 F2);
Bool FP64IsExact(FP64 F1, FP64 F2);
Bool S1Q4F59IsExact(S1Q4F59 F1, S1Q4F59 F2);
Bool S1Q5F58IsExact(S1Q5F58 F1, S1Q5F58 F2);