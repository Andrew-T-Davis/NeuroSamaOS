#include "str.h"
#include "int.h"
#include "kmalloc.h"

extern AllocPool KernelPool;

void PingPong(void **Ping, void **Pong){
    if (!Ping || !Pong) return;
    void* Temp=*Pong;
    *Pong=*Ping;
    *Ping=Temp;
}

void StrAdd(u16* Add,const char* S,Size64 Size){
    u64 start=0;
    while (Add[start]!='\0') start++;

    MemCopy(Add+start,S,Size);
    Add[start + Size] = '\0';
}

void MemCopySize32CountByte(u32* D, const u32* S, Size64 size_bytes) {
    if (size_bytes == 0) return;
    if (size_bytes >= 2000000 && ((u64)D & 0x1F) == 0 && (size_bytes & 0x1F) == 0 && ((u64)S & 0x1F) == 0) {
        Size64 loops = size_bytes / 32;

        __asm__ volatile(
            "1:\n\t"
            "vmovntdqa (%1), %%ymm0\n\t"
            "vmovntdq %%ymm0, (%0)\n\t"
            "addq $32, %0\n\t"
            "addq $32, %1\n\t"
            "decq %2\n\t"
            "jnz 1b\n\t"
            "vzeroupper\n\t"
            "sfence"
            : "+r"(D), "+r"(S), "+r"(loops)
            :: "ymm0", "memory"
        );
    } 
    else {
        Size64 dwords = size_bytes / 4; 
        __asm__ volatile(
            "cld\n\t"
            "rep movsl"
            : "+D"(D), "+S"(S), "+c"(dwords)
            :: "memory"
        );
    }
}

void MemCopy(void* D, const void* S, Size64 Size) {
    if (Size == 0) return;
    if (Size >= 2000000 && ((u64)D & 0x1F) == 0 && ((u64)S & 0x1F) == 0) {
        Size64 loops = Size >> 5;
        Size64 remainder = Size & 0x1F;

        __asm__ volatile(
            "1:\n\t"
            "vmovntdqa (%1), %%ymm0\n\t"
            "vmovntdq %%ymm0, (%0)\n\t"
            "addq $32, %0\n\t"
            "addq $32, %1\n\t"
            "decq %2\n\t"
            "jnz 1b\n\t"
            "vzeroupper\n\t"
            "sfence"
            : "+r"(D), "+r"(S), "+r"(loops)
            :: "ymm0", "memory"
        );

        if (remainder > 0) {
            __asm__ volatile(
                "cld\n\t"
                "rep movsb"
                : "+D"(D), "+S"(S), "+c"(remainder)
                :: "memory"
            );
        }
    } else {
        __asm__ volatile(
            "cld\n\t"
            "rep movsb"
            : "+D"(D), "+S"(S), "+c"(Size)
            :: "memory"
        );
    }
}


void MemSet(void* D, const u8 S, Size64 Size) {
    if (Size==0) return;
    if (Size >= 1000000 && ((u64)D & 0x0F) == 0 && (Size & 31) == 0) {
        Size64 loops = Size / 32;

        __asm__ volatile(
            "movd %2, %%xmm0\n\t"
            "vpbroadcastb %%xmm0, %%ymm0\n\t"

            "1:\n\t"
            "vmovntdq %%ymm0, (%0)\n\t"
            "addq $32, %0\n\t"
            "decq %1\n\t"
            "jnz 1b\n\t"
            "vzeroupper\n\t"
            "sfence"
            : "+r"(D), "+r"(loops)
            : "r"((u64)S)
            : "ymm0", "memory"
        );
    } else {
        __asm__ volatile("cld\n\t rep stosb"
            : "+D"(D), "+c"(Size)
            : "a"(S)
            : "memory");
    }
}

void MemSet16(void* D, const u16 S, Size64 Size) {
    if (Size==0) return;
    if (Size >= 1000000 && ((u64)D & 0x0F) == 0 && (Size & 31) == 0) {
        Size64 loops = Size / 16; 

        __asm__ volatile(
            "movd %2, %%xmm0\n\t"
            "vpbroadcastw %%xmm0, %%ymm0\n\t"

            "1:\n\t"
            "vmovntdq %%ymm0, (%0)\n\t"
            "addq $32, %0\n\t"
            "decq %1\n\t"
            "jnz 1b\n\t"
            "vzeroupper\n\t"
            "sfence"
            : "+r"(D), "+r"(loops)
            : "r"((u64)S)
            : "ymm0", "memory"
        );
    } else {
        __asm__ volatile(
            "cld\n\t rep stosw"
            : "+D"(D), "+c"(Size)
            : "a"(S)
            : "memory"
        );
    }
}

