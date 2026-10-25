#include "SystemInfo.h"
#include "gui.h"
#include "str.h"
#include "wm.h"
#include "ps2Mouse.h"
#include "int.h"
#include "kmalloc.h"
#include "gop.h"

extern AllocPool KernelPool;
extern WM KernelGlobalWindowManager;
extern u16 KernelFonts[65536][19];
static u64 SystemInfoCount = 0;

void GlobalSystemInfoExit(void* Self) {
    Window* WinSelf = Self;
    WMDelWindow(&KernelGlobalWindowManager, NULL, WinSelf->SomeID);
    MemSet(Self,0,sizeof(Window));
    Free(&KernelPool, WinSelf);
    SystemInfoCount--;
}

void GlobalSystemInfoPoll(void* Self) {
    Window* WinSelf = Self;
    u32 MouseX = GlobalPS2Mouse.X;
    u32 MouseY = GlobalPS2Mouse.Y;
    u32 WinX = (u32)WinSelf->Base.x;
    u32 WinY = (u32)WinSelf->Base.y;
    u32 WinW = (u32)WinSelf->Base.w;
    u32 TitleBarHeight = 20;

    Bool InTitleBar = (
        MouseX >= WinX && MouseX < WinX + WinW &&
        MouseY >= WinY && MouseY < WinY + TitleBarHeight
    );

    if (GlobalPS2Mouse.Code.ButtonLeft && InTitleBar && !WinSelf->IsDragging) {
        WMUpWindow(&KernelGlobalWindowManager, NULL, WinSelf->SomeID);
        WinSelf->IsDragging = 1;
        WinSelf->DragOffsetX = GlobalPS2Mouse.X - (u32)WinSelf->Base.x;
        WinSelf->DragOffsetY = GlobalPS2Mouse.Y - (u32)WinSelf->Base.y;
    }

    if (!GlobalPS2Mouse.Code.ButtonLeft) {
        WinSelf->IsDragging = 0;
    }

    if (KernelGlobalWindowManager.Tail!=WinSelf) return;

        if (WinSelf->IsDragging && GlobalPS2Mouse.Code.ButtonLeft) {
        s32 NewX = (s32)MouseX - (s32)WinSelf->DragOffsetX;
        s32 NewY = (s32)MouseY - (s32)WinSelf->DragOffsetY;
        
        if (NewX < 0) NewX = 0;
        if (NewY < 0) NewY = 0;
        
        WinSelf->Base.x = (u32)NewX;
        WinSelf->Base.y = (u32)NewY;
    }
}

void GlobalSystemInfoDraw(void* Self) {
    Window* WinSelf = Self;
    GOPRectFill(
        (u32[2]){(u32)WinSelf->Base.x, (u32)WinSelf->Base.x + (u32)WinSelf->Base.w},
        (u32[2]){(u32)WinSelf->Base.y, (u32)WinSelf->Base.y + 20},
        PACK_COLOR(0x00, 0xAA, 0xAA, 0xFF)
    );
    GOPStringWH(
        (u32)WinSelf->Base.x + 4,
        (u32)WinSelf->Base.y + 2,
        (u32)WinSelf->Base.w,
        (u32)WinSelf->Base.h,
        WinSelf->Title ? WinSelf->Title : "Window",
        PACK_COLOR(0x00, 0x00, 0x00, 0x00),
        PACK_COLOR(0xFF, 0xFF, 0xFF, 0xFF),
        KernelFonts,
        0
    );
    GOPRectFill(
        (u32[2]){(u32)WinSelf->Base.x, (u32)WinSelf->Base.x + (u32)WinSelf->Base.w},
        (u32[2]){(u32)WinSelf->Base.y + 20, (u32)WinSelf->Base.y + (u32)WinSelf->Base.h},
        PACK_COLOR(0xFF, 0xFF, 0xFF, 0xFF)
    );
    GOPStringWH(
        (u32)WinSelf->Base.x + 0,
        (u32)WinSelf->Base.y + 20 + 0,
        (u32)WinSelf->Base.w,
        (u32)WinSelf->Base.h,
        "System info:\n"
                    "\tVersion & Build data: " BUILD_DATE "\n"
                    "\tRepo *.c count: " "43" "\n"
                    "\tRepo *.h count: " "53" "\n"
                    "\tRepo *.s count: " "02" "\n"
        ,
        PACK_COLOR(0xFF, 0xFF, 0xFF, 0xFF),
        PACK_COLOR(0x00, 0x00, 0x00, 0xFF),
        KernelFonts,
        0
    );
}

void NewSystemInfoWindow(void){
    Window* NewSystemInfo = Alloc(&KernelPool, sizeof(Window));
    if (!NewSystemInfo) return;
    NewSystemInfo->SomeID = (0x535953494E464F00ULL+SystemInfoCount);
    NewSystemInfo->Base.Exit = GlobalSystemInfoExit;
    NewSystemInfo->Base.Visible = 1;
    NewSystemInfo->Base.x = 100;
    NewSystemInfo->Base.y = 100;
    NewSystemInfo->Base.w = 8 * 80;
    NewSystemInfo->Base.h = 19 * 22;
    NewSystemInfo->Base.Type = 0;
    NewSystemInfo->Base.Poll = GlobalSystemInfoPoll;
    NewSystemInfo->Base.Draw = GlobalSystemInfoDraw;
    NewSystemInfo->Hide = 0;
    NewSystemInfo->Title = "SystemInfo";
    WMAddWindow(&KernelGlobalWindowManager, NewSystemInfo);
    SystemInfoCount++;
}