#pragma once

#include "int.h"

#define ARGB2BGRA(argb) __builtin_bswap32(argb)

#define HDR_R_SHIFT  48
#define HDR_G_SHIFT  32
#define HDR_B_SHIFT  16
#define HDR_A_SHIFT  0

#define HDR_R_MASK  0xFFFF000000000000ULL
#define HDR_G_MASK  0x0000FFFF00000000ULL
#define HDR_B_MASK  0x00000000FFFF0000ULL
#define HDR_A_MASK  0x000000000000FFFFULL

typedef u64 HDR_PIXEL;

HDR_PIXEL HDR_Pack(u16 r, u16 g, u16 b, u16 a);
u16 HDR_Unpack_R(HDR_PIXEL p);
u16 HDR_Unpack_G(HDR_PIXEL p);
u16 HDR_Unpack_B(HDR_PIXEL p);
u16 HDR_Unpack_A(HDR_PIXEL p);
HDR_PIXEL ARGB_To_HDR(u32 argb);
u32 ARGBMix(u32 dst, u32 src);
u8 GetBitCountFromMask(u32 mask);

void GOPPixel(u32 x, u32 y, HDR_PIXEL hdr);
void GOPRect(u32 x[2], u32 y[2], HDR_PIXEL hdr);
void GOPRectFill(u32 x[2], u32 y[2], HDR_PIXEL hdr);
void GOPLine(u32 x[2], u32 y[2], HDR_PIXEL hdr);
void GOPChar(u32 x, u32 y, char Char, HDR_PIXEL Back, HDR_PIXEL Front, u32 Font[256][19][8]);
void GOPString(u32 x, u32 y, char* String, HDR_PIXEL Back, HDR_PIXEL Front, u32 Font[256][19][8]);
void GOPStringWH(u32 x, u32 y, u32 w, u32 h, char* String, HDR_PIXEL Back, HDR_PIXEL Front, u32 Font[256][19][8]);
void GOPClear(HDR_PIXEL hdr);
void GOPClearAlpha(HDR_PIXEL hdr);
void GOPFlash();