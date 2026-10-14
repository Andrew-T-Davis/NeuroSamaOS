#include "ioapic.h"
#include "int.h"

u64 IOApicBase = 0xFEC00000ULL;

void IOApicWrite(u32 Reg, u32 Data, u64 Offset) {
    volatile u32* IOApicReg = (volatile u32*)(IOApicBase + 0x00 + Offset);
    volatile u32* IOApicWin = (volatile u32*)(IOApicBase + 0x10 + Offset);
    *IOApicReg = Reg;
    *IOApicWin = Data;
}

u32 IOApicRead(u8 Reg, u64 Offset) {
    volatile u32* IOApicReg = (volatile u32*)(IOApicBase + 0x00 + Offset);
    volatile u32* IOApicWin = (volatile u32*)(IOApicBase + 0x10 + Offset);

    *IOApicReg = Reg;
    return *IOApicWin;
}

void LocalApicEOI(){
    volatile u32* LocalApic = (volatile u32*)(0xFEE000B0ULL);
    *LocalApic = 0;
}