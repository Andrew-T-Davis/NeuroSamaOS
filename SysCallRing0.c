#include "SysCallRing0.h"
#include "clock.h"
#include "debug.h"
#include "int.h"

#define IA32_STAR 0xC0000081
#define IA32_LSTAR 0xC0000082
#define IA32_FMASK 0xC0000084

u64 (*SysCallFuncTo[2][2])(u64 Args,u64 ArgCount)={};

void SysCallHandler(){
    u64 Func=0;
    u64 SubFunc=0;
    u64 Args=0;
    u64 ArgCount=0;
    __asm__ volatile (
        "movq %%rax, %0 \n\t"
        "movq %%rbx, %1 \n\t"
        "movq %%rdx, %2 \n\t"
        "movq %%rsi, %3 \n\t"
        : "=m"(Func),"=m"(SubFunc),"=m"(Args),"=m"(ArgCount)
        : 
        : "rax", "rbx", "rdx", "rsi", "memory"
    );
    
    DebugStr("[SYSCALL] Enter SysCallHandler:\n"
                "\tFunc=");DebugU64(Func);DebugStr(", \n"
                "\tSubFunc=");DebugU64(SubFunc);DebugStr(", \n"
                "\tArgs=");DebugU64(Args);DebugStr(", \n"
                "\tArgCount=");DebugU64(ArgCount);DebugStr("\n"
    );

    if (Func>=2 || SubFunc>=2) {
        DebugStr("[SYSCALL] Error: (Func>=2 || SubFunc>=2)==1\n");
        DebugStr("[SYSCALL] Exit, Rax=0x8000000000000000\n");
        __asm__ volatile ("movq $0x8000000000000000, %%rax \n\t");
        return;
    }
    register u64 Return __asm__("rax")=SysCallFuncTo[Func][SubFunc](Args,ArgCount);
    __asm__ volatile (
        "movq %0, %%rax\n\t"
        :
        : "x"(Return)
        : "rax", "memory"
    );
    return;
}

__attribute__((naked)) void SysCallHandlerEntry(){
    __asm__ volatile(
        "pushq %%rcx \n\t"
        "pushq %%r11 \n\t"
        "subq $0x20, %%rsp \n\t"
        "call SysCallHandler \n\t"
        "addq $0x20, %%rsp \n\t"
        "popq %%r11 \n\t"
        "ret \n\t"
    );
}

void InitSysCallRing0(){
    wrmsr(IA32_LSTAR, (u64)&SysCallHandlerEntry);
    wrmsr(IA32_STAR, 0x0008000000000000ULL);
    wrmsr(IA32_FMASK, 0x0000000000000000ULL);
}

u64 SysCall(u64 Func, u64 SubFunc, void *Args, u64 ArgCount){
    register u64 Rax __asm__("rax") = Func;
    register u64 Rbx __asm__("rbx") = SubFunc;
    register u64 Rcx __asm__("rdx") = (u64)Args;
    register u64 Rdx __asm__("rsi") = ArgCount;
    u64 Return=0;
    __asm__ volatile (
        "syscall\n\t"
        "movq %%rax, %0\n\t"
        : "=m"(Return)
        : "x"(Rax),"x"(Rbx),"x"(Rcx),"x"(Rdx)
        : "rax", "rbx", "rdx", "rsi", "memory"
    );
    return Return;
}