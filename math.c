#include "math.h"
#include "int.h"

const f64 DOUBLE_ONE = 1.0;
const f32 FLOAT_ONE = 1.0f;
const f80 inv_2pi = INV_2PI;
const f80 two_pi = M_2PI;
const f80 log_2e = LOG2E;
const f80 inv_pi = INV_PI;

const f64 CONST_1_OVER_6 = 1.0/6.0;
const f64 CONST_1_OVER_120 = 1.0/120.0;
const f64 CONST_1_OVER_5040 = 1.0/5040.0;
const f64 CONST_1_OVER_362880 = 1.0/362880.0;
const f64 CONST_1_OVER_40320 = 1.0/40320.0;
const f64 CONST_1_OVER_720 = 1.0/720.0;
const f64 CONST_1_OVER_24 = 1.0/24.0;
const f64 CONST_1_OVER_3628800 = 1.0/3628800.0;
const f64 CONST_1_OVER_39916800 = 1.0/39916800.0;
const f64 CONST_1_OVER_479001600 = 1.0/479001600.0;
const f64 CONST_1_OVER_87178291200 = 1.0/87178291200.0;
const f64 CONST_1_OVER_3 = 1.0/3.0;
const f64 CONST_1_OVER_5 = 1.0/5.0;
const f64 CONST_1_OVER_7 = 1.0/7.0;
const f64 CONST_1_OVER_9 = 1.0/9.0;
const f64 CONST_1_OVER_11 = 1.0/11.0;
const f64 CONST_1_OVER_13 = 1.0/13.0;
const f64 CONST_0_5 = 0.5;
const f64 CONST_1 = 1.0;

static const f64 SIN_C1 = (f64)0.16666666666666666666666666666666666666666666666667;
static const f64 SIN_C2 = (f64)0.0083333333333333333333333333333333333333333333333333;
static const f64 SIN_C3 = (f64)0.0001984126984126984126984126984126984126984126984127;
static const f64 SIN_C4 = (f64)0.0000027557319223985890652557319223985890652557319223986;
static const f64 SIN_C5 = (f64)0.000000025052108385441718775086526454433405762264671030183;
static const f64 SIN_C6 = (f64)0.00000000016059043836821614599367051893912127525694291685921;

static const f64 INV_2PI_F64 = (f64)INV_2PI;
static const f64 TWO_PI_F64 = (f64)M_2PI;
static const f64 PI_F64 = (f64)M_PI;
static const f64 HALF_PI_F64 = (f64)M_HALF_PI;
static const f64 LOG2E_F64 = (f64)LOG2E;
static const f64 LN2_F64 = (f64)LN2;

s64 S64Abs(s64 x) {
    return (x < 0) ? -x : x;
}

u64 U64Pow(u64 x, u64 exp) {
    u64 result = 1;
    while (exp) {
        if (exp & 1) result *= x;
        x *= x;
        exp >>= 1;
    }
    return result;
}

f64 DoublePow(f64 x, u64 time) {
    f64 result = 1.0;
    f64 base = x;
    while (time > 0) {
        if (time & 1) result *= base;
        base *= base;
        time >>= 1;
    }
    return result;
}

f64 DoubleAbs(f64 x) {
    const unsigned long long mask = 0x7FFFFFFFFFFFFFFFULL;
    f64 result;
    __asm__ volatile (
        "vmovsd %1, %%xmm0\n\t"
        "vpand %2, %%xmm0, %%xmm0\n\t"
        "vmovsd %%xmm0, %0"
        : "=m"(result)
        : "m"(x), "m"(mask)
        : "xmm0", "memory"
    );
    return result;
}

f64 DoubleMod(f64 x, f64 y) {
    if (y == 0.0 || x == 0.0) return 0.0;
    f64 result;
    __asm__ volatile (
        "vmovsd %1, %%xmm0\n\t"
        "vmovsd %2, %%xmm1\n\t"
        "vdivsd %%xmm1, %%xmm0, %%xmm0\n\t"
        "vroundsd $0x09, %%xmm0, %%xmm0, %%xmm0\n\t"
        "vmulsd %%xmm1, %%xmm0, %%xmm0\n\t"
        "vmovsd %1, %%xmm1\n\t"
        "vsubsd %%xmm0, %%xmm1, %%xmm1\n\t"
        "vmovsd %%xmm1, %0"
        : "=m"(result)
        : "m"(x), "m"(y)
        : "xmm0", "xmm1", "memory"
    );
    return result;
}