void MemSet32(void* D, const u32 S, Size64 Size) {
    if (Size==0) return;
    if (Size >= 1000000 && ((u64)D & 0x0F) == 0 && (Size & 31) == 0) {
        Size64 loops = Size / 8; 

        __asm__ volatile(
            "movd %2, %%xmm0\n\t"
            "vpbroadcastd %%xmm0, %%ymm0\n\t"

            "1:\n\t"
            "vmovntdq %%ymm0, (%0)\n\t"
            "addq $32, %0\n\t"
            "decq %1\n\t"
            "jnz 1b\n\t"
            "vzeroupper\n\t"
            "sfence"
            : "+r"(D), "+r"(loops)
            : "r"((u64)S)
            : "ymm0", "memory"
        );
    } else {
        __asm__ volatile(
            "cld\n\t rep stosl"
            : "+D"(D), "+c"(Size)
            : "a"(S)
            : "memory"
        );
    }
}

void MemSet64(void* D, const u64 S, Size64 Size) {
    if (Size==0) return;
    if (Size >= 1000000 && ((u64)D & 0x0F) == 0 && (Size & 31) == 0) {
        Size64 loops = Size / 4; 

        __asm__ volatile(
            "movd %2, %%xmm0\n\t"
            "vpbroadcastq %%xmm0, %%ymm0\n\t"

            "1:\n\t"
            "vmovntdq %%ymm0, (%0)\n\t"
            "addq $32, %0\n\t"
            "decq %1\n\t"
            "jnz 1b\n\t"
            "vzeroupper\n\t"
            "sfence"
            : "+r"(D), "+r"(loops)
            : "r"((u64)S)
            : "ymm0", "memory"
        );
    } else {
        __asm__ volatile(
            "cld\n\t rep stosq"
            : "+D"(D), "+c"(Size)
            : "a"(S)
            : "memory"
        );
    }
}

void Bit64Str(u64 N,u16* Buf){
    #pragma clang loop unroll(enable)
    for (s64 idx=63;idx>=0;idx--){
        Buf[63 - idx] = ((N >> idx) & 1) + 48;
    }
    Buf[64]='\0';
}

u64 HowManyCharInString(char* String,char Char){
    u64 Many=0;
    while (*String) {
        if (*String == Char) Many++;
        String++;
    }
    return Many;
}

u64 HowManyCharInStringWithSize(char* String,char Char,Size64 Size){
    u64 Many=0;
    for (Size64 Idx=0;Idx<Size;Idx++){
        if(String[Idx]==Char) Many++;
    }
    return Many;
}

Bool StrIs(const char* D, const char* S) {
    if (!D || !S) return D == S;
    while (*D && *S) {
        if (*D != *S) return 0;
        D++;
        S++;
    }
    return 1;
}

Bool StrIsNoCase(const char* D, const char* S) {
    if (!D || !S) return D == S;
    while (*D && *S) {
        if ((*D | 0x20) != (*S | 0x20)) return 0;
        D++; S++;
    }
    return 1;
}

Bool StrIsWithSize(const char* D, const char* S,Size64 Size) {
    if (!D || !S) return D == S;
    for (u64 Idx=0;Idx<Size;Idx++){
        if (*D != *S) return 0;
        D++;
        S++;
    }
    return 1;
}

Bool StrIsNoCaseWithSize(const char* D, const char* S, Size64 Size) {
    if (!D || !S) return D == S;
    for (u64 Idx=0;Idx<Size;Idx++){
        if ((*D | 0x20) != (*S | 0x20)) return 0;
        D++; S++;
    }
    return 1;
}

u64 StrLen(const char* S) {
    if (!S) return 0;
    u64 len = 0;
    while (*S) {
        len++;
        S++;
    }
    return len;
}

int StrSplit(char* Str, char Split, char** left, char** right) {
    if (!Str || !left || !right) return -1;
    
    char* pos = Str;
    while (*pos && *pos != Split) pos++;
    
    if (*pos == Split) {
        *pos = '\0';
        *left = Str;
        *right = pos + 1;
        return 0;
    }
    
    *left = Str;
    *right = NULL;
    return 0;
}

