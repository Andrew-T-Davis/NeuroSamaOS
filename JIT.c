#include "JIT.h"
#include "int.h"

__attribute__((aligned(64))) JITVar Vars[1<<20]={0};
u64 VarCount=0;
u64 VarEnd=0;

__attribute__((aligned(64))) JITFunc Funcs[1<<20]={0};
u64 FuncCount=0;
u64 FuncEnd=0;

u64 GetVarCount(){
    return VarCount;
}

JITVar GetVar(u64 SerialNumber){
    for (u64 Idx=0; Idx<VarEnd; Idx++){
        if (Vars[Idx].SerialNumber == SerialNumber) return Vars[Idx];
    }
    JITVar A;
    A.DataPoint=0;
    A.SerialNumber=0;
    A.Type=0;
    A.Size=0;
    return A;
}

void SetVarDataPoint(u64 SerialNumber, u64* DataPoint){
    for (u64 Idx=0; Idx<VarEnd; Idx++){
        if (Vars[Idx].SerialNumber == SerialNumber) {
            Vars[Idx].DataPoint = DataPoint;
        }
    }
}

void* GetVarDataPoint(u64 SerialNumber){
    for (u64 Idx=0; Idx<VarEnd; Idx++){
        if (Vars[Idx].SerialNumber == SerialNumber) {
            return Vars[Idx].DataPoint;
        }
    }
    return 0;
}

u16 GetVarType(u64 SerialNumber){
    for (u64 Idx=0; Idx<VarEnd; Idx++){
        if (Vars[Idx].SerialNumber == SerialNumber) {
            return Vars[Idx].Type;
        }
    }
    return 0;
}

void SetVarType(u64 SerialNumber, u16 Type){
    for (u64 Idx=0; Idx<VarEnd; Idx++){
        if (Vars[Idx].SerialNumber == SerialNumber) {
            Vars[Idx].Type = Type;
        }
    }
}

u64 VarAdd(JITVar Var){
    if (!Var.Type) return (u64)-1;
    for (u64 Idx=0; Idx < (1<<20); Idx++){
        if (!Vars[Idx].Type) {
            Vars[Idx].SerialNumber = Idx;
            Vars[Idx].Type = Var.Type;
            Vars[Idx].DataPoint = Var.DataPoint;
            if (Idx >= VarEnd) VarEnd = Idx + 1;
            VarCount++;
            return Idx;
        }
    }
    return (u64)-1;
}

u64 VarFoundEnd(){
    for (s64 Idx=(1<<20)-1; Idx>=0; Idx--){
        if (Vars[Idx].Type) return (u64)(Idx + 1);
    }
    return 0;
}

void VarDel(u64 SerialNumber){
    for (u64 Idx=0; Idx<VarEnd; Idx++){
        if (Vars[Idx].SerialNumber == SerialNumber) {
            if (Vars[Idx].Type) VarCount--;
            Vars[Idx].Type = 0;
            Vars[Idx].SerialNumber = 0;
            if (Idx == VarEnd - 1) VarEnd = VarFoundEnd();
            return;
        }
    }
}

u64 GetFuncCount(){
    return FuncCount;
}

JITFunc GetFunc(u64 SerialNumber){
    for (u64 Idx=0; Idx<FuncEnd; Idx++){
        if (Funcs[Idx].SerialNumber == SerialNumber) return Funcs[Idx];
    }
    JITFunc A;
    A.FuncPoint=0;
    A.SerialNumber=0;
    A.Type=0;
    return A;
}

void SetFuncPoint(u64 SerialNumber, u64* FuncPoint){
    for (u64 Idx=0; Idx<FuncEnd; Idx++){
        if (Funcs[Idx].SerialNumber == SerialNumber) {
            Funcs[Idx].FuncPoint = FuncPoint;
        }
    }
}

void* GetFuncPoint(u64 SerialNumber){
    for (u64 Idx=0; Idx<FuncEnd; Idx++){
        if (Funcs[Idx].SerialNumber == SerialNumber) {
            return Funcs[Idx].FuncPoint;
        }
    }
    return 0;
}

u16 GetFuncType(u64 SerialNumber){
    for (u64 Idx=0; Idx<FuncEnd; Idx++){
        if (Funcs[Idx].SerialNumber == SerialNumber) {
            return Funcs[Idx].Type;
        }
    }
    return 0;
}

void SetFuncType(u64 SerialNumber, u16 Type){
    for (u64 Idx=0; Idx<FuncEnd; Idx++){
        if (Funcs[Idx].SerialNumber == SerialNumber) {
            Funcs[Idx].Type = Type;
        }
    }
}

u64 FuncAdd(JITFunc Func){
    if (!Func.Type) return (u64)-1;
    for (u64 Idx=0; Idx < (1<<20); Idx++){
        if (!Funcs[Idx].Type) {
            Funcs[Idx].SerialNumber = Idx;
            Funcs[Idx].Type = Func.Type;
            Funcs[Idx].FuncPoint = Func.FuncPoint;
            if (Idx >= FuncEnd) FuncEnd = Idx + 1;
            FuncCount++;
            return Idx;
        }
    }
    return (u64)-1;
}