f64 DoubleMod2PI(f64 x) {
    f64 result;
    f64 inv2pi = INV_2PI_F64;
    f64 twopi = TWO_PI_F64;
    __asm__ volatile (
        "vmovsd %1, %%xmm0\n\t"
        "vmulsd %2, %%xmm0, %%xmm0\n\t"
        "vroundsd $0x09, %%xmm0, %%xmm0, %%xmm0\n\t"
        "vmulsd %3, %%xmm0, %%xmm0\n\t"
        "vmovsd %1, %%xmm1\n\t"
        "vsubsd %%xmm0, %%xmm1, %%xmm1\n\t"
        "vmovsd %%xmm1, %0"
        : "=m"(result)
        : "m"(x), "m"(inv2pi), "m"(twopi)
        : "xmm0", "xmm1", "memory"
    );
    return result;
}

f64 DoubleSin(f64 x) {
    if (x != x) return x;
    if (x == 0) return 0;
    
    f64 result;
    f64 inv2pi = INV_2PI_F64;
    f64 twopi = TWO_PI_F64;
    f64 pi = PI_F64;
    f64 half_pi = HALF_PI_F64;
    f64 one = 1.0;
    s64 n;
    
    const f64 c1 = -0.16666666666666666666666666666666666666666666666667;
    const f64 c2 =  0.0083333333333333333333333333333333333333333333333333;
    const f64 c3 = -0.0001984126984126984126984126984126984126984126984127;
    const f64 c4 =  0.0000027557319223985890652557319223985890652557319223986;
    const f64 c5 = -0.000000025052108385441718775086526454433405762264671030183;
    const f64 c6 =  0.00000000016059043836821614599367051893912127525694291685921;
    
    __asm__ volatile (
        "fldl %1\n\t"
        "fldl %2\n\t"
        "fmulp\n\t"
        "fistpll %0\n\t"
        : "=m"(n)
        : "m"(x), "m"(inv2pi)
        : "st", "st(1)", "memory"
    );

    f64 nx = x - (f64)n * twopi;
    
    if (nx > pi) {
        n++;
        nx = x - (f64)n * twopi;
    } else if (nx < -pi) {
        n--;
        nx = x - (f64)n * twopi;
    }
    
    f64 sign = 1.0;
    if (nx < 0) {
        nx = -nx;
        sign = -1.0;
    }
    if (nx > pi) {
        nx = twopi - nx;
        sign = -sign;
    }
    if (nx > half_pi) {
        nx = pi - nx;
    }
    
    __asm__ volatile (
        "movsd %1, %%xmm0\n\t"
        "movsd %2, %%xmm1\n\t"
        "movsd %3, %%xmm2\n\t"
        "movsd %4, %%xmm3\n\t"
        "movsd %5, %%xmm4\n\t"
        "movsd %6, %%xmm5\n\t"
        "movsd %7, %%xmm6\n\t"
        "movsd %8, %%xmm7\n\t"
        "movsd %9, %%xmm8\n\t"
        
        "movsd %%xmm0, %%xmm9\n\t"
        "mulsd %%xmm9, %%xmm9\n\t"
        
        "movsd %%xmm6, %%xmm10\n\t"
        "mulsd %%xmm9, %%xmm10\n\t"
        "addsd %%xmm5, %%xmm10\n\t"
        "mulsd %%xmm9, %%xmm10\n\t"
        "addsd %%xmm4, %%xmm10\n\t"
        "mulsd %%xmm9, %%xmm10\n\t"
        "addsd %%xmm3, %%xmm10\n\t"
        "mulsd %%xmm9, %%xmm10\n\t"
        "addsd %%xmm2, %%xmm10\n\t"
        "mulsd %%xmm9, %%xmm10\n\t"
        "addsd %%xmm1, %%xmm10\n\t"
        "mulsd %%xmm9, %%xmm10\n\t"
        "addsd %%xmm7, %%xmm10\n\t"
        
        "mulsd %%xmm0, %%xmm10\n\t"
        "mulsd %%xmm8, %%xmm10\n\t"
        "movsd %%xmm10, %0"
        : "=m"(result)
        : "m"(nx), "m"(c1), "m"(c2), "m"(c3), "m"(c4), "m"(c5), "m"(c6),
          "m"(one), "m"(sign)
        : "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5",
          "xmm6", "xmm7", "xmm8", "xmm9", "xmm10", "memory"
    );
    
    if (result == 0.0) return 0.0;
    return result;
}

f64 DoubleCos(f64 x) {
    return DoubleSin(x + (f64)M_HALF_PI);
}

f32 FloatSin(f32 x) {
    return (f32)DoubleSin(x);
}

