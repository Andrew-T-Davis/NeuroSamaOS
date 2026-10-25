#include "gop.h"
#include "efi.h"
#include "flash.h"
#include "str.h"
#include "int.h"
#include "math.h"

extern EFI_GOP_MODE_INFO GopInfo;
extern EFI_GOP_MODE GopMode;
extern u32* GopBack;
extern u32* GopOut;

u32 ARGBMix(u32 dst, u32 src) {
    u32 sA = (src >> 24) & 0xFF;
    if (sA == 255) return src;
    if (sA == 0)   return dst;

    u32 dA = 255 - sA;

    u32 srcRGB = src & 0x00FFFFFF;
    u32 dstRGB = dst & 0x00FFFFFF;

    u32 r = (((srcRGB >> 16) & 0xFF) * sA + ((dstRGB >> 16) & 0xFF) * dA + 128) >> 8;
    u32 g = (((srcRGB >> 8)  & 0xFF) * sA + ((dstRGB >> 8)  & 0xFF) * dA + 128) >> 8;
    u32 b = ((srcRGB & 0xFF) * sA + (dstRGB & 0xFF) * dA + 128) >> 8;

    return (sA << 24) | (r << 16) | (g << 8) | b;
}

void GOPPixel(u32 x, u32 y, u32 color) {
    if (x >= GopInfo.PixelsPerScanLine || y >= GopInfo.VerticalResolution) return;

    u64 offset = y * GopInfo.PixelsPerScanLine + x;
    GopBack[offset] = ARGBMix(GopBack[offset], color);
}

void GOPLine(u32 x[2], u32 y[2], u32 color){
    int x1 = (int)x[0];
    int y1 = (int)y[0];
    int x2 = (int)x[1];
    int y2 = (int)y[1];

    if (y1 == y2) {
        if (x1 > x2) { int tmp = x1; x1 = x2; x2 = tmp; }
        for (int Ix = x1; Ix <= x2; Ix++) {
            GOPPixel((u32)Ix, (u32)y1, color);
        }
        return;
    }

    if (x1 == x2) {
        if (y1 > y2) { int tmp = y1; y1 = y2; y2 = tmp; }
        for (int Iy = y1; Iy <= y2; Iy++) {
            GOPPixel((u32)x1, (u32)Iy, color);
        }
        return;
    }

    s64 dx = S64Abs((s64)x2 - (s64)x1);
    s64 dy = S64Abs((s64)y2 - (s64)y1);

    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    s64 err = dx - dy;

    while (1) {
        GOPPixel((u32)x1, (u32)y1, color);

        if (x1 == x2 && y1 == y2) {
            break;
        }

        s64 e2 = 2 * err;

        if (e2 >= -dy) {
            err -= dy;
            x1 += sx;
        }

        if (e2 <= dx) {
            err += dx;
            y1 += sy;
        }
    }
}

void GOPRect(u32 x[2], u32 y[2], u32 color){
    u32 StartX = x[0], StartY = y[0];
    u32 EndX = x[1], EndY = y[1];

    if (StartX >= GopInfo.PixelsPerScanLine || StartY >= GopInfo.VerticalResolution) return;
    if (EndX > GopInfo.PixelsPerScanLine) EndX = GopInfo.PixelsPerScanLine;
    if (EndY > GopInfo.VerticalResolution) EndY = GopInfo.VerticalResolution;
    if (StartX >= EndX || StartY >= EndY) return;

    for (u32 col = StartX; col < EndX; col++) {
        GOPPixel(col, StartY, color);
    }

    for (u32 col = StartX; col < EndX; col++) {
        GOPPixel(col, EndY - 1, color);
    }

    for (u32 Row = StartY + 1; Row < EndY - 1; Row++) {
        GOPPixel(StartX, Row, color);
    }

    for (u32 Row = StartY + 1; Row < EndY - 1; Row++) {
        GOPPixel(EndX - 1, Row, color);
    }
}

