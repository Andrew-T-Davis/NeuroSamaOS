#pragma once

#include "int.h"

const f80 M_PI =  3.1415926535897932384626433832795028841971693993751L;
const f80 M_2PI = 6.2831853071795864769252867665590057683943387987502L;
const f80 M_HALF_PI = 1.5707963267948966192313216916397514420985846996876L;
const f80 M_PI_4 = 0.78539816339744830961566084581987572104929234984378L;
const f80 INV_2PI = 0.15915494309189533576888376337251436203445964574046L;
const f80 INV_PI = 0.31830988618379067153776752674502872406891929148091L;
const f80 LOG2E = 1.44269504088896340735992468100189213742664595415299L;
const f80 LN2 = 0.69314718055994530941723212145817656807550013436026L;

s64 S64Abs(s64 x);
u64 U64Pow(u64 x, u64 exp);
f64 DoublePow(f64 x, u64 time);
f64 DoubleAbs(f64 x);
f64 DoubleMod(f64 x, f64 y);
f64 DoubleMod2PI(f64 x);
f64 DoubleSin(f64 x);
f64 DoubleCos(f64 x);
f32 FloatSin(f32 x);
f32 FloatCos(f32 x);
f64 DoubleExp(f64 x);
void DoubleExp4(const f64* x, f64* result);
f64 DoubleLog(f64 x);
f64 DoubleTanh(f64 x);
f64 DoubleAtan(f64 x);
f64 DoubleAtan2(f64 y, f64 x);
f64 DoubleSqrt(f64 x);
f64 DoubleSinCos(f64 x, f64* cos_out);
void DoubleSin4(const f64* x, f64* result);
void DoubleCos4(const f64* x, f64* result);
f64 DoubleRound(f64 val, int round_to);
f32 FloatRound(f32 val, int round_to);
f64 DoubleTan(f64 x);
f32 FloatTan(f64 x);