Bool MemCmp(const void* S1, const void* S2, u64 N) {
    if (N == 0) return true;
    
    const u8* P1 = (const u8*)S1;
    const u8* P2 = (const u8*)S2;
    
    if (N >= 256 && ((u64)P1 & 0x1F) == 0 && ((u64)P2 & 0x1F) == 0) {
        u64 loops = N >> 5;
        u64 remainder = N & 0x1F;
        u64 result = 0;
        
        __asm__ volatile(
            "1:\n\t"
            "vmovdqa (%1), %%ymm0\n\t"
            "vpcmpeqq (%2), %%ymm0, %%ymm0\n\t"
            "vpmovmskb %%ymm0, %%eax\n\t"
            "cmp $0xFFFFFFFF, %%eax\n\t"
            "jne 2f\n\t"
            "addq $32, %1\n\t"
            "addq $32, %2\n\t"
            "decq %3\n\t"
            "jnz 1b\n\t"
            "2:\n\t"
            "sete %b0\n\t"
            "vzeroupper\n\t"
            : "=r"(result)
            : "r"(P1), "r"(P2), "r"(loops)
            : "eax", "ymm0", "memory"
        );
        
        if (!result) return false;
        
        P1 += loops << 5;
        P2 += loops << 5;
        N = remainder;
    }
    
    while (((u64)P1 & 3) && N > 0) {
        if (*P1++ != *P2++) return false;
        N--;
    }
    
    while (N >= 4) {
        if (*(u32*)P1 != *(u32*)P2) return false;
        P1 += 4; P2 += 4; N -= 4;
    }
    
    while (N > 0) {
        if (*P1++ != *P2++) return false;
        N--;
    }
    
    return true;
}

void PingPongNotBuffer(void* a, void* b, u64 size) {
    u8 *tmp=Alloc(&KernelPool, size);
    MemCopy(tmp, a, size);
    MemCopy(a, b, size);
    MemCopy(b, tmp, size);
}

u8 StrToU8(char* S, Bool DecOrHex) {
    if (!S || !*S) return 0;
    
    if (S[0] == '0' && (S[1] == 'x' || S[1] == 'X')) {
        S += 2;
        DecOrHex = true;
    }
    
    Bool neg = (*S == '-');
    if (neg) S++;
    
    u32 val = 0;
    u32 base = DecOrHex ? 16 : 10;
    u32 limit = neg ? 128 : 255;
    
    while (*S) {
        u8 d;
        if (*S >= '0' && *S <= '9') d = (u8)(*S - '0');
        else if (DecOrHex && *S >= 'A' && *S <= 'F') d = (u8)(*S - 'A' + 10);
        else if (DecOrHex && *S >= 'a' && *S <= 'f') d = (u8)(*S - 'a' + 10);
        else break;
        
        if (val > (limit / base)) return neg ? 0x80 : 0xFF;
        val = val * base + d;
        S++;
    }
    
    return (val > limit) ? (neg ? 0x80 : 0xFF) : (neg ? (u8)(val | 0x80) : (u8)val);
}

u16 StrToU16(char* S, Bool DecOrHex) {
    if (!S || !*S) return 0;
    
    if (S[0] == '0' && (S[1] == 'x' || S[1] == 'X')) {
        S += 2;
        DecOrHex = true;
    }
    
    Bool neg = (*S == '-');
    if (neg) S++;
    if (!*S) return 0;
    
    u32 val = 0;
    u32 base = DecOrHex ? 16 : 10;
    u32 limit = neg ? 32768 : 65535;
    
    while (*S) {
        u8 d;
        if (*S >= '0' && *S <= '9') d = (u8)(*S - '0');
        else if (DecOrHex && *S >= 'A' && *S <= 'F') d = (u8)(*S - 'A' + 10);
        else if (DecOrHex && *S >= 'a' && *S <= 'f') d = (u8)(*S - 'a' + 10);
        else break;
        
        if (val > (limit / base)) return neg ? 0x8000 : 0xFFFF;
        val = val * base + d;
        S++;
    }
    
    if (val > limit) return neg ? 0x8000 : 0xFFFF;
    return neg ? (u16)(val | 0x8000) : (u16)val;
}