void GOPRectFillFast(u32 StartX, u32 EndX, u32 StartY, u32 EndY, u32 color) {
    u32 Width = EndX - StartX;

    if (Width == GopInfo.PixelsPerScanLine) {
        u32 offset = StartY * GopInfo.PixelsPerScanLine;
        u32 count = (EndY - StartY) * GopInfo.PixelsPerScanLine;
        MemSet32(GopBack + offset, color, count);
        return;
    }

    u32 RowSize = Width * sizeof(u32);

    for (u32 y = StartY; y < EndY; y++) {
        u32 offset = y * GopInfo.PixelsPerScanLine + StartX;
        if (RowSize >= 32 && ((u64)(GopBack + offset) & 31) == 0 && (RowSize & 31) == 0) {
            u32* dst = GopBack + offset;
            MemSet32(dst, color, EndX - StartX);
        } else {
            for (u32 x = StartX; x < EndX; x++) {
                GopBack[offset + (x - StartX)] = color;
            }
        }
    }
}

void GOPRectFillAlpha(u32 StartX, u32 EndX, u32 StartY, u32 EndY, u32 color) {
    u32 sA = (color >> 24) & 0xFF;
    if (sA == 0) return;
    if (sA == 0xFF) {
        GOPRectFillFast(StartX, EndX, StartY, EndY, color);
        return;
    }

    for (u32 y = StartY; y < EndY; y++) {
        for (u32 x = StartX; x < EndX; x++) {
            u32 offset = y * GopInfo.PixelsPerScanLine + x;
            GopBack[offset] = ARGBMix(GopBack[offset], color);
        }
    }
}

void GOPRectFill(u32 x[2], u32 y[2], u32 color){
    u32 StartX = x[0], StartY = y[0];
    u32 EndX = x[1], EndY = y[1];

    if (StartX >= GopInfo.PixelsPerScanLine || StartY >= GopInfo.VerticalResolution) return;
    if (EndX > GopInfo.PixelsPerScanLine) EndX = GopInfo.PixelsPerScanLine;
    if (EndY > GopInfo.VerticalResolution) EndY = GopInfo.VerticalResolution;
    if (StartX >= EndX || StartY >= EndY) return;

    u32 sA = (color >> 24) & 0xFF;
    if (sA == 0xFF) {
        GOPRectFillFast(StartX, EndX, StartY, EndY, color);
    } else {
        GOPRectFillAlpha(StartX, EndX, StartY, EndY, color);
    }
}

void GOPChar(u32 x, u32 y, u16 Char, u32 Back, u32 Front, u16 Font[65536][19]) {
    if (x >= GopInfo.HorizontalResolution || y >= GopInfo.VerticalResolution ||
        x + 16 > GopInfo.HorizontalResolution || y + 19 > GopInfo.VerticalResolution) return;
    if (((Front >> 24) & 0xFF) == 0 && ((Back >> 24) & 0xFF) == 0) return;

    GOPRectFill((u32[2]){x, x + 8*(1+Char<0xFF)}, (u32[2]){y, y + 19}, Back);
    if (((Front >> 24) & 0xFF) == 0) return;

    u32 *dst = GopBack + y * GopInfo.PixelsPerScanLine + x;
    u16 *mask = Font[Char];

    u32 sA = (Front >> 24) & 0xFF;
    if (sA == 0xFF) {
        for (int Row = 0; Row < 19; Row++) {
            u16 m = mask[Row];
            for (int col = 0; col < 16; col++) {
                if (m & (0x8000u >> col)) dst[col] = Front;
            }
            dst += GopInfo.PixelsPerScanLine;
        }
    } else {
        for (int Row = 0; Row < 19; Row++) {
            u16 m = mask[Row];
            for (int col = 0; col < 16; col++) {
                if (m & (0x8000u >> col)) dst[col] = ARGBMix(dst[col], Front);
            }
            dst += GopInfo.PixelsPerScanLine;
        }
    }
}

