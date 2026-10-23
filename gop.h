#pragma once

#include "int.h"

#define PACK_COLOR(r,g,b,a) ((u32)(a<<24|r<<16|g<<8|b))
#define ARGB2BGRA(argb) __builtin_bswap32(argb)

u32 ARGBMix(u32 dst, u32 src);

void GOPPixel(u32 x, u32 y, u32 color);
void GOPRect(u32 x[2], u32 y[2], u32 color);
void GOPRectFill(u32 x[2], u32 y[2], u32 color);
void GOPLine(u32 x[2], u32 y[2], u32 color);
void GOPChar(u32 x, u32 y, u16 Char, u32 Back, u32 Front, u16 Font[65536][19]);
void GOPString(u32 x, u32 y, char* String, u32 Back, u32 Front, u16 Font[65536][19]);
void GOPStringWH(u32 x, u32 y, u32 w, u32 h, char* String, u32 Back, u32 Front, u16 Font[65536][19]);
void GOPClear(u32 color);
void GOPClearAlpha(u32 color);
void GOPFlash();