#include "shell.h"
#include "CpuInfo.h"
#include "debug.h"
#include "gui.h"
#include "math.h"
#include "str.h"
#include "wm.h"
#include "ps2Keyboard.h"
#include "ps2Mouse.h"
#include "int.h"
#include "kmalloc.h"
#include "gop.h"
#include "acpi.h"
#include "RandomSomething.h"
#include "Notepad.h"
#include "kmalloc.h"
#include "SystemInfo.h"

#define SHELL_COLS 79
#define SHELL_ROWS 20
#define BUFFER_SIZE (SHELL_COLS * SHELL_ROWS * 2)

#define KEY_REPEAT_DELAY 60
#define KEY_REPEAT_INTERVAL 10

extern u64 FADTAddr;
extern u64 DSDTAddr;
extern AllocPool KernelPool;

static u64 KeyRepeatCounter = 0;
static u64 KeyRepeatPhase = 0;

extern AllocPool KernelPool;
extern WM KernelGlobalWindowManager;
extern u32 KernelEnglishFonts[255][19][8];

static u64 ShellCount = 0;
static char OldChar = 0;
static Bool KeyIsDown = 0;

static const char Hex[]="0123456789ABCDEF";

void GlobalShellExit(void* Self) {
    Window* WinSelf = Self;
    WMDelWindow(&KernelGlobalWindowManager, NULL, WinSelf->SomeID);
    MemSet(Self,0,sizeof(Window) + sizeof(TextBox) + (78 * 20) + sizeof(LineShell));
    Free(&KernelPool, WinSelf);
    ShellCount--;
}

static void ScrollText(TextBox* TextSelf) {
    if (!TextSelf || !TextSelf->Text) return;
    u64 total_lines = 0;
    u64 current_col = 0;
    
    for (u64 i = 0; i < TextSelf->Length; i++) {
        char c = TextSelf->Text[i];
        if (c == '_') continue;

        if (c == '\n') {
            total_lines++;
            current_col = 0;
        } else {
            current_col++;
            if (current_col >= SHELL_COLS) {
                total_lines++;
                current_col = 0;
            }
        }
    }
    if (current_col > 0) total_lines++;

    while (total_lines > 20) {
        u64 cut_pos = 0;
        u64 col = 0;

        while (cut_pos < TextSelf->Length) {
            char c = TextSelf->Text[cut_pos];
            if (c == '\n') {
                cut_pos++;
                break;
            }
            col++;
            cut_pos++;
            if (col >= SHELL_COLS) {
                break;
            }
        }

        if (cut_pos == 0 || cut_pos >= TextSelf->Length) break;

        u64 move_len = TextSelf->Length - cut_pos;
        MemCopy(TextSelf->Text, TextSelf->Text + cut_pos, move_len);
        TextSelf->Length -= cut_pos;
        MemSet(TextSelf->Text + TextSelf->Length, 0, cut_pos);

        total_lines--;
    }
}

static void ShellPrint(TextBox* TextSelf, char* String) {
    for (u64 i = 0; String[i] != 0; i++) {
        char c = String[i];

        if (c == '\n') {
            TextSelf->Text[TextSelf->Length++] = c;
            continue;
        }

        u64 CurrentCol = 0;
        for (s64 j = (s64)TextSelf->Length - 1; j >= 0; j--) {
            if (TextSelf->Text[j] == '\n') {
                break;
            }
            CurrentCol++;
        }

        if (CurrentCol >= SHELL_COLS) {
            TextSelf->Text[TextSelf->Length++] = '\n';
        }

        TextSelf->Text[TextSelf->Length++] = c;
    }

    ScrollText(TextSelf);
    DebugStr(String);
}

static void ShellPrintNum64(TextBox* TextSelf, u64 Num) {
    char buf[18];
    int pos = 0;
    for (u32 i = 0; i < 16; i++) {
        u64 shift = (15 - i) * 4;
        buf[pos++] = Hex[(Num >> shift) & 0xFULL];
    }
    buf[pos] = '\0';

    ShellPrint(TextSelf, buf);
    DebugU64(Num);
}

