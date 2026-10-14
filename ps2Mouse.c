#include "ps2Mouse.h"
#include "event.h"
#include "int.h"
#include "port.h"
#include "str.h"
#include "ioapic.h"
#include "efi.h"

static u8 Time=0;
static PS2MouseStatus GlobalMouseStatus={.Code={0},.XAxisMovementValue=0,.YAxisMovementValue=0};
PS2Mouse GlobalPS2Mouse={.Code={0},.X=640,.Y=480};
PS2Mouse GlobalOldPS2Mouse={.Code={0},.X=640,.Y=480};
extern EFI_GOP_MODE_INFO GopInfo;

void PS2MouseIntFunc() {
    u8 Status = inb(0x64);
    if (!(Status & 0x20)) {
        LocalApicEOI();
        return;
    }

    u8 Get = inb(0x60);
    if (Time == 0) {
        GlobalMouseStatus.Code.U = Get;
        if ((GlobalMouseStatus.Code.AlwaysOne) == 0) {
            Time = 0;
            LocalApicEOI();
            return;
        }
        GlobalPS2Mouse.Code.U = GlobalMouseStatus.Code.U;
        Time = 1;
    } else if (Time == 1) {
        GlobalMouseStatus.XAxisMovementValue = Get;
        Time = 2;
    } else if (Time == 2) {
        MemCopy(&GlobalOldPS2Mouse,&GlobalPS2Mouse,sizeof(PS2Mouse));
        GlobalMouseStatus.YAxisMovementValue = Get;
        
        s32 dx = (s32)GlobalMouseStatus.XAxisMovementValue;
        s32 dy = (s32)GlobalMouseStatus.YAxisMovementValue;
        
        if (GlobalMouseStatus.Code.XAxisSignBit) 
            dx |= 0xFFFFFF00;
            
        if (GlobalMouseStatus.Code.YAxisSignBit) 
            dy |= 0xFFFFFF00;
        
        s32 NewX = (s32)GlobalPS2Mouse.X + dx;
        s32 NewY = (s32)GlobalPS2Mouse.Y - dy;
        
        if (NewX < 0) NewX = 0;
        else if (NewX >= (s32)GopInfo.HorizontalResolution) 
            NewX = (s32)GopInfo.HorizontalResolution - 1;
            
        if (NewY < 0) NewY = 0;
        else if (NewY >= (s32)GopInfo.VerticalResolution) 
            NewY = (s32)GopInfo.VerticalResolution - 1;
        
        GlobalPS2Mouse.X = (u32)NewX;
        GlobalPS2Mouse.Y = (u32)NewY;
        Time = 0;
    }
    MakeEvent();

    LocalApicEOI();
}

__attribute__((naked)) void PS2MouseInt(){
    __asm__ volatile(
        "pushq %%rax\n\t" "pushq %%rcx\n\t" "pushq %%rdx\n\t" "pushq %%rbx\n\t"
        "pushq %%rbp\n\t" "pushq %%rsi\n\t" "pushq %%rdi\n\t"
        "pushq %%r8\n\t"  "pushq %%r9\n\t"  "pushq %%r10\n\t" "pushq %%r11\n\t"
        "pushq %%r12\n\t" "pushq %%r13\n\t" "pushq %%r14\n\t" "pushq %%r15\n\t"
        "call PS2MouseIntFunc\n\t"
        "popq %%r15\n\t" "popq %%r14\n\t" "popq %%r13\n\t" "popq %%r12\n\t"
        "popq %%r11\n\t" "popq %%r10\n\t" "popq %%r9\n\t"  "popq %%r8\n\t"
        "popq %%rdi\n\t" "popq %%rsi\n\t" "popq %%rbp\n\t" "popq %%rbx\n\t"
        "popq %%rdx\n\t" "popq %%rcx\n\t" "popq %%rax\n\t"
        "iretq\n\t"
        ::: "memory"
    );
}

PS2MouseStatus GetPS2MouseStatus(){
    PS2MouseStatus Tmp;
    MemCopy(&Tmp, &GlobalMouseStatus, sizeof(PS2MouseStatus));
    MemSet(&GlobalMouseStatus,0,sizeof(PS2MouseStatus));
    return Tmp;
}

PS2Mouse GetPS2Mouse(){
    return GlobalPS2Mouse;
}

void SetPS2Mouse(PS2Mouse PS2MouseArg){
    MemCopy(&GlobalPS2Mouse,&PS2MouseArg,sizeof(PS2Mouse));
}