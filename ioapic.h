#pragma once

#include "int.h"

extern u64 IOApicBase;

void IOApicWrite(u32 Reg, u32 Data, u64 Offset);
void LocalApicEOI();
u32 IOApicRead(u8 Reg, u64 Offset);