void ProcessCommand(TextBox* TextSelf, LineShell* Line, Window* WinSelf) {
    char* Cmd = Line->Text;
    u64 CmdLen = StrLen(Cmd);
    DebugChar('>');
    DebugStr(Cmd);
    DebugChar('\n');
    
    if (CmdLen == 0) {
        TextSelf->Text[TextSelf->Length] = '_';
        return;
    }
    
    TextSelf->Text[TextSelf->Length] = '\n';
    TextSelf->Length++;
    
    if (StrIsWithSize(Cmd, "Help", 5)) {
        ShellPrint(TextSelf, "Available commands:\n"); 
        ShellPrint(TextSelf, "\tHelp  - show this message\n");
        ShellPrint(TextSelf, "\tClear - clear screen\n");
        ShellPrint(TextSelf, "\tEcho XXX - Print(\"XXX\")\n");
        ShellPrint(TextSelf, "\tShutDown - shutdown\n");
        ShellPrint(TextSelf, "\tReBoot - reboot\n");
        ShellPrint(TextSelf, "\tVer - show version\n");
        ShellPrint(TextSelf, "\tCpuID - Just....cpuid\n");
        ShellPrint(TextSelf, "\tCpuInfo - Just....cpuid\n");
        ShellPrint(TextSelf, "\tSin xxxx - Sin(xxxx)\n");
        ShellPrint(TextSelf, "\tNotepad - Just....New Notepad\n");
        ShellPrint(TextSelf, "\tShell - Just....New Shell\n");
        ShellPrint(TextSelf, "\tExit - Close Shell Window\n");
    }
    else if (StrIsWithSize(Cmd, "Clear", 6)) {
        TextSelf->Length = 0;
        MemSet(TextSelf->Text, 0, BUFFER_SIZE);
        Line->X = 0;
        Line->XTmp = 0;
        MemSet(Line->Text, 0, SHELL_COLS);
        TextSelf->Text[TextSelf->Length] = '>';
        TextSelf->Length++;
        TextSelf->Text[TextSelf->Length] = '_';
        return;
    }
    else if (StrIsWithSize(Cmd, "Echo ", 5)) {
        ShellPrint(TextSelf, Cmd + 5);
        ShellPrint(TextSelf, "\n");
    }
    else if (StrIsWithSize(Cmd, "Echo", 5)) {
        ShellPrint(TextSelf, "\n");
    }
    else if (StrIsWithSize(Cmd, "ShutDown", 9)) {
        ShellPrint(TextSelf, "GoodBye! :D\n");
        ShutDown(FADTAddr, DSDTAddr);
        return;
    }
    else if (StrIsWithSize(Cmd, "ReBoot", 7)) {
        ShellPrint(TextSelf, "See You Next Time! :D\n");
        ReBoot(FADTAddr);
        return;
    }
    else if (StrIsWithSize(Cmd, "Ver", 4)) {
        ShellPrint(TextSelf, "Neuro-Sama OS (Pre-Alfa " BUILD_DATE ")\n");
        ShellPrint(TextSelf, "Producer: Andy / AndyFish / Fish / Dydudu37d (All Is Me)\n");
        ShellPrint(TextSelf, "License: MIT License\n");
        ShellPrint(TextSelf, "Nickname: Fish\n");
        ShellPrint(TextSelf, "You Should Download It From An FTP Server, \nGoogle Drive, Or A Chinese Cloud Storage Service. \nIf Not, Be Wary Of Potential Piracy.\n");
        ShellPrint(TextSelf, "Have Fun!\n");
    }
    else if (StrIsWithSize(Cmd, "CpuID", 6) || StrIsWithSize(Cmd, "CpuInfo", 8)) {
        ShellPrint(TextSelf, "Neuro-Sama OS (Pre-Alfa " BUILD_DATE ")\n");
        
        CpuInfoResult vendor = CpuInfo(0, 0);
        char vendor_name[13];
        ((u32*)vendor_name)[0] = vendor.Ebx;
        ((u32*)vendor_name)[1] = vendor.Edx;
        ((u32*)vendor_name)[2] = vendor.Ecx;
        vendor_name[12] = '\0';
        ShellPrint(TextSelf, "Vendor: ");
        ShellPrint(TextSelf, vendor_name);
        ShellPrint(TextSelf, "\n");
        
        ShellPrint(TextSelf, "Max Standard Leaf: 0x");
        ShellPrintNum64(TextSelf, vendor.Eax);
        ShellPrint(TextSelf, "\n");
        
        CpuInfoResult features = CpuInfo(1, 0);
        ShellPrint(TextSelf, "Family: 0x");
        ShellPrintNum64(TextSelf, (features.Eax >> 8) & 0xF);
        ShellPrint(TextSelf, ", Model: 0x");
        ShellPrintNum64(TextSelf, (features.Eax >> 4) & 0xF);
        ShellPrint(TextSelf, ", Stepping: 0x");
        ShellPrintNum64(TextSelf, features.Eax & 0xF);
        ShellPrint(TextSelf, "\n");
        
        ShellPrint(TextSelf, "SSE3: ");
        ShellPrintNum64(TextSelf, (features.Ecx >> 0) & 1);
        ShellPrint(TextSelf, ", AVX: ");
        ShellPrintNum64(TextSelf, (features.Ecx >> 28) & 1);
        ShellPrint(TextSelf, "\n");
        
        if (vendor.Eax >= 0x07) {
            CpuInfoResult leaf7 = CpuInfo(7, 0);
            ShellPrint(TextSelf, "AVX2: ");
            ShellPrintNum64(TextSelf, (leaf7.Ebx >> 5) & 1);
            ShellPrint(TextSelf, "\n");
        } else {
            ShellPrint(TextSelf, "AVX2: Not Supported (Leaf 7 not available)\n");
        }
        
        CpuInfoResult ext = CpuInfo(0x80000000, 0);
        ShellPrint(TextSelf, "Max Extended Leaf: 0x");
        ShellPrintNum64(TextSelf, ext.Eax);
        ShellPrint(TextSelf, "\n");
        
        if (ext.Eax >= 0x80000001) {
            CpuInfoResult longmode = CpuInfo(0x80000001, 0);
            ShellPrint(TextSelf, "Long Mode: ");
            ShellPrintNum64(TextSelf, (longmode.Edx >> 29) & 1);
            ShellPrint(TextSelf, "\n");
        }

        ShellPrint(TextSelf, "Mode ID: ");
        char CpuMode[49];
        MemSet(CpuMode,0,40);
        for (u32 Leaf=0x80000002;Leaf<0x80000005;Leaf++){
            CpuInfoResult ModeID = CpuInfo(Leaf, 0);
            char* ModeIDList=(char*)&ModeID;
            MemCopy(CpuMode+(16*((Leaf&0x07UL)-0x2)),ModeIDList,16);
        }
        CpuMode[48]='\0';
        ShellPrint(TextSelf, CpuMode);
        ShellPrint(TextSelf, "\n");
    }
    else if (StrIsWithSize(Cmd, "Sin ", 4) && CmdLen>4){
        char Buffer[256];
        MemSet(Buffer,0,256);
        F64ToStr(DoubleSin(StrToF64(Cmd+4)), Buffer);
        ShellPrint(TextSelf,Buffer);
        ShellPrint(TextSelf, "\n");
    }
    else if (StrIsWithSize(Cmd, "Notepad", 8)){
        NewNotepadWindow();
    }
    else if (StrIsWithSize(Cmd, "Shell", 6)){
        NewShellWindow();
    }
    else if (StrIsWithSize(Cmd, "SystemInfo", 11)){
        NewSystemInfoWindow();
    }
    else if (StrIsWithSize(Cmd, "Exit", 5)) {
        ShellPrint(TextSelf, "73\n");
        WinSelf->Base.Exit(WinSelf);
        return;
    }
    else {
        ShellPrint(TextSelf, "Unknown command: ");
        ShellPrint(TextSelf, Cmd);
        ShellPrint(TextSelf, "\n");
    }
    
    Line->X = 0;
    Line->XTmp = 0;
    MemSet(Line->Text, 0, SHELL_COLS);
    TextSelf->Text[TextSelf->Length] = '>';
    TextSelf->Length++;
    TextSelf->Text[TextSelf->Length] = '_';
    ScrollText(TextSelf);
}