u32 StrToU32(char* S, Bool DecOrHex) {
    if (!S || !*S) return 0;
    
    if (S[0] == '0' && (S[1] == 'x' || S[1] == 'X')) {
        S += 2;
        DecOrHex = true;
    }
    
    Bool neg = (*S == '-');
    if (neg) S++;
    if (!*S) return 0;
    
    u64 val = 0;
    u64 base = DecOrHex ? 16 : 10;
    u64 limit = neg ? 0x80000000ULL : 0xFFFFFFFFULL;
    
    while (*S) {
        u8 d;
        if (*S >= '0' && *S <= '9') d = (u8)(*S - '0');
        else if (DecOrHex && *S >= 'A' && *S <= 'F') d = (u8)(*S - 'A' + 10);
        else if (DecOrHex && *S >= 'a' && *S <= 'f') d = (u8)(*S - 'a' + 10);
        else break;
        
        if (val > (limit / base)) return neg ? 0x80000000UL : 0xFFFFFFFFUL;
        val = val * base + d;
        S++;
    }
    
    if (val > limit) return neg ? 0x80000000UL : 0xFFFFFFFFUL;
    return neg ? (u32)(val | 0x80000000ULL) : (u32)val;
}

u64 StrToU64(char* S, Bool DecOrHex) {
    if (!S || !*S) return 0;
    
    if (S[0] == '0' && (S[1] == 'x' || S[1] == 'X')) {
        S += 2;
        DecOrHex = true;
    }
    
    Bool neg = (*S == '-');
    if (neg) S++;
    if (!*S) return 0;
    
    u64 val = 0;
    u64 base = DecOrHex ? 16 : 10;
    u64 limit = neg ? 0x8000000000000000ULL : 0xFFFFFFFFFFFFFFFFULL;
    
    while (*S) {
        u8 d;
        if (*S >= '0' && *S <= '9') d = (u8)(*S - '0');
        else if (DecOrHex && *S >= 'A' && *S <= 'F') d = (u8)(*S - 'A' + 10);
        else if (DecOrHex && *S >= 'a' && *S <= 'f') d = (u8)(*S - 'a' + 10);
        else break;
        
        if (val > (limit - d) / base) return neg ? 0x8000000000000000ULL : 0xFFFFFFFFFFFFFFFFULL;
        val = val * base + d;
        S++;
    }
    
    if (val > limit) return neg ? 0x8000000000000000ULL : 0xFFFFFFFFFFFFFFFFULL;
    return neg ? (val | 0x8000000000000000ULL) : val;
}

static f64 ParseFraction(const u16** S) {
    f64 frac = 0.0;
    f64 divisor = 10.0;
    
    while (**S >= '0' && **S <= '9') {
        frac += (f64)(**S - '0') / divisor;
        divisor *= 10.0;
        (*S)++;
    }
    return frac;
}

static f64 ParseExponent(const u16** S) {
    if (**S != 'e' && **S != 'E') return 1.0;
    (*S)++;
    
    Bool expNeg = false;
    if (**S == '-') { expNeg = true; (*S)++; }
    else if (**S == '+') { (*S)++; }
    
    s64 expVal = 0;
    while (**S >= '0' && **S <= '9') {
        expVal = expVal * 10 + (**S - '0');
        (*S)++;
    }
    
    if (expNeg) expVal = -expVal;
    
    f64 result = 1.0;
    if (expVal >= 0) {
        for (s64 i = 0; i < expVal; i++) result *= 10.0;
    } else {
        for (s64 i = 0; i < -expVal; i++) result /= 10.0;
    }
    return result;
}

f32 StrToF32(char* S) {
    if (!S || !*S) return 0.0f;
    if ((u64)S < 0x1000) {
        return 0.0f;
    }
    
    while (*S == ' ' || *S == '\t') S++;
    if (!*S) return 0.0f;
    
    Bool neg = false;
    if (*S == '-') { neg = true; S++; }
    else if (*S == '+') { S++; }
    if (!*S) return 0.0f;
    
    f64 result = 0.0;
    while (*S >= '0' && *S <= '9') {
        result = result * 10.0 + (*S - '0');
        S++;
    }
    
    if (*S == '.') {
        S++;
        result += ParseFraction((const u16**)&S);
    }
    
    if (*S == 'e' || *S == 'E') {
        result *= ParseExponent((const u16**)&S);
    }
    
    if (neg) result = -result;
    
    if (result > 3.40282347e38f) return 3.40282347e38f;
    if (result < -3.40282347e38f) return -3.40282347e38f;
    return (f32)result;
}