f32 FloatCos(f32 x) {
    return (f32)DoubleCos(x);
}

f64 DoubleExp(f64 x) {
    f64 result;
    f64 log2e = LOG2E_F64;
    f64 ln2 = LN2_F64;
    f64 c40320 = CONST_1_OVER_40320;
    f64 c5040 = CONST_1_OVER_5040;
    f64 c720 = CONST_1_OVER_720;
    f64 c120 = CONST_1_OVER_120;
    f64 c24 = CONST_1_OVER_24;
    f64 c6 = CONST_1_OVER_6;
    f64 c0_5 = CONST_0_5;
    f64 one = DOUBLE_ONE;
    
    register f64 x_reg __asm__("xmm0") = x;
    register f64 log2e_reg __asm__("xmm5") = log2e;
    register f64 ln2_reg __asm__("xmm6") = ln2;
    register f64 c40320_reg __asm__("xmm7") = c40320;
    register f64 c5040_reg __asm__("xmm8") = c5040;
    register f64 c720_reg __asm__("xmm9") = c720;
    register f64 c120_reg __asm__("xmm10") = c120;
    register f64 c24_reg __asm__("xmm11") = c24;
    register f64 c6_reg __asm__("xmm12") = c6;
    register f64 c0_5_reg __asm__("xmm13") = c0_5;
    register f64 one_reg __asm__("xmm14") = one;
    
    __asm__ volatile (
        "movsd %%xmm0, %%xmm1\n\t"
        "mulsd %%xmm5, %%xmm1\n\t"
        "movsd %%xmm1, %%xmm2\n\t"
        "roundsd $0x09, %%xmm2, %%xmm2\n\t"
        "subsd %%xmm2, %%xmm1\n\t"
        "cvttsd2si %%xmm2, %%rax\n\t"
        "mulsd %%xmm6, %%xmm1\n\t"
        "movsd %%xmm1, %%xmm2\n\t"
        "mulsd %%xmm2, %%xmm2\n\t"
        "movsd %%xmm7, %%xmm3\n\t"
        "mulsd %%xmm2, %%xmm3\n\t"
        "addsd %%xmm8, %%xmm3\n\t"
        "mulsd %%xmm2, %%xmm3\n\t"
        "addsd %%xmm9, %%xmm3\n\t"
        "mulsd %%xmm2, %%xmm3\n\t"
        "addsd %%xmm10, %%xmm3\n\t"
        "mulsd %%xmm2, %%xmm3\n\t"
        "addsd %%xmm11, %%xmm3\n\t"
        "mulsd %%xmm2, %%xmm3\n\t"
        "addsd %%xmm12, %%xmm3\n\t"
        "mulsd %%xmm2, %%xmm3\n\t"
        "addsd %%xmm13, %%xmm3\n\t"
        "mulsd %%xmm1, %%xmm3\n\t"
        "addsd %%xmm1, %%xmm3\n\t"
        "addsd %%xmm14, %%xmm3\n\t"
        "addq $1023, %%rax\n\t"
        "salq $52, %%rax\n\t"
        "movq %%rax, %%xmm4\n\t"
        "mulsd %%xmm4, %%xmm3\n\t"
        "movsd %%xmm3, %0"
        : "=m"(result)
        : "x"(x_reg), "x"(log2e_reg), "x"(ln2_reg),
          "x"(c40320_reg), "x"(c5040_reg), "x"(c720_reg),
          "x"(c120_reg), "x"(c24_reg), "x"(c6_reg),
          "x"(c0_5_reg), "x"(one_reg)
        : "rax", "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", 
          "xmm5", "xmm6", "xmm7", "xmm8", "xmm9", "xmm10",
          "xmm11", "xmm12", "xmm13", "xmm14", "memory"
    );
    return result;
}