u64 FuncFoundEnd(){
    for (s64 Idx=(1<<20)-1; Idx>=0; Idx--){
        if (Funcs[Idx].Type) return (u64)(Idx + 1);
    }
    return 0;
}

void FuncDel(u64 SerialNumber){
    for (u64 Idx=0; Idx<FuncEnd; Idx++){
        if (Funcs[Idx].SerialNumber == SerialNumber) {
            if (Funcs[Idx].Type) FuncCount--;
            Funcs[Idx].Type = 0;
            Funcs[Idx].SerialNumber = 0;
            if (Idx == FuncEnd - 1) FuncEnd = FuncFoundEnd();
            return;
        }
    }
}

void EmitPush(u8 **p, Reg reg) {
    if (reg >= 8) *(*p)++ = 0x41;
    *(*p)++ = 0x50 | (reg & 7);
}

void EmitPop(u8 **p, Reg reg) {
    if (reg >= 8) *(*p)++ = 0x41;
    *(*p)++ = 0x58 | (reg & 7);
}

void EmitRet(u8 **p) {
    *(*p)++ = 0xC3;
}

void EmitCallReg(u8 **p, Reg reg) {
    u8 rex = 0x40 | ((reg >= 8) ? 1 : 0);
    if (rex > 0x40) *(*p)++ = rex;
    *(*p)++ = 0xFF;
    *(*p)++ = 0xD0 | (reg & 7);
}

void EmitJmp(u8 **p, void *target, void *here) {
    *(*p)++ = 0xE9;
    s64 offset = (s64)target - ((s64)here + 5);
    *(s32 *)(*p) = (s32)offset;
    *p += 4;
}

void EmitCall(u8 **p, void *target, void *here) {
    *(*p)++ = 0xE8;
    s64 offset = (s64)target - ((s64)here + 5);
    *(s32 *)(*p) = (s32)offset;
    *p += 4;
}

void EmitJcc(u8 **p, Condition cond, void *target, void *here) {
    *(*p)++ = 0x0F;
    *(*p)++ = 0x80 | (u8)cond;
    s64 offset = (s64)target - ((s64)here + 6);
    *(s32 *)(*p) = (s32)offset;
    *p += 4;
}

void EmitBinaryOp(u8 **p, OpType op, Reg src, Reg dst) {
    u8 rex = (u8)(0x48 | (((src >= 8) ? 1 : 0) << 2) | ((dst >= 8) ? 1 : 0));
    *(*p)++ = rex;
    *(*p)++ = (u8)op;
    *(*p)++ = (u8)(0xC0 | ((src & 7) << 3) | (dst & 7));
}

void EmitUnaryMuldv(u8 **p, u8 reg_ext, Reg src) {
    u8 rex = 0x48 | ((src >= 8) ? 1 : 0);
    *(*p)++ = rex;
    *(*p)++ = 0xF7;
    *(*p)++ = (u8)(0xC0U | (u8)(reg_ext << 3) | (src & 7));
}

void EmitMovRegImm64(u8 **p, Reg dst, u64 val) {
    u8 rex = (u8)(0x48 | ((dst >= 8) ? 1 : 0));
    *(*p)++ = rex;
    *(*p)++ = (u8)(0xB8 | (dst & 7));
    *(u64*)(*p) = val;
    *p += 8;
}

void EmitMovRegReg(u8 **p, Reg dst, Reg src) {
    u8 rex = (u8)(0x48 | (((src >= 8) ? 1 : 0) << 2) | ((dst >= 8) ? 1 : 0));
    *(*p)++ = rex;
    *(*p)++ = 0x89;
    *(*p)++ = (u8)(0xC0 | ((src & 7) << 3) | (dst & 7));
}

void EmitMovRegPtrReg(u8 **p, Reg dst, Reg base) {
    u8 rex = (u8)(0x48 | (((dst >= 8) ? 1 : 0) << 2) | ((base >= 8) ? 1 : 0));
    *(*p)++ = rex;
    *(*p)++ = 0x8B;
    *(*p)++ = (u8)(0x00 | ((dst & 7) << 3) | (base & 7));
}

void EmitSetcc(u8 **p, Condition cond, Reg dst) {
    *(*p)++ = 0x0F;
    *(*p)++ = (u8)(0x90 | (u8)cond);
    *(*p)++ = 0xC0;
    u8 rex = (u8)(0x48 | (((dst >= 8) ? 1 : 0) << 2));
    *(*p)++ = rex;
    *(*p)++ = 0x0F;
    *(*p)++ = 0xB6;
    *(*p)++ = (u8)(0xC0 | ((dst & 7) << 3) | 0);
}

void EmitIRet(u8 **p){
    *(*p)++ = 0x48;
    *(*p)++ = 0xCF;
}