static void HandleKeyInput(TextBox* TextSelf, LineShell* Line, char c, Window* WinSelf) {
    if (c == '\b' && Line->X > 0) {
        Line->X--;
        Line->XTmp--;
        TextSelf->Text[TextSelf->Length] = 0;
        Line->Text[Line->X] = 0;
        TextSelf->Length--;
        TextSelf->Text[TextSelf->Length] = '_';
    }
    else if (c == '\n') {
        Line->Text[Line->X] = '\0';

        if (Line->X > 0) {
            ProcessCommand(TextSelf, Line, WinSelf);
        } else {
            TextSelf->Text[TextSelf->Length] = '\n';
            TextSelf->Length++;
            TextSelf->Text[TextSelf->Length] = '>';
            TextSelf->Length++;
            TextSelf->Text[TextSelf->Length] = '_';
            ScrollText(TextSelf);
        }
    }
    else if (c != '\b') {
        if ((Line->XTmp + 1) >= SHELL_COLS) {
            Line->XTmp=0;
            TextSelf->Text[TextSelf->Length] = '\n';
            TextSelf->Length++;
        }

        TextSelf->Text[TextSelf->Length] = c;
        Line->Text[Line->X] = c;
        TextSelf->Length++;
        Line->X++;
        Line->XTmp++;
        TextSelf->Text[TextSelf->Length] = '_';
    }
}

