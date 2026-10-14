#pragma once

#include "int.h"

typedef enum {
    REG_RAX=0, REG_RCX=1, REG_RDX=2, REG_RBX=3, REG_RSP=4, REG_RBP=5, REG_RSI=6, REG_RDI=7,
    REG_R8=8,  REG_R9=9,  REG_R10=10,REG_R11=11,REG_R12=12,REG_R13=13,REG_R14=14,REG_R15=15
} Reg;

typedef enum {
    COND_O  = 0x0, COND_NO = 0x1, COND_B  = 0x2, COND_AE = 0x3,
    COND_E  = 0x4, COND_NE = 0x5, COND_BE = 0x6, COND_A  = 0x7,
    COND_S  = 0x8, COND_NS = 0x9, COND_P  = 0xA, COND_NP = 0xB,
    COND_L  = 0xC, COND_GE = 0xD, COND_LE = 0xE, COND_G  = 0xF
} Condition;

typedef enum {
    OP_ADD = 0x01,
    OP_SUB = 0x29,
    OP_CMP = 0x39
} OpType;

#define TypeChar (1<<0)
#define TypeUnsigned (1<<1)
#define TypeSigned (1<<2)
#define TypeLong (1<<3)
#define TypeLongLong (1<<4)
#define TypeShort (1<<5)
#define TypePointer (1<<6)
#define TypeVoid (1<<7)
#define TypeList (1<<8)
#define VarUnUse 0

typedef struct{
    u64 SerialNumber;
    u16 Type;
    u64 Size;
    void* DataPoint;
}__attribute__((packed)) JITVar;

typedef struct{
    u64 SerialNumber;
    u16 Type;
    void* FuncPoint;
}__attribute__((packed)) JITFunc;

u64 GetVarCount();
JITVar GetVar(u64 SerialNumber);
void SetVarDataPoint(u64 SerialNumber,u64* DataPoint);
void* GetVarDataPoint(u64 SerialNumber);
u16 GetVarType(u64 SerialNumber);
void SetVarType(u64 SerialNumber, u16 Type);
u64 VarAdd(JITVar Var);
void VarDel(u64 SerialNumber);

u64 GetFuncCount();
JITFunc GetFunc(u64 SerialNumber);
void SetFuncPoint(u64 SerialNumber,u64* FuncPoint);
void* GetFuncPoint(u64 SerialNumber);
u16 GetFuncType(u64 SerialNumber);
void SetFuncType(u64 SerialNumber, u16 Type);
u64 FuncAdd(JITFunc Func);
void FuncDel(u64 SerialNumber);

extern __attribute__((aligned(64))) JITVar Vars[1<<20];
extern __attribute__((aligned(64))) JITFunc Funcs[1<<20];

extern u64 VarCount;
extern u64 VarEnd;
extern u64 FuncCount;
extern u64 FuncEnd;

void EmitRet(u8 **p);
void EmitIRet(u8 **p);
void EmitPush(u8 **p, Reg reg);
void EmitPop(u8 **p, Reg reg);

void EmitJmp(u8 **p, void *target, void *here);
void EmitCall(u8 **p, void *target, void *here);
void EmitCallReg(u8 **p, Reg reg);
void EmitJcc(u8 **p, Condition cond, void *target, void *here);

void EmitBinaryOp(u8 **p, OpType op, Reg src, Reg dst);
void EmitUnaryMuldv(u8 **p, u8 reg_ext, Reg src);
void EmitUnaryOp(u8 **p, u8 reg_ext, Reg dst);
void EmitSetcc(u8 **p, Condition cond, Reg dst);
void EmitMovRegImm64(u8 **p, Reg dst, u64 val);
void EmitMovRegReg(u8 **p, Reg dst, Reg src);
void EmitMovRegPtrReg(u8 **p, Reg dst, Reg base);