void GOPString(u32 x, u32 y, char *String, u32 Back, u32 Front, u16 Font[65536][19], Bool RightBreakOrEnter) {
    u32 NewX = x, NewY = y;
    if (x >= GopInfo.HorizontalResolution || y >= GopInfo.VerticalResolution ||
        x + 8 > GopInfo.HorizontalResolution || y + 19 > GopInfo.VerticalResolution) return;

    const u8 *p = (const u8*)String;
    while (*p) {
        u32 cp;
        u8 c0 = p[0];

        if (c0 < 0x80) {
            cp = c0; p += 1;
        } else if ((c0 & 0xE0) == 0xC0 && p[1] && (p[1] & 0xC0) == 0x80) {
            cp = ((u32)(c0 & 0x1F) << 6) | (p[1] & 0x3F);
            p += 2;
        } else if ((c0 & 0xF0) == 0xE0 && p[1] && p[2] &&
                   (p[1] & 0xC0) == 0x80 && (p[2] & 0xC0) == 0x80) {
            cp = ((u32)(c0 & 0x0F) << 12) | ((u32)(p[1] & 0x3F) << 6) | (p[2] & 0x3F);
            p += 3;
        } else if ((c0 & 0xF8) == 0xF0 && p[1] && p[2] && p[3] &&
                   (p[1] & 0xC0) == 0x80 && (p[2] & 0xC0) == 0x80 && (p[3] & 0xC0) == 0x80) {
            cp = ((u32)(c0 & 0x07) << 18) | ((u32)(p[1] & 0x3F) << 12)
               | ((u32)(p[2] & 0x3F) << 6) | (p[3] & 0x3F);
            p += 4;
        } else {
            cp = 0xFFFD; p += 1;
        }

        if (cp == '\n') { NewY += 19; NewX = x; continue; }
        if (cp == '\r') { NewX = x; continue; }
        if (cp == '\b') { if (NewX >= x + 8) NewX -= 8; continue; }
        if (cp == '\t') { NewX += 8 * 4; continue; }

        u32 Gw = (cp < 0x80) ? 8 : 16;

        if (NewX + Gw > GopInfo.HorizontalResolution) {
            if (RightBreakOrEnter) {
                NewX = x;
                NewY += 19;
            }
            else break;
        }
        GOPChar(NewX, NewY, (u16)cp, Back, Front, Font);
        NewX += Gw;
    }
}

void GOPStringWH(u32 x, u32 y, u32 w, u32 h, char* String, u32 Back, u32 Front, u16 Font[65536][19], Bool RightBreakOrEnter) {
    u32 NewX = x;
    u32 NewY = y;
    if (x >= GopInfo.HorizontalResolution || y >= GopInfo.VerticalResolution) return;

    const u8 *p = (const u8*)String;
    while (*p) {
        u32 cp;
        u8 c0 = p[0];

        if (c0 < 0x80) {
            cp = c0; p += 1;
        } else if ((c0 & 0xE0) == 0xC0 && p[1] && (p[1] & 0xC0) == 0x80) {
            cp = ((u32)(c0 & 0x1F) << 6) | (p[1] & 0x3F);
            p += 2;
        } else if ((c0 & 0xF0) == 0xE0 && p[1] && p[2] &&
                   (p[1] & 0xC0) == 0x80 && (p[2] & 0xC0) == 0x80) {
            cp = ((u32)(c0 & 0x0F) << 12) | ((u32)(p[1] & 0x3F) << 6) | (p[2] & 0x3F);
            p += 3;
        } else if ((c0 & 0xF8) == 0xF0 && p[1] && p[2] && p[3] &&
                   (p[1] & 0xC0) == 0x80 && (p[2] & 0xC0) == 0x80 && (p[3] & 0xC0) == 0x80) {
            cp = ((u32)(c0 & 0x07) << 18) | ((u32)(p[1] & 0x3F) << 12)
               | ((u32)(p[2] & 0x3F) << 6) | (p[3] & 0x3F);
            p += 4;
        } else {
            cp = 0xFFFD; p += 1;
        }

        if (cp == '\n') {
            NewY += 19;
            NewX = x;
            if (NewY > y + h) break;
            continue;
        }
        if (cp == '\r') {
            NewX = x;
            continue;
        }
        if (cp == '\b') {
            if (NewX - 8 >= x) NewX -= 8;
            continue;
        }
        if (cp == '\t') {
            if ((NewX + (8 * 4) >= x + w) || (NewX >= x + w)) break;
            NewX += 8 * 4;
            continue;
        }

        u32 Gw = (cp < 0x80) ? 8 : 16;

        if ((NewX + Gw >= x + w) || (NewX >= x + w)) {
            if (RightBreakOrEnter) {
                NewX = x;
                NewY += 19;
            }
            else break;
        }
        GOPChar(NewX, NewY, (u16)cp, Back, Front, Font);
        NewX += Gw;

        if (NewY >= y + h) break;
    }
}