void GlobalShellPoll(void* Self) {
    Window* WinSelf = Self;
    TextBox* TextSelf = WinSelf->Objs[0];
    LineShell* Line = (LineShell*)(((u64)TextSelf) + sizeof(TextBox) + (78 * 20));
    u32 MouseX = GlobalPS2Mouse.X;
    u32 MouseY = GlobalPS2Mouse.Y;
    u32 WinX = (u32)WinSelf->Base.x;
    u32 WinY = (u32)WinSelf->Base.y;
    u32 WinW = (u32)WinSelf->Base.w;
    u32 TitleBarHeight = 20;

    Bool InTitleBar = (
        MouseX >= WinX && MouseX < WinX + WinW &&
        MouseY >= WinY && MouseY < WinY + TitleBarHeight &&
        ((KernelGlobalWindowManager.Tail == WinSelf) ||
        (WMWindowsTitleOverlapping(&KernelGlobalWindowManager, WinSelf, TitleBarHeight, MouseX-WinX, MouseY-WinY) == NULL))
    );

    if (GlobalPS2Mouse.Code.ButtonLeft && InTitleBar && !WinSelf->IsDragging) {
        WMUpWindow(&KernelGlobalWindowManager, NULL, WinSelf->SomeID);
        WinSelf->IsDragging = 1;
        WinSelf->DragOffsetX = GlobalPS2Mouse.X - (u32)WinSelf->Base.x;
        WinSelf->DragOffsetY = GlobalPS2Mouse.Y - (u32)WinSelf->Base.y;
    }

    if (WinSelf->IsDragging && GlobalPS2Mouse.Code.ButtonLeft) {
        s32 NewX = (s32)MouseX - (s32)WinSelf->DragOffsetX;
        s32 NewY = (s32)MouseY - (s32)WinSelf->DragOffsetY;
        
        if (NewX < 0) NewX = 0;
        if (NewY < 0) NewY = 0;
        
        WinSelf->Base.x = (u32)NewX;
        WinSelf->Base.y = (u32)NewY;
    }

    if (!GlobalPS2Mouse.Code.ButtonLeft) {
        WinSelf->IsDragging = 0;
    }

    if (KernelGlobalWindowManager.Tail!=WinSelf) return;

    if (TopF1To12Key == 7) {
        ShellPrint(TextSelf, RandomString());
        ShellPrint(TextSelf, " ");
    }else if (TopF1To12Key == 6){
        ShellPrint(TextSelf, RandomQuestions());
        ShellPrint(TextSelf, "\n");
    }

    if (Get >= 0x80) {
        KeyIsDown = 0;
        OldChar = 0;
        KeyRepeatCounter = 0;
        KeyRepeatPhase = 0;
    }

    char CurrentChar = Char;

    if (CurrentChar != 0) {
        KeyIsDown = 1;
        OldChar = CurrentChar;
        KeyRepeatCounter = 0;
        KeyRepeatPhase = 0;

        HandleKeyInput(TextSelf, Line, CurrentChar, WinSelf);
    } 
    else if (KeyIsDown && OldChar != 0) {
        KeyRepeatCounter++;
        u64 Threshold = (KeyRepeatPhase == 0) ? KEY_REPEAT_DELAY : KEY_REPEAT_INTERVAL;

        if (KeyRepeatCounter >= Threshold) {
            KeyRepeatCounter = 0;
            KeyRepeatPhase = 1;
            HandleKeyInput(TextSelf, Line, OldChar, WinSelf);
        }
    }
}

