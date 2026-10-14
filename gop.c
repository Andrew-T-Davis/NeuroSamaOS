#include "gop.h"
#include "efi.h"
#include "flash.h"
#include "str.h"
#include "int.h"
#include "math.h"

extern EFI_GOP_MODE_INFO GopInfo;
extern EFI_GOP_MODE GopMode;
extern HDR_PIXEL* GopBack;
extern void* GopOut;

HDR_PIXEL HDR_Pack(u16 r, u16 g, u16 b, u16 a) {
    return ((u64)r << HDR_R_SHIFT) |
           ((u64)g << HDR_G_SHIFT) |
           ((u64)b << HDR_B_SHIFT) |
           ((u64)a << HDR_A_SHIFT);
}

u16 HDR_Unpack_R(HDR_PIXEL p) {
    return (p >> HDR_R_SHIFT) & 0xFFFF;
}

u16 HDR_Unpack_G(HDR_PIXEL p) {
    return (p >> HDR_G_SHIFT) & 0xFFFF;
}

u16 HDR_Unpack_B(HDR_PIXEL p) {
    return (p >> HDR_B_SHIFT) & 0xFFFF;
}

u16 HDR_Unpack_A(HDR_PIXEL p) {
    return (p >> HDR_A_SHIFT) & 0xFFFF;
}

HDR_PIXEL ARGB_To_HDR(u32 argb) {
    u16 r = (((argb >> 16) & 0xFF) * 65535) / 255;
    u16 g = (((argb >> 8) & 0xFF) * 65535) / 255;
    u16 b = ((argb & 0xFF) * 65535) / 255;
    u16 a = (((argb >> 24) & 0xFF) * 65535) / 255;
    return HDR_Pack(r, g, b, a);
}

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

u8 GetBitCountFromMask(u32 mask) {
    return (u8)__builtin_popcount(mask);
}

HDR_PIXEL HDRMix(HDR_PIXEL dst, HDR_PIXEL src, u8 BitCount) {
    u32 sA = HDR_Unpack_A(src);
    if (sA == 0) return dst;
    if (sA == 0xFFFF) return src;
    
    u32 sR = HDR_Unpack_R(src);
    u32 sG = HDR_Unpack_G(src);
    u32 sB = HDR_Unpack_B(src);
    
    u32 dR = HDR_Unpack_R(dst);
    u32 dG = HDR_Unpack_G(dst);
    u32 dB = HDR_Unpack_B(dst);
    u32 dA = HDR_Unpack_A(dst);
    
    u32 tA = 0xFFFF - sA;
    
    u16 oR = (sR * sA + dR * tA + 0x8000) >> 16;
    u16 oG = (sG * sA + dG * tA + 0x8000) >> 16;
    u16 oB = (sB * sA + dB * tA + 0x8000) >> 16;
    u16 oA = (sA + ((dA * tA + 0x8000) >> 16)) & 0xFFFF;
    
    HDR_PIXEL result = HDR_Pack(oR, oG, oB, oA);
    
    if (BitCount == 10 || BitCount == 12) {
        u64 mask = (1ULL << BitCount) - 1;
        result &= (mask << HDR_R_SHIFT) |
                  (mask << HDR_G_SHIFT) |
                  (mask << HDR_B_SHIFT) |
                  (mask << HDR_A_SHIFT);
    }
    
    return result;
}

void GOPPixel(u32 x, u32 y, HDR_PIXEL hdr) {
    if (x >= GopInfo.PixelsPerScanLine || y >= GopInfo.VerticalResolution) return;

    u32 offset = y * GopInfo.PixelsPerScanLine + x;
    u8 bitCount = 16;
    if (GopInfo.PixelFormat == PixelBitMask) {
        bitCount = GetBitCountFromMask(GopInfo.PixelInformation.RedMask);
    } else if (GopInfo.PixelFormat == PixelRedGreenBlueReserved8BitPerColor ||
               GopInfo.PixelFormat == PixelBlueGreenRedReserved8BitPerColor) {
        bitCount = 8;
    }
    GopBack[offset] = HDRMix(GopBack[offset], hdr, bitCount);
}