f64 StrToF64(char* S) {
    if (!S || !*S) return 0.0;
    
    while (*S == ' ' || *S == '\t') S++;
    if (!*S) return 0.0;
    
    Bool neg = false;
    if (*S == '-') { neg = true; S++; }
    else if (*S == '+') { S++; }
    if (!*S) return 0.0;
    
    f64 result = 0.0;
    while (*S >= '0' && *S <= '9') {
        result = result * 10.0 + (*S - '0');
        S++;
    }
    
    if (*S == '.') {
        S++;
        result += ParseFraction((const u16**)&S);
    }
    
    if (*S == 'e' || *S == 'E') {
        result *= ParseExponent((const u16**)&S);
    }
    
    if (neg) result = -result;
    
    if (result > 1.7976931348623157e308) return 1.7976931348623157e308;
    if (result < -1.7976931348623157e308) return -1.7976931348623157e308;
    return result;
}

f80 StrToF80(char* S) {
    if (!S || !*S) return 0.0L;
    
    while (*S == ' ' || *S == '\t') S++;
    if (!*S) return 0.0L;
    
    Bool neg = false;
    if (*S == '-') { neg = true; S++; }
    else if (*S == '+') { S++; }
    if (!*S) return 0.0L;
    
    f80 result = 0.0L;
    while (*S >= '0' && *S <= '9') {
        result = result * 10.0L + (*S - '0');
        S++;
    }
    
    if (*S == '.') {
        S++;
        f80 divisor = 10.0L;
        while (*S >= '0' && *S <= '9') {
            result += (f80)(*S - '0') / divisor;
            divisor *= 10.0L;
            S++;
        }
    }
    
    if (*S == 'e' || *S == 'E') {
        S++;
        Bool expNeg = false;
        if (*S == '-') { expNeg = true; S++; }
        else if (*S == '+') { S++; }
        
        s64 expVal = 0;
        while (*S >= '0' && *S <= '9') {
            expVal = expVal * 10 + (*S - '0');
            S++;
        }
        
        if (expNeg) expVal = -expVal;
        
        f80 expResult = 1.0L;
        if (expVal >= 0) {
            for (s64 i = 0; i < expVal; i++) expResult *= 10.0L;
        } else {
            for (s64 i = 0; i < -expVal; i++) expResult /= 10.0L;
        }
        result *= expResult;
    }
    
    if (neg) result = -result;
    return result;
}

void Reverse(u16* start, u16* end) {
    while (start < end) {
        u16 tmp = *start;
        *start = *end;
        *end = tmp;
        start++;
        end--;
    }
}

static char DigitToChar(u8 d, Bool upper) {
    if (d < 10) return (char)('0' + d);
    return upper ? (char)('A' + d - 10) : (char)('a' + d - 10);
}

Size64 U8ToStr(u8 val, char* buf, Bool hex) {
    if (!buf) return 0;
    
    union { u8 v; struct { u8 low:7; u8 sign:1; }; } n = { .v = val };
    
    if (hex) {
        char* p = buf;
        if (n.sign) *p++ = '-';
        *p++ = '0';
        *p++ = 'x';
        
        u8 high = n.low >> 4;
        u8 low  = n.low & 0x0F;
        
        if (high) *p++ = DigitToChar(high, true);
        *p++ = DigitToChar(low, true);
        *p = '\0';
        return (Size64)(p - buf);
    } else {
        u8 abs_val = n.low;
        if (abs_val == 0) {
            buf[0] = '0';
            buf[1] = '\0';
            return 1;
        }
        
        char* p = buf;
        if (n.sign) *p++ = '-';
        
        char temp[4];
        int idx = 0;
        while (abs_val > 0) {
            temp[idx++] = (char)('0' + (abs_val % 10));
            abs_val /= 10;
        }
        for (int i = idx - 1; i >= 0; i--) *p++ = temp[i];
        *p = '\0';
        return (Size64)(p - buf);
    }
}