void GlobalShellDraw(void* Self) {
    Window* WinSelf = Self;
    TextBox* TextSelf = WinSelf->Objs[0];
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
        KernelEnglishFonts
    );
    GOPRectFill(
        (u32[2]){(u32)WinSelf->Base.x, (u32)WinSelf->Base.x + (u32)WinSelf->Base.w},
        (u32[2]){(u32)WinSelf->Base.y + 20, (u32)WinSelf->Base.y + (u32)WinSelf->Base.h},
        PACK_COLOR(0xFF, 0xFF, 0xFF, 0xFF)
    );
    GOPStringWH(
        (u32)WinSelf->Base.x + (u32)TextSelf->Base.x,
        (u32)WinSelf->Base.y + 20 + (u32)TextSelf->Base.y,
        (u32)TextSelf->Base.w,
        (u32)TextSelf->Base.h,
        TextSelf->Text,
        PACK_COLOR(0xFF, 0xFF, 0xFF, 0xFF),
        PACK_COLOR(0x00, 0x00, 0x00, 0xFF),
        KernelEnglishFonts
    );
}

void NewShellWindow(void) {
    Window* NewShell = Alloc(&KernelPool, sizeof(Window) + sizeof(TextBox) + (78 * 20) + sizeof(LineShell));
    if (!NewShell) return;
    TextBox* TextSelf = (TextBox*)((u64)NewShell + sizeof(Window));
    NewShell->SomeID = (0x49484C4C45485300ULL+ShellCount);
    NewShell->Base.Exit = GlobalShellExit;
    NewShell->Base.Visible = 1;
    NewShell->Base.x = 100;
    NewShell->Base.y = 100;
    NewShell->Base.w = 8 * 80;
    NewShell->Base.h = 19 * 22;
    NewShell->Base.Type = 0;
    NewShell->Base.Poll = GlobalShellPoll;
    NewShell->Base.Draw = GlobalShellDraw;
    NewShell->Hide = 0;
    NewShell->Title = "Shell";
    TextSelf->Base.Visible = 1;
    TextSelf->Base.x = 0;
    TextSelf->Base.y = 0;
    TextSelf->Base.w = 8 * 79;
    TextSelf->Base.h = 19 * 20;
    TextSelf->Base.Type = 1;
    TextSelf->Base.Draw = NULL;
    TextSelf->Text = (char*)((u64)TextSelf + sizeof(TextBox));
    ShellPrint(TextSelf, "CQ CQ CQ\n");
    TextSelf->Text[TextSelf->Length] = '>';
    TextSelf->Length++;
    TextSelf->Text[TextSelf->Length] = '_';
    
    NewShell->Objs[0] = TextSelf;
    NewShell->Count = 1;
    WMAddWindow(&KernelGlobalWindowManager, NewShell);
    ShellCount++;
}