void DoubleExp4(const f64* x, f64* result) {
    f64 log2e = LOG2E_F64;
    f64 ln2 = LN2_F64;
    __asm__ volatile (
        "vmovupd %1, %%ymm0\n\t"
        "vbroadcastsd %2, %%ymm1\n\t"
        "vmulpd %%ymm1, %%ymm0, %%ymm0\n\t"
        "vroundpd $0x09, %%ymm0, %%ymm1\n\t"
        "vsubpd %%ymm1, %%ymm0, %%ymm0\n\t"
        "vbroadcastsd %3, %%ymm1\n\t"
        "vmulpd %%ymm1, %%ymm0, %%ymm0\n\t"
        "vmulpd %%ymm0, %%ymm0, %%ymm1\n\t"
        "vbroadcastsd %4, %%ymm2\n\t"
        "vmulpd %%ymm1, %%ymm2, %%ymm2\n\t"
        "vbroadcastsd %5, %%ymm3\n\t"
        "vaddpd %%ymm3, %%ymm2, %%ymm2\n\t"
        "vmulpd %%ymm1, %%ymm2, %%ymm2\n\t"
        "vbroadcastsd %6, %%ymm3\n\t"
        "vaddpd %%ymm3, %%ymm2, %%ymm2\n\t"
        "vmulpd %%ymm1, %%ymm2, %%ymm2\n\t"
        "vbroadcastsd %7, %%ymm3\n\t"
        "vaddpd %%ymm3, %%ymm2, %%ymm2\n\t"
        "vmulpd %%ymm1, %%ymm2, %%ymm2\n\t"
        "vbroadcastsd %8, %%ymm3\n\t"
        "vaddpd %%ymm3, %%ymm2, %%ymm2\n\t"
        "vmulpd %%ymm1, %%ymm2, %%ymm2\n\t"
        "vbroadcastsd %9, %%ymm3\n\t"
        "vaddpd %%ymm3, %%ymm2, %%ymm2\n\t"
        "vmulpd %%ymm1, %%ymm2, %%ymm2\n\t"
        "vbroadcastsd %10, %%ymm3\n\t"
        "vaddpd %%ymm3, %%ymm2, %%ymm2\n\t"
        "vmulpd %%ymm1, %%ymm2, %%ymm2\n\t"
        "vbroadcastsd %10, %%ymm3\n\t"
        "vaddpd %%ymm3, %%ymm2, %%ymm2\n\t"
        "vmulpd %%ymm1, %%ymm2, %%ymm2\n\t"
        "vbroadcastsd %10, %%ymm3\n\t"
        "vaddpd %%ymm3, %%ymm2, %%ymm2\n\t"
        "vaddpd %%ymm0, %%ymm2, %%ymm0\n\t"
        "vmovupd %%ymm0, %0"
        : "=m"(result[0])
        : "m"(x[0]), "m"(log2e), "m"(ln2),
          "m"(CONST_1_OVER_5040),
          "m"(CONST_1_OVER_720),
          "m"(CONST_1_OVER_120),
          "m"(CONST_1_OVER_24),
          "m"(CONST_1_OVER_6),
          "m"(CONST_0_5),
          "m"(CONST_1)
        : "ymm0", "ymm1", "ymm2", "ymm3", "memory"
    );
}

f64 DoubleLog(f64 x) {
    if (x <= 0.0) return -1.0 / 0.0;
    
    union { f64 d; u64 u; } v = { .d = x };
    int exp = (int)((v.u >> 52) & 0x7FF) - 1023;
    v.u = (v.u & 0x800FFFFFFFFFFFFFULL) | 0x3FF0000000000000ULL;
    f64 m = v.d;
    
    f64 z = (m - 1.0) / (m + 1.0);
    f64 z2 = z * z;
    f64 result;
    f64 ln2 = LN2_F64;
    
    __asm__ volatile (
        "vmovsd %1, %%xmm0\n\t"
        "vmovsd %2, %%xmm1\n\t"
        "vmulsd %%xmm1, %%xmm0, %%xmm2\n\t"
        "vmulsd %%xmm1, %%xmm2, %%xmm2\n\t"
        "vaddsd %3, %%xmm2, %%xmm2\n\t"
        "vmulsd %%xmm1, %%xmm2, %%xmm2\n\t"
        "vaddsd %4, %%xmm2, %%xmm2\n\t"
        "vmulsd %%xmm1, %%xmm2, %%xmm2\n\t"
        "vaddsd %5, %%xmm2, %%xmm2\n\t"
        "vmulsd %%xmm1, %%xmm2, %%xmm2\n\t"
        "vaddsd %6, %%xmm2, %%xmm2\n\t"
        "vmulsd %%xmm1, %%xmm2, %%xmm2\n\t"
        "vaddsd %7, %%xmm2, %%xmm2\n\t"
        "vmulsd %%xmm1, %%xmm2, %%xmm2\n\t"
        "vaddsd %8, %%xmm2, %%xmm2\n\t"
        "vmulsd %%xmm0, %%xmm2, %%xmm2\n\t"
        "vaddsd %%xmm2, %%xmm0, %%xmm0\n\t"
        "vmulsd %9, %%xmm0, %%xmm0\n\t"
        "cvtsi2sd %10, %%xmm1\n\t"
        "vmulsd %9, %%xmm1, %%xmm1\n\t"
        "vaddsd %%xmm1, %%xmm0, %%xmm0\n\t"
        "vmovsd %%xmm0, %0"
        : "=m"(result)
        : "m"(z), "m"(z2),
          "m"(CONST_1_OVER_3),
          "m"(CONST_1_OVER_5),
          "m"(CONST_1_OVER_7),
          "m"(CONST_1_OVER_9),
          "m"(CONST_1_OVER_11),
          "m"(CONST_1_OVER_13),
          "m"(ln2),
          "r"(exp)
        : "xmm0", "xmm1", "xmm2", "memory"
    );
    return result;
}

