#include "NeuroFloat.h"
#include "int.h"

void FP8Align(FP8* F1, FP8* F2) {
    if (F1->Point > F2->Point) {
        F2->Mantissa <<= (F1->Point - F2->Point);
        F2->Point = F1->Point;
    } else if (F2->Point > F1->Point) {
        F1->Mantissa <<= (F2->Point - F1->Point);
        F1->Point = F2->Point;
    }
}

void FP16Align(FP16* F1, FP16* F2) {
    if (F1->Point > F2->Point) {
        F2->Mantissa <<= (F1->Point - F2->Point);
        F2->Point = F1->Point;
    } else if (F2->Point > F1->Point) {
        F1->Mantissa <<= (F2->Point - F1->Point);
        F1->Point = F2->Point;
    }
}

void FP32Align(FP32* F1, FP32* F2) {
    if (F1->Point > F2->Point) {
        F2->Mantissa <<= (F1->Point - F2->Point);
        F2->Point = F1->Point;
    } else if (F2->Point > F1->Point) {
        F1->Mantissa <<= (F2->Point - F1->Point);
        F1->Point = F2->Point;
    }
}

void FP64Align(FP64* F1, FP64* F2) {
    if (F1->Point > F2->Point) {
        F2->Mantissa <<= (F1->Point - F2->Point);
        F2->Point = F1->Point;
    } else if (F2->Point > F1->Point) {
        F1->Mantissa <<= (F2->Point - F1->Point);
        F1->Point = F2->Point;
    }
}

Bool FP8PrecisionIs(FP8 F1, FP8 F2){
    return F1.Point == F2.Point;
}

Bool FP16PrecisionIs(FP16 F1, FP16 F2){
    return F1.Point == F2.Point;
}

Bool FP32PrecisionIs(FP32 F1, FP32 F2){
    return F1.Point == F2.Point;
}

Bool FP64PrecisionIs(FP64 F1, FP64 F2){
    return F1.Point == F2.Point;
}

Bool S1Q4F59PrecisionIs(S1Q4F59 F1, S1Q4F59 F2){
    (void)F1;
    (void)F2;
    return 1;
}

Bool FP8Is(FP8 F1, FP8 F2){
    if (F1.MSB != F2.MSB) {
        return (F1.Mantissa == 0 && F2.Mantissa == 0);
    }
    if (F1.Point == F2.Point) {
        return F1.Mantissa == F2.Mantissa;
    }
    if (F1.Point < F2.Point) {
        u64 diff = (u64)F2.Point - F1.Point;
        return ((u64)F1.Mantissa << diff) == (u64)F2.Mantissa;
    } else {
        u64 diff = (u64)F1.Point - F2.Point;
        return (u64)F1.Mantissa == ((u64)F2.Mantissa << diff);
    }
}

Bool FP16Is(FP16 F1, FP16 F2){
    if (F1.MSB != F2.MSB) {
        return (F1.Mantissa == 0 && F2.Mantissa == 0);
    }
    if (F1.Point == F2.Point) {
        return F1.Mantissa == F2.Mantissa;
    }
    if (F1.Point < F2.Point) {
        u64 diff = (u64)F2.Point - F1.Point;
        return ((u64)F1.Mantissa << diff) == (u64)F2.Mantissa;
    } else {
        u64 diff = (u64)F1.Point - F2.Point;
        return (u64)F1.Mantissa == ((u64)F2.Mantissa << diff);
    }
}

Bool FP32Is(FP32 F1, FP32 F2){
    if (F1.MSB != F2.MSB) {
        return (F1.Mantissa == 0 && F2.Mantissa == 0);
    }
    if (F1.Point == F2.Point) {
        return F1.Mantissa == F2.Mantissa;
    }
    if (F1.Point < F2.Point) {
        u64 diff = (u64)F2.Point - F1.Point;
        return ((u64)F1.Mantissa << diff) == (u64)F2.Mantissa;
    } else {
        u64 diff = (u64)F1.Point - F2.Point;
        return (u64)F1.Mantissa == ((u64)F2.Mantissa << diff);
    }
}

Bool FP64Is(FP64 F1, FP64 F2){
    if (F1.MSB != F2.MSB) {
        return (F1.Mantissa == 0 && F2.Mantissa == 0);
    }
    if (F1.Point == F2.Point) {
        return F1.Mantissa == F2.Mantissa;
    }
    if (F1.Point < F2.Point) {
        u64 diff = F2.Point - F1.Point;
        if (diff >= 64) return 0; 
        return (F1.Mantissa << diff) == F2.Mantissa;
    } else {
        u64 diff = F1.Point - F2.Point;
        if (diff >= 64) return 0;
        return F1.Mantissa == (F2.Mantissa << diff);
    }
}