void GOPClear(u32 color) {
    MemSet32(GopBack, color, GopInfo.PixelsPerScanLine * GopInfo.VerticalResolution);
    __asm__("sfence");
}

void GOPClearAlpha(u32 color) {
    u32 totalPixels = GopInfo.PixelsPerScanLine * GopInfo.VerticalResolution;

    u32 sA = (color >> 24) & 0xFF;
    if (sA == 0) return;

    if (sA == 0xFF) {
        MemSet32(GopBack, color, totalPixels);
        return;
    }

    for (u32 i = 0; i < totalPixels; i++) {
        GopBack[i] = ARGBMix(GopBack[i], color);
    }
}

void GOPFlash() {
    MemFlash();
    u32 pixelCount = GopInfo.PixelsPerScanLine * GopInfo.VerticalResolution;
    u32* fb = (u32*)GopOut;

    if (GopInfo.PixelFormat == PixelBitMask) {
        u64 rMask = GopInfo.PixelInformation.RedMask;
        u64 gMask = GopInfo.PixelInformation.GreenMask;
        u64 bMask = GopInfo.PixelInformation.BlueMask;
        u64 aMask = GopInfo.PixelInformation.ReservedMask;

        u32 rShift = (u32)__builtin_ctzll(rMask);
        u32 gShift = (u32)__builtin_ctzll(gMask);
        u32 bShift = (u32)__builtin_ctzll(bMask);
        u32 aShift = (u32)__builtin_ctzll(aMask);
        u32 rBits = (u32)__builtin_popcountll(rMask);
        u32 gBits = (u32)__builtin_popcountll(gMask);
        u32 bBits = (u32)__builtin_popcountll(bMask);
        u32 aBits = (u32)__builtin_popcountll(aMask);

        for (u32 i = 0; i < pixelCount; i++) {
            u32 p = GopBack[i];
            u32 r = (p >> 16) & 0xFF;
            u32 g = (p >> 8)  & 0xFF;
            u32 b = p & 0xFF;
            u32 a = (p >> 24) & 0xFF;

            r = rBits == 8 ? r : (r * ((1u << rBits) - 1) + 127) / 255;
            g = gBits == 8 ? g : (g * ((1u << gBits) - 1) + 127) / 255;
            b = bBits == 8 ? b : (b * ((1u << bBits) - 1) + 127) / 255;
            a = aBits == 8 ? a : (a * ((1u << aBits) - 1) + 127) / 255;

            fb[i] = (r << rShift) | (g << gShift) | (b << bShift) | (a << aShift);
        }
        return;
    }

    if (GopInfo.PixelFormat == PixelRedGreenBlueReserved8BitPerColor) {
        for (u32 i = 0; i < pixelCount; i++) {
            u32 p = GopBack[i];
            u32 r = (p >> 16) & 0xFF;
            u32 g = (p >> 8)  & 0xFF;
            u32 b = p & 0xFF;
            u32 a = (p >> 24) & 0xFF;
            fb[i] = (a << 24) | (b << 16) | (g << 8) | r;
        }
    } else if (GopInfo.PixelFormat == PixelBlueGreenRedReserved8BitPerColor) {
        for (u32 i = 0; i < pixelCount; i++) {
            u32 p = GopBack[i];
            u32 r = (p >> 16) & 0xFF;
            u32 g = (p >> 8)  & 0xFF;
            u32 b = p & 0xFF;
            u32 a = (p >> 24) & 0xFF;
            fb[i] = (a << 24) | (r << 16) | (g << 8) | b;
        }
    }
}