Size64 U16ToStr(u16 val, char* buf, Bool hex) {
    if (!buf) return 0;
    
    union { u16 v; struct { u16 low:15; u16 sign:1; }; } n = { .v = val };
    
    if (hex) {
        char* p = buf;
        if (n.sign) *p++ = '-';
        *p++ = '0';
        *p++ = 'x';
        
        Bool started = false;
        for (int i = 12; i >= 0; i -= 4) {
            u8 d = (u8)((n.low >> i) & 0x0F);
            if (!started && d == 0) continue;
            started = true;
            *p++ = DigitToChar(d, true);
        }
        if (!started) *p++ = '0';
        *p = '\0';
        return (Size64)(p - buf);
    } else {
        u16 abs_val = n.low;
        if (abs_val == 0) {
            buf[0] = '0';
            buf[1] = '\0';
            return 1;
        }
        
        char* p = buf;
        if (n.sign) *p++ = '-';
        
        char temp[6];
        int idx = 0;
        while (abs_val > 0) {
            temp[idx++] = (char)('0' + (abs_val % 10));
            abs_val /= 10;
        }
        for (int i = idx - 1; i >= 0; i--) *p++ = temp[i];
        *p = '\0';
        return (Size64)(p - buf);
    }
}

Size64 U32ToStr(u32 val, char* buf, Bool hex) {
    if (!buf) return 0;
    
    union { u32 v; struct { u32 low:31; u32 sign:1; }; } n = { .v = val };
    
    if (hex) {
        char* p = buf;
        if (n.sign) *p++ = '-';
        *p++ = '0';
        *p++ = 'x';
        
        Bool started = false;
        for (int i = 28; i >= 0; i -= 4) {
            u8 d = (u8)((n.low >> i) & 0x0F);
            if (!started && d == 0) continue;
            started = true;
            *p++ = DigitToChar(d, true);
        }
        if (!started) *p++ = '0';
        *p = '\0';
        return (Size64)(p - buf);
    } else {
        u32 abs_val = n.low;
        if (abs_val == 0) {
            buf[0] = '0';
            buf[1] = '\0';
            return 1;
        }
        
        char* p = buf;
        if (n.sign) *p++ = '-';
        
        char temp[12];
        int idx = 0;
        while (abs_val > 0) {
            temp[idx++] = (char)('0' + (abs_val % 10));
            abs_val /= 10;
        }
        for (int i = idx - 1; i >= 0; i--) *p++ = temp[i];
        *p = '\0';
        return (Size64)(p - buf);
    }
}
    
Size64 U64ToStr(u64 val, char* buf, Bool hex) {
    if (!buf) return 0;
    
    union { u64 v; struct { u64 low:63; u64 sign:1; }; } n = { .v = val };
    
    if (hex) {
        char* p = buf;
        if (n.sign) *p++ = '-';
        *p++ = '0';
        *p++ = 'x';
        
        Bool started = false;
        for (int i = 60; i >= 0; i -= 4) {
            u8 d = (u8)((n.low >> i) & 0x0F);
            if (!started && d == 0) continue;
            started = true;
            *p++ = DigitToChar(d, true);
        }
        if (!started) *p++ = '0';
        *p = '\0';
        return (Size64)(p - buf);
    } else {
        u64 abs_val = n.low;
        if (abs_val == 0) {
            buf[0] = '0';
            buf[1] = '\0';
            return 1;
        }
        
        char* p = buf;
        if (n.sign) *p++ = '-';
        
        char temp[22];
        int idx = 0;
        while (abs_val > 0) {
            temp[idx++] = (char)('0' + (abs_val % 10));
            abs_val /= 10;
        }
        for (int i = idx - 1; i >= 0; i--) *p++ = temp[i];
        *p = '\0';
        return (Size64)(p - buf);
    }
}