Bool S1Q4F59Is(S1Q4F59 F1, S1Q4F59 F2){
    return (F1.U == F2.U);
}

FP8 FP8Add(FP8 F1, FP8 F2){
    FP8 result;
    FP8Align(&F1, &F2);
    s32 v1 = F1.MSB ? -(s32)F1.Mantissa : (s32)F1.Mantissa;
    s32 v2 = F2.MSB ? -(s32)F2.Mantissa : (s32)F2.Mantissa;
    s32 sum = v1 + v2;
    if (sum < 0) {
        result.MSB = 1;
        result.Mantissa = (u8)(-sum);
    } else {
        result.MSB = 0;
        result.Mantissa = (u8)sum;
    }
    result.Point = F1.Point;
    return result;
}

FP16 FP16Add(FP16 F1, FP16 F2){
    FP16 result;
    FP16Align(&F1, &F2);
    s32 v1 = F1.MSB ? -(s32)F1.Mantissa : (s32)F1.Mantissa;
    s32 v2 = F2.MSB ? -(s32)F2.Mantissa : (s32)F2.Mantissa;
    s32 sum = v1 + v2;
    if (sum < 0) {
        result.MSB = 1;
        result.Mantissa = (u16)(-sum);
    } else {
        result.MSB = 0;
        result.Mantissa = (u16)sum;
    }
    result.Point = F1.Point;
    return result;
}

FP32 FP32Add(FP32 F1, FP32 F2){
    FP32 result;
    FP32Align(&F1, &F2);
    s64 v1 = F1.MSB ? -(s64)F1.Mantissa : (s64)F1.Mantissa;
    s64 v2 = F2.MSB ? -(s64)F2.Mantissa : (s64)F2.Mantissa;
    s64 sum = v1 + v2;
    if (sum < 0) {
        result.MSB = 1;
        result.Mantissa = (u32)(-sum);
    } else {
        result.MSB = 0;
        result.Mantissa = (u32)sum;
    }
    result.Point = F1.Point;
    return result;
}

FP64 FP64Add(FP64 F1, FP64 F2){
    FP64 result;
    FP64Align(&F1, &F2);
    s64 v1 = F1.MSB ? -(s64)F1.Mantissa : (s64)F1.Mantissa;
    s64 v2 = F2.MSB ? -(s64)F2.Mantissa : (s64)F2.Mantissa;
    s64 sum = v1 + v2;
    if (sum < 0) {
        result.MSB = 1;
        result.Mantissa = (u64)(-sum);
    } else {
        result.MSB = 0;
        result.Mantissa = (u64)sum;
    }
    result.Point = F1.Point;
    return result;
}

S1Q4F59 S1Q4F59Add(S1Q4F59 F1, S1Q4F59 F2){
    S1Q4F59 Ret = {0};
    Ret.U = (F1.U + F2.U);
    return Ret;
}

void FP8Split(FP8 F, u8* integer, u8* fraction) {
    if (integer) {
        *integer = F.Mantissa >> F.Point;
    }
    if (fraction) {
        *fraction = F.Mantissa & ((1 << F.Point) - 1);
    }
}

void FP16Split(FP16 F, u16* integer, u16* fraction) {
    if (integer) {
        *integer = F.Mantissa >> F.Point;
    }
    if (fraction) {
        *fraction = F.Mantissa & ((1 << F.Point) - 1);
    }
}

void FP32Split(FP32 F, u32* integer, u32* fraction) {
    if (integer) {
        *integer = F.Mantissa >> F.Point;
    }
    if (fraction) {
        *fraction = F.Mantissa & ((1 << F.Point) - 1);
    }
}

void FP64Split(FP64 F, u64* integer, u64* fraction) {
    if (F.Point >= 64) {
        if (integer) {
            *integer = 0;
        }
        if (fraction) {
            *fraction = F.Mantissa;
        }
    } else {
        if (integer) {
            *integer = F.Mantissa >> F.Point;
        }
        if (fraction) {
            *fraction = F.Mantissa & ((1ULL << F.Point) - 1);
        }
    }
}

void S1Q4F59Split(S1Q4F59 F, u64* integer, u64* fraction){
    *integer = F.Int;
    *fraction = F.Float;
}