f64 DoubleTanh(f64 x) {
    f64 ax = DoubleAbs(x);
    if (ax > 19.0) return (x > 0.0) ? 1.0 : -1.0;
    
    f64 result;
    __asm__ volatile (
        "vmovsd %1, %%xmm0\n\t"
        "vmovsd %2, %%xmm1\n\t"
        "vmovsd %3, %%xmm2\n\t"
        "vmulsd %%xmm1, %%xmm2, %%xmm2\n\t"
        "vmovsd %5, %%xmm3\n\t"
        "vaddsd %%xmm2, %%xmm3, %%xmm3\n\t"
        "vmovsd %4, %%xmm2\n\t"
        "vmulsd %%xmm1, %%xmm2, %%xmm2\n\t"
        "vmulsd %%xmm1, %%xmm2, %%xmm2\n\t"
        "vmovsd %5, %%xmm4\n\t"
        "vaddsd %%xmm1, %%xmm4, %%xmm4\n\t"
        "vaddsd %%xmm2, %%xmm4, %%xmm4\n\t"
        "vdivsd %%xmm4, %%xmm3, %%xmm3\n\t"
        "vmulsd %%xmm0, %%xmm3, %%xmm3\n\t"
        "vmovsd %%xmm3, %0"
        : "=m"(result)
        : "m"(x), "m"(ax),
          "m"(CONST_0_5),
          "m"(CONST_1_OVER_3),
          "m"(DOUBLE_ONE)
        : "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "memory"
    );
    return result;
}

f64 DoubleAtan(f64 x) {
    f64 ax = DoubleAbs(x);
    int inverted = 0;
    if (ax > 1.0) {
        ax = 1.0 / ax;
        inverted = 1;
    }
    
    f64 ax2 = ax * ax;
    f64 result;
    
    __asm__ volatile (
        "vmovsd %1, %%xmm0\n\t"
        "vmovsd %2, %%xmm1\n\t"
        "vmulsd %%xmm1, %%xmm0, %%xmm2\n\t"
        "vmulsd %%xmm1, %%xmm2, %%xmm2\n\t"
        "vaddsd %3, %%xmm2, %%xmm2\n\t"
        "vmulsd %%xmm1, %%xmm2, %%xmm2\n\t"
        "vaddsd %4, %%xmm2, %%xmm2\n\t"
        "vmulsd %%xmm1, %%xmm2, %%xmm2\n\t"
        "vaddsd %5, %%xmm2, %%xmm2\n\t"
        "vmulsd %%xmm1, %%xmm2, %%xmm2\n\t"
        "vaddsd %6, %%xmm2, %%xmm2\n\t"
        "vmulsd %%xmm1, %%xmm2, %%xmm2\n\t"
        "vaddsd %7, %%xmm2, %%xmm2\n\t"
        "vmulsd %%xmm1, %%xmm2, %%xmm2\n\t"
        "vaddsd %8, %%xmm2, %%xmm2\n\t"
        "vmulsd %%xmm0, %%xmm2, %%xmm2\n\t"
        "vmovsd %%xmm2, %0"
        : "=m"(result)
        : "m"(ax), "m"(ax2),
          "m"(CONST_1_OVER_3),
          "m"(CONST_1_OVER_5),
          "m"(CONST_1_OVER_7),
          "m"(CONST_1_OVER_9),
          "m"(CONST_1_OVER_11),
          "m"(CONST_1_OVER_13)
        : "xmm0", "xmm1", "xmm2", "memory"
    );
    
    if (inverted) result = HALF_PI_F64 - result;
    if (x < 0.0) result = -result;
    return result;
}

f64 DoubleAtan2(f64 y, f64 x) {
    if (x == 0.0) {
        if (y > 0.0) return HALF_PI_F64;
        if (y < 0.0) return -HALF_PI_F64;
        return 0.0;
    }
    if (y == 0.0) {
        if (x > 0.0) return 0.0;
        return PI_F64;
    }
    
    f64 result = DoubleAtan(y / x);
    if (x < 0.0) {
        if (y >= 0.0) result += PI_F64;
        else result -= PI_F64;
    }
    return result;
}

