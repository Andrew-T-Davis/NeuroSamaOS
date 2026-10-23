#pragma clang optimize off
#include "debug.h"
#include "int.h"
#include "port.h"

static const char hex[]="0123456789ABCDEF";

void DebugInit(void) {
    outb(0x3F8 + 1, 0x00);
    outb(0x3F8 + 3, 0x80);
    outb(0x3F8 + 0, 0x01);
    outb(0x3F8 + 1, 0x00);
    outb(0x3F8 + 3, 0x03);
    outb(0x3F8 + 2, 0xC7);
    outb(0x3F8 + 4, 0x0B);
    inb(0x3F8 + 2);
    inb(0x3F8 + 0);
}

void DebugChar(const char C) {
    outb(0x3F8, (u8)C);
}

void DebugStr(const char *S){
    if (!S) return;
    while (*S) DebugChar(*S++);
}

void DebugU64(u64 LLU) {
    DebugChar(hex[(LLU&0xF000000000000000)>>60]);
    DebugChar(hex[(LLU&0x0F00000000000000)>>56]);
    DebugChar(hex[(LLU&0x00F0000000000000)>>52]);
    DebugChar(hex[(LLU&0x000F000000000000)>>48]);
    DebugChar(hex[(LLU&0x0000F00000000000)>>44]);
    DebugChar(hex[(LLU&0x00000F0000000000)>>40]);
    DebugChar(hex[(LLU&0x000000F000000000)>>36]);
    DebugChar(hex[(LLU&0x0000000F00000000)>>32]);
    DebugChar(hex[(LLU&0x00000000F0000000)>>28]);
    DebugChar(hex[(LLU&0x000000000F000000)>>24]);
    DebugChar(hex[(LLU&0x0000000000F00000)>>20]);
    DebugChar(hex[(LLU&0x00000000000F0000)>>16]);
    DebugChar(hex[(LLU&0x000000000000F000)>>12]);
    DebugChar(hex[(LLU&0x0000000000000F00)>>8 ]);
    DebugChar(hex[(LLU&0x00000000000000F0)>>4 ]);
    DebugChar(hex[(LLU&0x000000000000000F)>>0 ]);
}

void DebugU32(u32 LU) {
    DebugChar(hex[(LU&0x00000000F0000000)>>28]);
    DebugChar(hex[(LU&0x000000000F000000)>>24]);
    DebugChar(hex[(LU&0x0000000000F00000)>>20]);
    DebugChar(hex[(LU&0x00000000000F0000)>>16]);
    DebugChar(hex[(LU&0x000000000000F000)>>12]);
    DebugChar(hex[(LU&0x0000000000000F00)>>8 ]);
    DebugChar(hex[(LU&0x00000000000000F0)>>4 ]);
    DebugChar(hex[(LU&0x000000000000000F)>>0 ]);
}

void DebugU16(u16 D) {
    DebugChar(hex[(D&0x000000000000F000)>>12]);
    DebugChar(hex[(D&0x0000000000000F00)>>8 ]);
    DebugChar(hex[(D&0x00000000000000F0)>>4 ]);
    DebugChar(hex[(D&0x000000000000000F)>>0 ]);
}

void DebugU8(u8 B) {
    DebugChar(hex[(B&0x00000000000000F0)>>4 ]);
    DebugChar(hex[(B&0x000000000000000F)>>0 ]);
}

void DebugI64(s64 LLU)
{
    if (LLU < 0)
    {
        DebugChar('-');
        DebugU64((u64)(-LLU));
    }
    else
    {
        DebugU64((u64)LLU);
    }
}

void DebugI32(s32 LU)
{
    if (LU < 0)
    {
        DebugChar('-');
        DebugU32((u32)(-LU));
    }
    else
    {
        DebugU32((u32)LU);
    }
}

void DebugI16(s16 D)
{
    if (D < 0)
    {
        DebugChar('-');
        DebugU16((u16)(-D));
    }
    else
    {
        DebugU16((u16)D);
    }
}

void DebugI8(s8 B)
{
    if (B < 0)
    {
        DebugChar('-');
        DebugU8((u8)(-B));
    }
    else
    {
        DebugU8((u8)B);
    }
}
#pragma clang optimize on