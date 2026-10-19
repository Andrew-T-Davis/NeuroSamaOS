#include "SHA256.h"
#include "int.h"
#include "str.h"

#define ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))

#define Ch(x, y, z)  (((x) & (y)) ^ (~(x) & (z)))
#define Maj(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define Sigma0(x)    (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#define Sigma1(x)    (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))
#define sigma0(x)    (ROTR(x, 7) ^ ROTR(x, 18) ^ ((x) >> 3))
#define sigma1(x)    (ROTR(x, 17) ^ ROTR(x, 19) ^ ((x) >> 10))

static const u32 K[64] = {
    [0] = 0x428A2F98,[1] = 0x71374491,[2] = 0xB5C0FBCF,[3] = 0xE9B5DBA5,[4] = 0x3956C25B,
    [5] = 0x59F111F1,[6] = 0x923F82A4,[7] = 0xAB1C5ED5,[8] = 0xD807AA98,[9] = 0x12835B01,
    [10]= 0x243185BE,[11]= 0x550C7DC3,[12]= 0x72BE5D74,[13]= 0x80DEB1FE,[14]= 0x9BDC06A7,
    [15]= 0xC19BF174,[16]= 0xE49B69C1,[17]= 0xEFBE4786,[18]= 0x0FC19DC6,[19]= 0x240CA1CC,
    [20]= 0x2DE92C6F,[21]= 0x4A7484AA,[22]= 0x5CB0A9DC,[23]= 0x76F988DA,[24]= 0x983E5152,
    [25]= 0xA831C66D,[26]= 0xB00327C8,[27]= 0xBF597FC7,[28]= 0xC6E00BF3,[29]= 0xD5A79147,
    [30]= 0x06CA6351,[31]= 0x14292967,[32]= 0x27B70A85,[33]= 0x2E1B2138,[34]= 0x4D2C6DFC,
    [35]= 0x53380D13,[36]= 0x650A7354,[37]= 0x766A0ABB,[38]= 0x81C2C92E,[39]= 0x92722C85,
    [40]= 0xA2BFE8A1,[41]= 0xA81A664B,[42]= 0xC24B8B70,[43]= 0xC76C51A3,[44]= 0xD192E819,
    [45]= 0xD6990624,[46]= 0xF40E3585,[47]= 0x106AA070,[48]= 0x19A4C116,[49]= 0x1E376C08,
    [50]= 0x2748774C,[51]= 0x34B0BCB5,[52]= 0x391C0CB3,[53]= 0x4ED8AA4A,[54]= 0x5B9CCA4F,
    [55]= 0x682E6FF3,[56]= 0x748F82EE,[57]= 0x78A5636F,[58]= 0x84C87814,[59]= 0x8CC70208,
    [60]= 0x90BEFFFA,[61]= 0xA4506CEB,[62]= 0xBEF9A3F7,[63]= 0xC67178F2
};

void SHA256(void* Buffer, void* Msg, u64 MsgSize) {
    if (MsgSize >= 56) return;

    u8 TmpBuffer[64];
    MemSet(TmpBuffer, 0, 64);
    MemCopy(TmpBuffer, Msg, MsgSize);
    
    TmpBuffer[MsgSize] = 0x80;

    u64 MsgSizeBit = MsgSize * 8;
    TmpBuffer[56] = (MsgSizeBit >> 56) & 0xFF;
    TmpBuffer[57] = (MsgSizeBit >> 48) & 0xFF;
    TmpBuffer[58] = (MsgSizeBit >> 40) & 0xFF;
    TmpBuffer[59] = (MsgSizeBit >> 32) & 0xFF;
    TmpBuffer[60] = (MsgSizeBit >> 24) & 0xFF;
    TmpBuffer[61] = (MsgSizeBit >> 16) & 0xFF;
    TmpBuffer[62] = (MsgSizeBit >>  8) & 0xFF;
    TmpBuffer[63] = (MsgSizeBit >>  0) & 0xFF;

    u32 H[8] = {
        0x6A09E667, 0xBB67AE85, 0x3C6EF372, 0xA54FF53A,
        0x510E527F, 0x9B05688C, 0x1F83D9AB, 0x5BE0CD19
    };

    u32 W[64];
    for (int i = 0; i < 16; i++) {
        W[i] = ((u32)TmpBuffer[i * 4 + 0] << 24) |
               ((u32)TmpBuffer[i * 4 + 1] << 16) |
               ((u32)TmpBuffer[i * 4 + 2] << 8)  |
               ((u32)TmpBuffer[i * 4 + 3] << 0);
    }
    for (int i = 16; i < 64; i++) {
        W[i] = sigma1(W[i - 2]) + W[i - 7] + sigma0(W[i - 15]) + W[i - 16];
    }

    u32 a = H[0];
    u32 b = H[1];
    u32 c = H[2];
    u32 d = H[3];
    u32 e = H[4];
    u32 f = H[5];
    u32 g = H[6];
    u32 h = H[7];

    for (int i = 0; i < 64; i++) {
        u32 T1 = h + Sigma1(e) + Ch(e, f, g) + K[i] + W[i];
        u32 T2 = Sigma0(a) + Maj(a, b, c);
        
        h = g;
        g = f;
        f = e;
        e = d + T1;
        d = c;
        c = b;
        b = a;
        a = T1 + T2;
    }

    H[0] += a; H[1] += b; H[2] += c; H[3] += d;
    H[4] += e; H[5] += f; H[6] += g; H[7] += h;

    u8* OutBuffer = (u8*)Buffer;
    for (int i = 0; i < 8; i++) {
        OutBuffer[i * 4 + 0] = (H[i] >> 24) & 0xFF;
        OutBuffer[i * 4 + 1] = (H[i] >> 16) & 0xFF;
        OutBuffer[i * 4 + 2] = (H[i] >>  8) & 0xFF;
        OutBuffer[i * 4 + 3] = (H[i] >>  0) & 0xFF;
    }
    a=0;
    b=0;
    c=0;
    d=0;
    e=0;
    f=0;
    g=0;
    h=0;
    MemSet(H,0,32);
    MemSet(W,0,128);
    MsgSizeBit=0;
    MemSet(TmpBuffer,0,64);
}