FP8 FP8Sub(FP8 F1, FP8 F2){
    FP8 result;
    FP8Align(&F1, &F2);
    F2.MSB = !F2.MSB;
    s32 v1 = F1.MSB ? -(s32)F1.Mantissa : (s32)F1.Mantissa;
    s32 v2 = F2.MSB ? -(s32)F2.Mantissa : (s32)F2.Mantissa;
    s32 sum = v1 + v2;
    if (sum < 0) {
        result.MSB = 1;
        result.Mantissa = (u8)(-sum);
    } else {
        result.MSB = 0;
        result.Mantissa = (u8)sum;
    }
    result.Point = F1.Point;
    return result;
}

FP16 FP16Sub(FP16 F1, FP16 F2){
    FP16 result;
    FP16Align(&F1, &F2);
    F2.MSB = !F2.MSB;
    s32 v1 = F1.MSB ? -(s32)F1.Mantissa : (s32)F1.Mantissa;
    s32 v2 = F2.MSB ? -(s32)F2.Mantissa : (s32)F2.Mantissa;
    s32 sum = v1 + v2;
    if (sum < 0) {
        result.MSB = 1;
        result.Mantissa = (u16)(-sum);
    } else {
        result.MSB = 0;
        result.Mantissa = (u16)sum;
    }
    result.Point = F1.Point;
    return result;
}

FP32 FP32Sub(FP32 F1, FP32 F2){
    FP32 result;
    FP32Align(&F1, &F2);
    F2.MSB = !F2.MSB;
    s64 v1 = F1.MSB ? -(s64)F1.Mantissa : (s64)F1.Mantissa;
    s64 v2 = F2.MSB ? -(s64)F2.Mantissa : (s64)F2.Mantissa;
    s64 sum = v1 + v2;
    if (sum < 0) {
        result.MSB = 1;
        result.Mantissa = (u32)(-sum);
    } else {
        result.MSB = 0;
        result.Mantissa = (u32)sum;
    }
    result.Point = F1.Point;
    return result;
}

FP64 FP64Sub(FP64 F1, FP64 F2){
    FP64 result;
    FP64Align(&F1, &F2);
    F2.MSB = !F2.MSB;
    s64 v1 = F1.MSB ? -(s64)F1.Mantissa : (s64)F1.Mantissa;
    s64 v2 = F2.MSB ? -(s64)F2.Mantissa : (s64)F2.Mantissa;
    s64 sum = v1 + v2;
    if (sum < 0) {
        result.MSB = 1;
        result.Mantissa = (u64)(-sum);
    } else {
        result.MSB = 0;
        result.Mantissa = (u64)sum;
    }
    result.Point = F1.Point;
    return result;
}

S1Q4F59 S1Q4F59Sub(S1Q4F59 F1, S1Q4F59 F2){
    return (S1Q4F59){.U=F1.U - F2.U};
}

Bool FP8IsExact(FP8 F1, FP8 F2){
    return F1.U == F2.U;
}

Bool FP16IsExact(FP16 F1, FP16 F2){
    return F1.U == F2.U;
}

Bool FP32IsExact(FP32 F1, FP32 F2){
    return F1.U == F2.U;
}

Bool FP64IsExact(FP64 F1, FP64 F2){
    return F1.U == F2.U;
}

Bool S1Q4F59IsExact(S1Q4F59 F1, S1Q4F59 F2){
    return F1.U == F2.U;
}

Bool S1Q5F58PrecisionIs(S1Q4F59 F1, S1Q4F59 F2){
    (void)F1;
    (void)F2;
    return 1;
}

Bool S1Q5F58Is(S1Q5F58 F1, S1Q5F58 F2){
    return F1.U==F2.U;
}

S1Q5F58 S1Q5F58Add(S1Q5F58 F1, S1Q5F58 F2){
    return (S1Q5F58){.U=F1.U+F2.U};
}

void S1Q5F58Split(S1Q5F58 F, u64* integer, u64* fraction){
    *integer=(u64)F.MSB<<63|F.Int;
    *fraction=(u64)F.Float;
}

S1Q5F58 S1Q5F58Sub(S1Q5F58 F1, S1Q5F58 F2){
    return (S1Q5F58){.U=F1.U-F2.U};
}

Bool S1Q5F58IsExact(S1Q5F58 F1, S1Q5F58 F2){
    return F1.U == F2.U;
}