f64 DoubleSqrt(f64 x) {
    if (x <= 0.0) return 0.0;
    f64 result;
    __asm__ volatile (
        "vsqrtsd %1, %%xmm0, %%xmm0\n\t"
        "vmovsd %%xmm0, %0"
        : "=m"(result)
        : "m"(x)
        : "xmm0", "memory"
    );
    return result;
}

f64 DoubleSinCos(f64 x, f64* cos_out) {
    f64 ax = DoubleMod2PI(x);
    f64 sign_sin = 1.0;
    f64 sign_cos = 1.0;
    
    if (ax < 0.0) {
        ax = -ax;
        sign_sin = -sign_sin;
    }
    if (ax > PI_F64) {
        ax = TWO_PI_F64 - ax;
        sign_sin = -sign_sin;
        sign_cos = -sign_cos;
    }
    if (ax > HALF_PI_F64) {
        ax = PI_F64 - ax;
        sign_cos = -sign_cos;
    }
    
    f64 ax2 = ax * ax;
    f64 sin_res, cos_res;
    
    __asm__ volatile (
        "vmovsd %2, %%xmm0\n\t"
        "vmovsd %3, %%xmm1\n\t"
        "vmulsd %%xmm1, %%xmm0, %%xmm2\n\t"
        "vmulsd %%xmm1, %%xmm2, %%xmm2\n\t"
        "vaddsd %4, %%xmm2, %%xmm2\n\t"
        "vmulsd %%xmm1, %%xmm2, %%xmm2\n\t"
        "vaddsd %5, %%xmm2, %%xmm2\n\t"
        "vmulsd %%xmm1, %%xmm2, %%xmm2\n\t"
        "vaddsd %6, %%xmm2, %%xmm2\n\t"
        "vmulsd %%xmm1, %%xmm2, %%xmm2\n\t"
        "vaddsd %7, %%xmm2, %%xmm2\n\t"
        "vmulsd %%xmm1, %%xmm2, %%xmm2\n\t"
        "vaddsd %8, %%xmm2, %%xmm2\n\t"
        "vmulsd %%xmm1, %%xmm2, %%xmm2\n\t"
        "vaddsd %9, %%xmm2, %%xmm2\n\t"
        "vmulsd %%xmm0, %%xmm2, %%xmm2\n\t"
        "vmovsd %%xmm2, %0\n\t"
        "vmulsd %%xmm1, %%xmm1, %%xmm2\n\t"
        "vmulsd %%xmm1, %%xmm2, %%xmm2\n\t"
        "vaddsd %10, %%xmm2, %%xmm2\n\t"
        "vmulsd %%xmm1, %%xmm2, %%xmm2\n\t"
        "vaddsd %11, %%xmm2, %%xmm2\n\t"
        "vmulsd %%xmm1, %%xmm2, %%xmm2\n\t"
        "vaddsd %12, %%xmm2, %%xmm2\n\t"
        "vmulsd %%xmm1, %%xmm2, %%xmm2\n\t"
        "vaddsd %13, %%xmm2, %%xmm2\n\t"
        "vmulsd %%xmm1, %%xmm2, %%xmm2\n\t"
        "vaddsd %14, %%xmm2, %%xmm2\n\t"
        "vmulsd %%xmm1, %%xmm2, %%xmm2\n\t"
        "vaddsd %15, %%xmm2, %%xmm2\n\t"
        "vaddsd %16, %%xmm2, %%xmm2\n\t"
        "vmovsd %%xmm2, %1"
        : "=m"(sin_res), "=m"(cos_res)
        : "m"(ax), "m"(ax2),
          "m"(CONST_1_OVER_3),
          "m"(CONST_1_OVER_5),
          "m"(CONST_1_OVER_7),
          "m"(CONST_1_OVER_9),
          "m"(CONST_1_OVER_11),
          "m"(CONST_1_OVER_13),
          "m"(CONST_1_OVER_87178291200),
          "m"(CONST_1_OVER_479001600),
          "m"(CONST_1_OVER_39916800),
          "m"(CONST_1_OVER_3628800),
          "m"(CONST_1_OVER_40320),
          "m"(CONST_1_OVER_720),
          "m"(CONST_1_OVER_24),
          "m"(CONST_0_5),
          "m"(DOUBLE_ONE)
        : "xmm0", "xmm1", "xmm2", "memory"
    );
    
    *cos_out = sign_cos * cos_res;
    return sign_sin * sin_res;
}

