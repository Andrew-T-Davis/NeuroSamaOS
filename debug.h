#pragma once

#include "int.h"

extern char ScreenDebug[1501];
extern u64 ScreenDebugCurX;

void DebugInit(void);

void DebugChar(const char C);
void DebugStr(const char* S);
void DebugU64(u64 LLU);
void DebugU32(u32 LU);
void DebugU16(u16 D);
void DebugU8(u8 B);

void DebugI64(s64 LLU);
void DebugI32(s32 LU);
void DebugI16(s16 D);
void DebugI8(s8 B);