void GOPLine(u32 x[2], u32 y[2], HDR_PIXEL hdr){
    int x1 = (int)x[0];
    int y1 = (int)y[0];
    int x2 = (int)x[1];
    int y2 = (int)y[1];
    
    if (y1 == y2) {
        if (x1 > x2) { int tmp = x1; x1 = x2; x2 = tmp; }
        for (int Ix = x1; Ix <= x2; Ix++) {
            GOPPixel((u32)Ix, (u32)y1, hdr);
        }
        return;
    }

    if (x1 == x2) {
        if (y1 > y2) { int tmp = y1; y1 = y2; y2 = tmp; }
        for (int Iy = y1; Iy <= y2; Iy++) {
            GOPPixel((u32)x1, (u32)Iy, hdr);
        }
        return;
    }
    
    s64 dx = S64Abs((s64)x2 - (s64)x1);
    s64 dy = S64Abs((s64)y2 - (s64)y1);
    
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    s64 err = dx - dy;

    while (1) {
        GOPPixel((u32)x1, (u32)y1, hdr);
    
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

void GOPRect(u32 x[2], u32 y[2], HDR_PIXEL hdr){
    u32 startX = x[0], startY = y[0];
    u32 endX = x[1], endY = y[1];
    
    if (startX >= GopInfo.PixelsPerScanLine || startY >= GopInfo.VerticalResolution) return;
    if (endX > GopInfo.PixelsPerScanLine) endX = GopInfo.PixelsPerScanLine;
    if (endY > GopInfo.VerticalResolution) endY = GopInfo.VerticalResolution;
    if (startX >= endX || startY >= endY) return;
    
    for (u32 col = startX; col < endX; col++) {
        GOPPixel(col, startY, hdr);
    }
    
    for (u32 col = startX; col < endX; col++) {
        GOPPixel(col, endY - 1, hdr);
    }
    
    for (u32 row = startY + 1; row < endY - 1; row++) {
        GOPPixel(startX, row, hdr);
    }
    
    for (u32 row = startY + 1; row < endY - 1; row++) {
        GOPPixel(endX - 1, row, hdr);
    }
}

void GOPRectFillFast(u32 startX, u32 endX, u32 startY, u32 endY, HDR_PIXEL hdr) {
    u32 width = endX - startX;
    
    if (width == GopInfo.PixelsPerScanLine) {
        u32 offset = startY * GopInfo.PixelsPerScanLine;
        u32 count = (endY - startY) * GopInfo.PixelsPerScanLine;
        MemSet64(GopBack + offset, hdr, count);
        return;
    }
    
    u32 rowSize = width * sizeof(HDR_PIXEL);
    
    for (u32 y = startY; y < endY; y++) {
        u32 offset = y * GopInfo.PixelsPerScanLine + startX;
        if (rowSize >= 32 && ((u64)(GopBack + offset) & 31) == 0 && (rowSize & 31) == 0) {
            HDR_PIXEL* dst = GopBack + offset;
            MemSet64(dst,hdr,endX-startX);
        } else {
            for (u32 x = startX; x < endX; x++) {
                GopBack[offset + (x - startX)] = hdr;
            }
        }
    }
}

__attribute__((force_align_arg_pointer)) void GOPRectFillAlpha(u32 startX, u32 endX, u32 startY, u32 endY, HDR_PIXEL hdr, u8 bitCount) {
    u32 sA = HDR_Unpack_A(hdr);
    if (sA == 0) return;
    if (sA == 0xFFFF) {
        GOPRectFillFast(startX, endX, startY, endY, hdr);
        return;
    }
    
    for (u32 y = startY; y < endY; y++) {
        for (u32 x = startX; x < endX; x++) {
            u32 offset = y * GopInfo.PixelsPerScanLine + x;
            GopBack[offset] = HDRMix(GopBack[offset], hdr, bitCount);
        }
    }
}

void GOPRectFill(u32 x[2], u32 y[2], HDR_PIXEL hdr){
    u32 startX = x[0], startY = y[0];
    u32 endX = x[1], endY = y[1];
    
    if (startX >= GopInfo.PixelsPerScanLine || startY >= GopInfo.VerticalResolution) return;
    if (endX > GopInfo.PixelsPerScanLine) endX = GopInfo.PixelsPerScanLine;
    if (endY > GopInfo.VerticalResolution) endY = GopInfo.VerticalResolution;
    if (startX >= endX || startY >= endY) return;
    
    u8 bitCount = 16;
    if (GopInfo.PixelFormat == PixelBitMask) {
        bitCount = GetBitCountFromMask(GopInfo.PixelInformation.RedMask);
    } else if (GopInfo.PixelFormat == PixelRedGreenBlueReserved8BitPerColor ||
               GopInfo.PixelFormat == PixelBlueGreenRedReserved8BitPerColor) {
        bitCount = 8;
    }
    
    u32 sA = HDR_Unpack_A(hdr);
    if (sA == 0xFFFF) {
        GOPRectFillFast(startX, endX, startY, endY, hdr);
    } else {
        GOPRectFillAlpha(startX, endX, startY, endY, hdr, bitCount);
    }
}

void GOPChar(u32 x, u32 y, char Char, HDR_PIXEL Back, HDR_PIXEL Front, u32 Font[256][19][8]) {
    if (x>=GopInfo.HorizontalResolution||y>=GopInfo.VerticalResolution||x+8>=GopInfo.HorizontalResolution||y+19>=GopInfo.VerticalResolution) return;
    if (HDR_Unpack_A(Front) == 0x0000 && HDR_Unpack_A(Back) == 0x0000) return;
    GOPRectFill((u32[2]){x, x+8}, (u32[2]){y, y+19}, Back);
    if (HDR_Unpack_A(Front) == 0x0000) return;
    
    HDR_PIXEL *dst = GopBack + y * GopInfo.PixelsPerScanLine + x;
    u32 *mask = Font[(int)Char][0];

    if (HDR_Unpack_A(Front) == 0xFFFF) {
        for (int row = 0; row < 19; row++) {
            for (int col = 0; col < 8; col++) {
                if (mask[col]) dst[col] = Front;
            }
            dst += GopInfo.PixelsPerScanLine;
            mask += 8;
        }
    } else {
        for (int row = 0; row < 19; row++) {
            for (int col = 0; col < 8; col++) {
                if (mask[col]) {
                    dst[col] = HDRMix(dst[col], Front, 16);
                }
            }
            dst += GopInfo.PixelsPerScanLine;
            mask += 8;
        }
    }
}

void GOPString(u32 x, u32 y, char* String, HDR_PIXEL Back, HDR_PIXEL Front, u32 Font[256][19][8]) {
    u32 NewX = x;
    u32 NewY = y;
    if ((x>=GopInfo.HorizontalResolution||y>=GopInfo.VerticalResolution)||x+8>=GopInfo.HorizontalResolution||y+19>=GopInfo.VerticalResolution) return;
    for (u32 Idx = 0; String[Idx]; Idx++) {
        char ch = String[Idx];
        if (ch == '\n') {
            NewY += 19;
            NewX = x;
            continue;
        }
        if (ch == '\r') {
            NewX = x;
            continue;
        }
        if (ch == '\b') {
            NewX -= 8;
            continue;
        }
        if (ch == '\t') {
            NewX += 8*4;
            continue;
        }
        GOPChar(NewX, NewY, ch, Back, Front, Font);
        if ((NewX + 8 >= GopInfo.HorizontalResolution) || (NewX >= GopInfo.HorizontalResolution)) break;
        else NewX += 8;
    }
}

void GOPStringWH(u32 x, u32 y, u32 w, u32 h, char* String, HDR_PIXEL Back, HDR_PIXEL Front, u32 Font[256][19][8]) {
    u32 NewX = x;
    u32 NewY = y;
    if (x>=GopInfo.HorizontalResolution||y>=GopInfo.VerticalResolution) return;
    for (u32 Idx = 0; String[Idx]; Idx++) {
        char ch = String[Idx];
        if (ch == '\n') {
            NewY += 19;
            NewX = x;
            if (NewY > y+h) break;
            continue;
        }
        if (ch == '\r') {
            NewX = x;
            continue;
        }
        if (ch == '\b') {
            if (NewX - 8 >= x) {
                NewX -= 8;
            }
            continue;
        }
        if (ch == '\t') {
            if ((NewX + (8*4) >= x+w) || (NewX >= x+w)) {
                break;
            }
            NewX += 8*4;
            continue;
        }
        GOPChar(NewX, NewY, ch, Back, Front, Font);
        if ((NewX + 8 >= x+w) || (NewX >= x+w)) {
            break;
        } else {
            NewX += 8;
        }
        if (NewY >= y+h) break;
    }
}

void GOPClear(HDR_PIXEL hdr) {
    MemSet64(GopBack, hdr, GopInfo.PixelsPerScanLine*GopInfo.VerticalResolution);
    __asm__("sfence");
}

void GOPClearAlpha(HDR_PIXEL hdr) {
    u32 totalPixels = GopInfo.PixelsPerScanLine * GopInfo.VerticalResolution;
    u8 bitCount = 16;
    if (GopInfo.PixelFormat == PixelBitMask) {
        bitCount = GetBitCountFromMask(GopInfo.PixelInformation.RedMask);
    } else if (GopInfo.PixelFormat == PixelRedGreenBlueReserved8BitPerColor ||
               GopInfo.PixelFormat == PixelBlueGreenRedReserved8BitPerColor) {
        bitCount = 8;
    }

    u32 sA = HDR_Unpack_A(hdr);
    if (sA == 0) return;
    
    if (sA == 0xFFFF) {
        MemSet64(GopBack, hdr, totalPixels);
        return;
    }
    
    for (u32 i = 0; i < totalPixels; i++) {
        GopBack[i] = HDRMix(GopBack[i], hdr, bitCount);
    }
}

u32 GOPackPixel(HDR_PIXEL p, u32 rShift, u32 gShift, u32 bShift, u32 aShift, u32 rBits, u32 gBits, u32 bBits, u32 aBits) {
    u32 r = (p >> HDR_R_SHIFT) & 0xFFFF;
    u32 g = (p >> HDR_G_SHIFT) & 0xFFFF;
    u32 b = (p >> HDR_B_SHIFT) & 0xFFFF;
    u32 a = (p >> HDR_A_SHIFT) & 0xFFFF;
    
    r = (r * ((1 << rBits) - 1) + 32767) / 65535;
    g = (g * ((1 << gBits) - 1) + 32767) / 65535;
    b = (b * ((1 << bBits) - 1) + 32767) / 65535;
    a = (a * ((1 << aBits) - 1) + 32767) / 65535;
    
    return (r << rShift) | (g << gShift) | (b << bShift) | (a << aShift);
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
        
        if (rMask == 0xFFFF000000000000ULL && gMask == 0x0000FFFF00000000ULL && 
            bMask == 0x00000000FFFF0000ULL && aMask == 0x000000000000FFFFULL) {
            MemCopy(GopOut, GopBack, pixelCount * sizeof(HDR_PIXEL));
            return;
        }
        
        u32 rShift = (u32)__builtin_ctzll(rMask);
        u32 gShift = (u32)__builtin_ctzll(gMask);
        u32 bShift = (u32)__builtin_ctzll(bMask);
        u32 aShift = (u32)__builtin_ctzll(aMask);
        u32 rBits = (u32)__builtin_popcountll(rMask);
        u32 gBits = (u32)__builtin_popcountll(gMask);
        u32 bBits = (u32)__builtin_popcountll(bMask);
        u32 aBits = (u32)__builtin_popcountll(aMask);
        
        if (rBits == 16 && gBits == 16 && bBits == 16 && aBits == 16 && 
            rShift == 48 && gShift == 32 && bShift == 16 && aShift == 0) {
            MemCopy(GopOut, GopBack, pixelCount * sizeof(HDR_PIXEL));
            return;
        }
        
        for (u32 i = 0; i < pixelCount; i++) {
            fb[i] = GOPackPixel(GopBack[i], rShift, gShift, bShift, aShift, rBits, gBits, bBits, aBits);
        }
        return;
    }

    if (GopInfo.PixelFormat == PixelRedGreenBlueReserved8BitPerColor) {
        for (u32 i = 0; i < pixelCount; i++) {
            HDR_PIXEL p = GopBack[i];
            u32 r = ((p >> HDR_R_SHIFT) & 0xFFFF) >> 8;
            u32 g = ((p >> HDR_G_SHIFT) & 0xFFFF) >> 8;
            u32 b = ((p >> HDR_B_SHIFT) & 0xFFFF) >> 8;
            u32 a = ((p >> HDR_A_SHIFT) & 0xFFFF) >> 8;
            fb[i] = (a << 24) | (b << 16) | (g << 8) | r;
        }
    } else if (GopInfo.PixelFormat == PixelBlueGreenRedReserved8BitPerColor) {
        for (u32 i = 0; i < pixelCount; i++) {
            HDR_PIXEL p = GopBack[i];
            u32 r = ((p >> HDR_R_SHIFT) & 0xFFFF) >> 8;
            u32 g = ((p >> HDR_G_SHIFT) & 0xFFFF) >> 8;
            u32 b = ((p >> HDR_B_SHIFT) & 0xFFFF) >> 8;
            u32 a = ((p >> HDR_A_SHIFT) & 0xFFFF) >> 8;
            fb[i] = (a << 24) | (r << 16) | (g << 8) | b;
        }
    }
}