void DoubleSin4(const f64* x, f64* result) {
    f64 processed[4] = {0, 0, 0, 0};
    f64 signs[4] = {1, 1, 1, 1};
    #pragma clang loop unroll(enable)
    for (int i = 0; i < 4; i++) {
        f64 v = x[i];
        
        if (v != v) {
            result[i] = v;
            continue;
        }
        if (v == 0.0) {
            result[i] = 0.0;
            continue;
        }

        s64 n;
        __asm__ volatile (
            "fldl %1\n\t"
            "fldl %2\n\t"
            "fmulp\n\t"
            "fistpll %0\n\t"
            : "=m"(n)
            : "m"(v), "m"(INV_2PI_F64)
            : "st", "st(1)", "memory"
        );
        
        f64 nx = v - (f64)n * TWO_PI_F64;
        
        if (nx > PI_F64) {
            n++;
            nx = v - (f64)n * TWO_PI_F64;
        } else if (nx < -PI_F64) {
            n--;
            nx = v - (f64)n * TWO_PI_F64;
        }
        
        f64 sign = 1.0;
        if (nx < 0) {
            nx = -nx;
            sign = -1.0;
        }
        if (nx > PI_F64) {
            nx = TWO_PI_F64 - nx;
            sign = -sign;
        }
        if (nx > HALF_PI_F64) {
            nx = PI_F64 - nx;
        }
        
        processed[i] = nx;
        signs[i] = sign;
    }
    
    f64 raw[4];
    __asm__ volatile (
        "vmovupd %1, %%ymm0\n\t"
        "vmulpd %%ymm0, %%ymm0, %%ymm1\n\t"
        "vmulpd %%ymm0, %%ymm1, %%ymm2\n\t"
        
        "vbroadcastsd %2, %%ymm3\n\t"
        "vmulpd %%ymm3, %%ymm2, %%ymm3\n\t"
        "vsubpd %%ymm3, %%ymm0, %%ymm3\n\t"
        
        "vmulpd %%ymm1, %%ymm3, %%ymm4\n\t"
        "vbroadcastsd %3, %%ymm5\n\t"
        "vmulpd %%ymm5, %%ymm4, %%ymm4\n\t"
        "vaddpd %%ymm4, %%ymm3, %%ymm3\n\t"
        
        "vmulpd %%ymm1, %%ymm3, %%ymm4\n\t"
        "vbroadcastsd %4, %%ymm5\n\t"
        "vmulpd %%ymm5, %%ymm4, %%ymm4\n\t"
        "vsubpd %%ymm4, %%ymm3, %%ymm3\n\t"
        
        "vmulpd %%ymm1, %%ymm3, %%ymm4\n\t"
        "vbroadcastsd %5, %%ymm5\n\t"
        "vmulpd %%ymm5, %%ymm4, %%ymm4\n\t"
        "vaddpd %%ymm4, %%ymm3, %%ymm3\n\t"
        
        "vmulpd %%ymm1, %%ymm3, %%ymm4\n\t"
        "vbroadcastsd %6, %%ymm5\n\t"
        "vmulpd %%ymm5, %%ymm4, %%ymm4\n\t"
        "vsubpd %%ymm4, %%ymm3, %%ymm3\n\t"
        
        "vmulpd %%ymm1, %%ymm3, %%ymm4\n\t"
        "vbroadcastsd %7, %%ymm5\n\t"
        "vmulpd %%ymm5, %%ymm4, %%ymm4\n\t"
        "vaddpd %%ymm4, %%ymm3, %%ymm0\n\t"
        
        "vmovupd %%ymm0, %0"
        : "=m"(raw[0])
        : "m"(processed[0]), 
          "m"(SIN_C1), "m"(SIN_C2), "m"(SIN_C3),
          "m"(SIN_C4), "m"(SIN_C5), "m"(SIN_C6)
        : "ymm0", "ymm1", "ymm2", "ymm3", "ymm4", "ymm5", "memory"
    );
    #pragma clang loop unroll(enable)
    for (int i = 0; i < 4; i++) {
        result[i] = raw[i] * signs[i];
        if (result[i] == 0.0) result[i] = 0.0;
    }
}