static Size64 FloatToStr(f64 val, char* buf, Bool force_scientific) {
    if (!buf) return 0;
    
    union { f64 f; u64 u; } n = { .f = val };
    
    u64 exp = (n.u >> 52) & 0x7FF;
    u64 man = n.u & 0x000FFFFFFFFFFFFFULL;
    
    if (exp == 0x7FF) {
        if (man == 0) {
            if (n.u & 0x8000000000000000ULL) {
                MemCopy(buf, "-inf", 4);
                buf[4] = '\0';
                return 4;
            }
            MemCopy(buf, "inf", 3);
            buf[3] = '\0';
            return 3;
        }
        MemCopy(buf, "nan", 3);
        buf[3] = '\0';
        return 3;
    }
    
    Bool neg = (n.u >> 63) & 1;
    if (neg) val = -val;
    
    if (val == 0.0) {
        if (neg) {
            neg = 0;
        }
    }
    
    if (val > 1e15 || (val < 1e-6 && val != 0.0)) {
        force_scientific = true;
    }
    
    if (force_scientific) {
        int exponent = 0;
        if (val != 0.0) {
            while (val >= 10.0) { val /= 10.0; exponent++; }
            while (val < 1.0) { val *= 10.0; exponent--; }
        }
        
        char* p = buf;
        if (neg) *p++ = '-';
        
        u64 int_part = (u64)val;
        val -= (f64)int_part;
        *p++ = (char)('0' + int_part);
        
        if (val != 0.0) {
            *p++ = '.';
            for (int i = 0; i < 8; i++) {
                val *= 10.0;
                int digit = (int)val;
                *p++ = (char)('0' + digit);
                val -= digit;
                if (val == 0.0) break;
            }
            if (p > buf && *(p - 1) == '.') {
                *p++ = '0';
            } else {
                p--;
                while (p > buf && *p == '0') p--;
                p++;
                if (p > buf && *(p - 1) == '.') {
                    char* q = p;
                    while (q > buf && *(q - 1) != '.') q--;
                    if (q > buf && *(q - 1) == '.') {
                        *p++ = '0';
                    }
                }
            }
        }
        
        *p++ = 'e';
        if (exponent < 0) {
            *p++ = '-';
            exponent = -exponent;
        }
        if (exponent >= 100) {
            *p++ = (char)('0' + exponent / 100);
            exponent %= 100;
        }
        if (exponent >= 10) {
            *p++ = (char)('0' + exponent / 10);
            exponent %= 10;
        }
        *p++ = (char)('0' + exponent);
        *p = '\0';
        
        return (Size64)(p - buf);
    } else {
        u64 int_part = (u64)val;
        f64 frac_part = val - (f64)int_part;
        
        char* p = buf;
        if (neg) *p++ = '-';
        
        if (int_part == 0) {
            *p++ = '0';
        } else {
            char temp[32];
            int idx = 0;
            while (int_part > 0) {
                temp[idx++] = (char)('0' + (int_part % 10));
                int_part /= 10;
            }
            for (int i = idx - 1; i >= 0; i--) {
                *p++ = temp[i];
            }
        }
        
        if (frac_part != 0.0) {
            *p++ = '.';
            int count = 0;
            while (frac_part > 0.0 && count < 10) {
                frac_part *= 10.0;
                int digit = (int)frac_part;
                *p++ = (char)('0' + digit);
                frac_part -= digit;
                count++;
            }
            if (p > buf && *(p - 1) == '.') {
                *p++ = '0';
            } else {
                p--;
                while (p > buf && *p == '0') p--;
                p++;
                if (p > buf && *(p - 1) == '.') {
                    char* q = p;
                    while (q > buf && *(q - 1) != '.') q--;
                    if (q > buf && *(q - 1) == '.') {
                        *p++ = '0';
                    }
                }
            }
        }
        
        *p = '\0';
        return (Size64)(p - buf);
    }
}

Size64 F32ToStr(f32 val, char* buf) {
    if (!buf) return 0;
    
    union { f32 f; u32 u; } n = { .f = val };
    u32 exp = (n.u >> 23) & 0xFF;
    u32 man = n.u & 0x007FFFFF;
    
    if (exp == 0xFF) {
        if (man == 0) {
            if (n.u & 0x80000000) {
                MemCopy(buf, "-inf", 4);
                buf[4] = '\0';
                return 4;
            }
            MemCopy(buf, "inf", 3);
            buf[3] = '\0';
            return 3;
        }
        MemCopy(buf, "nan", 3);
        buf[3] = '\0';
        return 3;
    }
    
    return FloatToStr((f64)val, buf, 0);
}

Size64 F64ToStr(f64 val, char* buf) {
    if (!buf) return 0;
    return FloatToStr(val, buf, 0);
}

Size64 F80ToStr(f80 val, char* buf) {
    if (!buf) return 0;
    return FloatToStr((f64)val, buf, 0);
}