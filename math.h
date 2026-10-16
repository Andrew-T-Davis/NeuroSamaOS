#pragma once

#include "int.h"

extern const f80 M_PI;
extern const f80 M_2PI;
extern const f80 M_HALF_PI;
extern const f80 M_PI_4;
extern const f80 INV_2PI;
extern const f80 INV_PI;
extern const f80 LOG2E;
extern const f80 LN2;

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