void DoubleCos4(const f64* x, f64* result) {
    f64 inv2pi = INV_2PI_F64;
    f64 twopi = TWO_PI_F64;
    __asm__ volatile (
        "vmovupd %1, %%ymm0\n\t"
        "vbroadcastsd %2, %%ymm1\n\t"
        "vmulpd %%ymm1, %%ymm0, %%ymm0\n\t"
        "vroundpd $0x09, %%ymm0, %%ymm0\n\t"
        "vbroadcastsd %3, %%ymm1\n\t"
        "vmulpd %%ymm1, %%ymm0, %%ymm0\n\t"
        "vmovupd %1, %%ymm1\n\t"
        "vsubpd %%ymm0, %%ymm1, %%ymm0\n\t"
        "vmulpd %%ymm0, %%ymm0, %%ymm1\n\t"
        "vbroadcastsd %4, %%ymm2\n\t"
        "vmulpd %%ymm1, %%ymm2, %%ymm2\n\t"
        "vbroadcastsd %5, %%ymm3\n\t"
        "vaddpd %%ymm3, %%ymm2, %%ymm2\n\t"
        "vmulpd %%ymm1, %%ymm2, %%ymm2\n\t"
        "vbroadcastsd %6, %%ymm3\n\t"
        "vaddpd %%ymm3, %%ymm2, %%ymm2\n\t"
        "vmulpd %%ymm1, %%ymm2, %%ymm2\n\t"
        "vbroadcastsd %7, %%ymm3\n\t"
        "vaddpd %%ymm3, %%ymm2, %%ymm2\n\t"
        "vmulpd %%ymm1, %%ymm2, %%ymm2\n\t"
        "vbroadcastsd %8, %%ymm3\n\t"
        "vaddpd %%ymm3, %%ymm2, %%ymm2\n\t"
        "vmulpd %%ymm1, %%ymm2, %%ymm2\n\t"
        "vbroadcastsd %9, %%ymm3\n\t"
        "vaddpd %%ymm3, %%ymm2, %%ymm2\n\t"
        "vmulpd %%ymm1, %%ymm2, %%ymm2\n\t"
        "vbroadcastsd %10, %%ymm3\n\t"
        "vaddpd %%ymm3, %%ymm2, %%ymm2\n\t"
        "vmulpd %%ymm1, %%ymm2, %%ymm2\n\t"
        "vbroadcastsd %11, %%ymm3\n\t"
        "vaddpd %%ymm3, %%ymm2, %%ymm2\n\t"
        "vmulpd %%ymm1, %%ymm2, %%ymm2\n\t"
        "vbroadcastsd %12, %%ymm3\n\t"
        "vaddpd %%ymm3, %%ymm2, %%ymm0\n\t"
        "vmovupd %%ymm0, %0"
        : "=m"(result[0])
        : "m"(x[0]), "m"(inv2pi), "m"(twopi),
          "m"(CONST_1_OVER_87178291200),
          "m"(CONST_1_OVER_479001600),
          "m"(CONST_1_OVER_39916800),
          "m"(CONST_1_OVER_3628800),
          "m"(CONST_1_OVER_40320),
          "m"(CONST_1_OVER_720),
          "m"(CONST_1_OVER_24),
          "m"(CONST_0_5),
          "m"(DOUBLE_ONE)
        : "ymm0", "ymm1", "ymm2", "ymm3", "memory"
    );
}

f64 DoubleRound(f64 val, int round_to) {
    f64 factor = 1.0;
    for (int i = 0; i < round_to; i++) {
        factor *= 10.0;
    }
    val *= factor;
    f64 result;
    __asm__ volatile (
        "vroundsd $0x04, %1, %1, %0\n\t"
        : "=x"(result)
        : "x"(val)
    );
    return result / factor;
}

f32 FloatRound(f32 val, int round_to) {
    static const f32 pow10[] = {
        1.0f, 10.0f, 100.0f, 1000.0f, 10000.0f,
        100000.0f, 1000000.0f
    };
    if (round_to < 0 || round_to > 6) return val;
    f32 factor = pow10[round_to];
    f32 sign = (val < 0.0f) ? -1.0f : 1.0f;
    if (val < 0.0f) val = -val;
    val *= factor;
    f32 result;
    __asm__ volatile (
        "vroundss $0x04, %1, %1, %0\n\t"
        : "=x"(result)
        : "x"(val)
    );
    return (result / factor) * sign;
}

f64 DoubleTan(f64 x) {
    f64 cos_x;
    f64 sin_x = DoubleSinCos(x, &cos_x);
    return sin_x / cos_x;
}

f32 FloatTan(f64 x) {
    f64 cos_x;
    f64 sin_x = DoubleSinCos(x, &cos_x);
    return (f32)(sin_x / cos_x);
}