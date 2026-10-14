#pragma once

#include "int.h"

void PingPong(void** Ping,void** Pong);
void MemCopy(void* D,const void* S,Size64 Size);
void MemCopySize32CountByte(u32* D,const u32* S,Size64 size_bytes);
void StrAdd(char* Add,const char* S,Size64 Size);
void MemSet(void* D,const u8 S,Size64 Size);
void MemSet16(void* D,const u16 S,Size64 Size);
void MemSet32(void* D,const u32 S,Size64 Size);
void MemSet64(void* D,const u64 S,Size64 Size);
Bool StrIs(const char* D,const char* S);
u64 StrLen(const char* S);
void Bit64Str(u64 N,char* Buf);
int StrSplit(char* Str, char Split, char** left, char** right);
Bool StrIsNoCase(const char* D, const char* S);
Bool StrIsWithSize(const char* D, const char* S,Size64 Size);
Bool StrIsNoCaseWithSize(const char* D, const char* S,Size64 Size);
u64 HowManyCharInString(char* String,char Char);
u64 HowManyCharInStringWithSize(char* String,char Char,Size64 Size);

Bool MemCmp(const void* S1, const void* S2, u32 N);

u8 StrToU8(char* S, Bool DecOrHex);
u16 StrToU16(char* S, Bool DecOrHex);
u32 StrToU32(char* S, Bool DecOrHex);
u64 StrToU64(char* S, Bool DecOrHex);
f32 StrToF32(char* S);
f64 StrToF64(char* S);
f80 StrToF80(char* S);

Size64 U8ToStr (u8  val, char* buf, Bool hex);
Size64 U16ToStr(u16 val, char* buf, Bool hex);
Size64 U32ToStr(u32 val, char* buf, Bool hex);
Size64 U64ToStr(u64 val, char* buf, Bool hex);
Size64 F32ToStr(f32 val, char* buf);
Size64 F64ToStr(f64 val, char* buf);
Size64 F80ToStr(f80 val, char* buf);

void Reverse(char* start, char* end);

void PingPongNotBuffer(void* a, void* b, u64 size);