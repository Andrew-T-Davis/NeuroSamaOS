#include "debug.h"
#include "gui.h"
#include "str.h"
#include "task.h"
#include "wm.h"
#include "ps2Keyboard.h"
#include "ps2Mouse.h"
#include "int.h"
#include "kmalloc.h"
#include "gop.h"
#include "RandomSomething.h"
#include "Notepad.h"
#include "clock.h"
#include "HolyNeuro.h"

typedef struct {
    u32 CursorX; 
    u32 CursorY;
    u32 ContentMaxX; 
    u32 ContentMaxY;

    u32 XTmp, YTmp;
    char Buffer[1 << 20];
    u32 CursorBlinkTimer;
    Bool CursorVisible;
    u32 ScrollOffset;
    u32 TotalLines;
    Bool IsCommandMode;
    
    u32 SelectionStartX, SelectionStartY;
    u32 SelectionEndX, SelectionEndY;
    Bool HasSelection;
} NotepadText;

typedef struct {
    u32 X;
    char Buffer[79 * 20];
} NotepadShell;

static char ClipboardBuffer[1 << 16] = {0};
static u32 ClipboardLen = 0;
static u16 JITTaskId = 0;

#define SHELL_COLS 79
#define SHELL_ROWS 20
#define BUFFER_SIZE (SHELL_COLS * SHELL_ROWS * 2)

#define KEY_REPEAT_DELAY 60
#define KEY_REPEAT_INTERVAL 10

extern u64 FADTAddr;
extern u64 DSDTAddr;

extern AllocPool KernelPool;
extern WM KernelGlobalWindowManager;
extern u16 KernelFonts[65536][19];

static u64 NotepadCount = 0;

static u64 entry = 0;

void GlobalNotepadExit(void* Self) {
    Window* WinSelf = Self;
    WMDelWindow(&KernelGlobalWindowManager, NULL, WinSelf->SomeID);
    Free(&KernelPool, WinSelf);
}

static u32 GetRealBufferLength(NotepadText* TextSelf) {
    u32 len = 0;
    while (len < (1 << 20) && TextSelf->Buffer[len] != '\0') {
        len++;
    }
    return len;
}

static u32 GetLineLength(NotepadText* TextSelf, u32 TargetY) {
    u32 cur_y = 0;
    u32 cur_x = 0;
    u32 idx = 0;
    u32 line_len = 0;
    u32 len = GetRealBufferLength(TextSelf);

    while (idx < len) {
        if (cur_y == TargetY) {
            if (TextSelf->Buffer[idx] == '\n') {
                break;
            }
            line_len++;
        }
        
        if (TextSelf->Buffer[idx] == '\n') {
            cur_y++;
            cur_x = 0;
        } else {
            cur_x++;
            if (cur_x >= 76) {
                if (cur_y == TargetY) break;
                cur_y++;
                cur_x = 0;
            }
        }
        idx++;
    }
    return line_len;
}

static u32 GetBufferIndexFromCursor(NotepadText* TextSelf) {
    u32 cur_y = 0;
    u32 cur_x = 0;
    u32 idx = 0;
    u32 len = GetRealBufferLength(TextSelf);

    while (idx < len) {
        if (cur_y == TextSelf->CursorY && cur_x == TextSelf->CursorX) {
            return idx;
        }
        if (TextSelf->Buffer[idx] == '\n') {
            if (cur_y == TextSelf->CursorY) {
                return idx;
            }
            cur_y++;
            cur_x = 0;
        } else {
            cur_x++;
            if (cur_x >= 76) {
                cur_y++;
                cur_x = 0;
            }
        }
        idx++;
    }
    return idx;
}

static void NotepadInsertSingleChar(NotepadText* TextSelf, NotepadShell* ShellSelf, char c) {
    u32 text_idx = GetBufferIndexFromCursor(TextSelf);
    u32 current_len = GetRealBufferLength(TextSelf);

    if (text_idx > current_len) {
        text_idx = current_len;
    }

    if (current_len < (1 << 20) - 1) {
        for (u32 i = current_len + 1; i > text_idx; i--) {
            TextSelf->Buffer[i] = TextSelf->Buffer[i - 1];
        }
    }

    TextSelf->Buffer[text_idx] = c;

    if (c == '\n') {
        TextSelf->CursorY++;
        TextSelf->CursorX = 0;
    } else {
        TextSelf->CursorX++;
        if (TextSelf->CursorX >= 76) {
            TextSelf->CursorX = 0;
            TextSelf->CursorY++;
        }
    }

    if (TextSelf->CursorY > TextSelf->ContentMaxY) {
        TextSelf->ContentMaxY = TextSelf->CursorY;
        TextSelf->ContentMaxX = TextSelf->CursorX;
    } else if (TextSelf->CursorY == TextSelf->ContentMaxY && TextSelf->CursorX > TextSelf->ContentMaxX) {
        TextSelf->ContentMaxX = TextSelf->CursorX;
    }

    if (ShellSelf->X < (SHELL_COLS * SHELL_ROWS - 1)) {
        ShellSelf->Buffer[ShellSelf->X] = c;
        ShellSelf->X++;
    }
}

void NotepadInsertChar(NotepadText* TextSelf, NotepadShell* ShellSelf, char c) {
    if (c == '\t') {
        for (int i = 0; i < 4; i++) {
            NotepadInsertSingleChar(TextSelf, ShellSelf, ' ');
        }
    } else {
        NotepadInsertSingleChar(TextSelf, ShellSelf, c);
    }
}

void* RunJIT(void* Rsvd){
    void (*func)() = (void(*)())(*((u64*)Rsvd));
    func();
    return Rsvd;
}

void KillJIT(){
    DebugChar('\n');
    FreeHolyNeuroJIT(entry);
}

void GlobalNotepadPoll(void* Self) {
    Window* WinSelf = Self;
    NotepadText* TextSelf = Self + sizeof(Window);
    NotepadShell* ShellSelf = Self + sizeof(Window) + sizeof(NotepadText);
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

    TextSelf->CursorBlinkTimer += (u64)(DeltaMs + 0.5);
    if (TextSelf->CursorBlinkTimer >= 500) {
        TextSelf->CursorBlinkTimer = 0;
        TextSelf->CursorVisible = !TextSelf->CursorVisible;
    }

    if (Escape == 1) {
        TextSelf->IsCommandMode = !TextSelf->IsCommandMode;
        Escape = 0;
    }

    TextSelf->TotalLines = TextSelf->ContentMaxY + 1;

    if (TextSelf->IsCommandMode) {
        if (Char == 'i' || Char == 'I') {
            TextSelf->IsCommandMode = 0;
            Char=0;
        } else if (Char == 'q' || Char == 'Q') {
            GlobalNotepadExit(WinSelf);
            return;
        } else if (Char == 'c' || Char == 'C') {
            TextSelf->CursorX = 0; TextSelf->CursorY = 0;
            TextSelf->ContentMaxX = 0; TextSelf->ContentMaxY = 0;
            TextSelf->XTmp = 0; TextSelf->YTmp = 0;
            MemSet(TextSelf->Buffer,0,1048576);
            ShellSelf->X = 0;
            MemSet(ShellSelf->Buffer,0,1580);
        } else if (Char == 'r' || Char == 'R') {
            char* TextOut = RandomString();
            while (*TextOut) { NotepadInsertChar(TextSelf, ShellSelf, *TextOut++); }
            NotepadInsertChar(TextSelf, ShellSelf, ' ');
        } else if (Char =='f' || Char == 'F') {
            char* TextOut = RandomIDKWhatTheFuckingThisWord();
            while (*TextOut) { NotepadInsertChar(TextSelf, ShellSelf, *TextOut++); }
            NotepadInsertChar(TextSelf, ShellSelf, ' ');
        } else if (Char == 'j') {
            if (TextSelf->CursorY < TextSelf->ContentMaxY) TextSelf->CursorY++;
        } else if (Char == 'k') {
            if (TextSelf->CursorY > 0) TextSelf->CursorY--;
        } else if (Char == 'h') {
            if (TextSelf->CursorX > 0) TextSelf->CursorX--;
        } else if (Char == 'l') {
            if (TextSelf->CursorX < GetLineLength(TextSelf, TextSelf->CursorY)) TextSelf->CursorX++;
        } else if (Char == 'm') {
            u8* buffer = (u8*)AlignedAlloc(&KernelPool, 512 << 10, 64);
            if (buffer) {
                entry = HolyNeuroJIT(TextSelf->Buffer, buffer, 512 << 10);
                if (entry) {
                    JITTaskId = TaskAdd((Task){
                        .Active=1,
                        .Arg=&entry,
                        .IntervalNs=10000,
                        .CallFunc=RunJIT,
                        .DelFunc=NULL,
                        .CoreID=(u8)rdrand(),
                        .Name=L"RunningJIT Notepad",
                        .NextWaitNs=0,
                        .Target=NULL,
                    }, 0);
                }
            }
        } else if (Char=='n') {
            TaskDel(JITTaskId);
            KillJIT();
        } else if (Char == 'z') {
            u8* buffer = (u8*)AlignedAlloc(&KernelPool, 512 << 10, 64);
            if (buffer) {
                entry = HolyNeuroJIT(TextSelf->Buffer, buffer, 512 << 10);
                if (entry) {
                    RunJIT(&entry);
                }
                KillJIT();
            }
        }
    }

    u32 VisibleRows = 20;
    if (TextSelf->CursorY >= TextSelf->ScrollOffset + VisibleRows) {
        TextSelf->ScrollOffset = TextSelf->CursorY - VisibleRows + 1;
    }
    if (TextSelf->CursorY < TextSelf->ScrollOffset) {
        TextSelf->ScrollOffset = TextSelf->CursorY;
    }

    if (TextSelf->IsCommandMode) return;

    if (TopF1To12Key == 7) {
        char* TextOut = RandomString();
        DebugStr(TextOut);
        DebugChar(' ');
        while (*TextOut) {
            NotepadInsertChar(TextSelf, ShellSelf, *TextOut++);
        }
        NotepadInsertChar(TextSelf, ShellSelf, ' ');
    } else if (TopF1To12Key == 8) {
        char* TextOut = RandomIDKWhatTheFuckingThisWord();
        DebugStr(TextOut);
        DebugChar(' ');
        while (*TextOut) {
            NotepadInsertChar(TextSelf, ShellSelf, *TextOut++);
        }
        NotepadInsertChar(TextSelf, ShellSelf, ' ');
    }

    if (Get0xE0Down) {
        if (Get == 0x4B) {
            if (TextSelf->CursorX > 0) {
                TextSelf->CursorX--;
            } else if (TextSelf->CursorY > 0) {
                TextSelf->CursorY--;
                TextSelf->CursorX = GetLineLength(TextSelf, TextSelf->CursorY);
            }
        } else if (Get == 0x4D) {
            if (TextSelf->CursorX < GetLineLength(TextSelf, TextSelf->CursorY)) {
                TextSelf->CursorX++;
            } else if (TextSelf->CursorY < TextSelf->ContentMaxY) {
                TextSelf->CursorY++;
                TextSelf->CursorX = 0;
            }
        } else if (Get == 0x48) {
            if (TextSelf->CursorY > 0) {
                TextSelf->CursorY--;
                u32 line_len = GetLineLength(TextSelf, TextSelf->CursorY);
                if (TextSelf->CursorX > line_len) TextSelf->CursorX = line_len;
            }
        } else if (Get == 0x50) {
            if (TextSelf->CursorY < TextSelf->ContentMaxY) {
                TextSelf->CursorY++;
                u32 line_len = GetLineLength(TextSelf, TextSelf->CursorY);
                if (TextSelf->CursorX > line_len) TextSelf->CursorX = line_len;
            }
        } else if (Get == 0x47) {
            TextSelf->CursorX = 0;
        } else if (Get == 0x4F) {
            TextSelf->CursorX = GetLineLength(TextSelf, TextSelf->CursorY);
        } else if (Get == 0x53) {
            u32 delete_idx = TextSelf->CursorY * 76 + TextSelf->CursorX;
            u32 total_bytes = GetRealBufferLength(TextSelf);

            if (delete_idx < total_bytes) {
                for (u32 i = delete_idx; i < total_bytes; i++) {
                    TextSelf->Buffer[i] = TextSelf->Buffer[i + 1];
                }

                if (total_bytes > 0) total_bytes--;
                TextSelf->Buffer[total_bytes] = '\0';

                TextSelf->ContentMaxY = total_bytes / 76;
                TextSelf->ContentMaxX = total_bytes % 76;
            }
        }
        return;
    }

    if (Get == 0x53) {
        u32 delete_idx = TextSelf->CursorY * 76 + TextSelf->CursorX;
        u32 total_bytes = GetRealBufferLength(TextSelf);

        if (delete_idx < total_bytes) {
            for (u32 i = delete_idx; i < total_bytes; i++) {
                TextSelf->Buffer[i] = TextSelf->Buffer[i + 1];
            }

            if (total_bytes > 0) total_bytes--;
            TextSelf->Buffer[total_bytes] = '\0';

            TextSelf->ContentMaxY = total_bytes / 76;
            TextSelf->ContentMaxX = total_bytes % 76;
        }
    }

    if (Get==0x1D || (Get0xE0Down && Get==0x1D)) {
        if (Char == 'a' || Char == 'A') {
            TextSelf->SelectionStartX = 0;
            TextSelf->SelectionStartY = 0;
            TextSelf->SelectionEndX = TextSelf->ContentMaxX;
            TextSelf->SelectionEndY = TextSelf->ContentMaxY;
            TextSelf->HasSelection = 1;
        } else if (Char == 'c' || Char == 'C') {
            u32 line_start = TextSelf->CursorY * 76;
            u32 len = GetLineLength(TextSelf, TextSelf->CursorY);
            ClipboardLen = 0;
            for (u32 i = 0; i < len && ClipboardLen < sizeof(ClipboardBuffer) - 1; i++) {
                ClipboardBuffer[ClipboardLen++] = TextSelf->Buffer[line_start + i];
            }
            ClipboardBuffer[ClipboardLen] = '\0';
        } else if (Char == 'v' || Char == 'V') {
            for (u32 i = 0; i < ClipboardLen; i++) {
                NotepadInsertChar(TextSelf, ShellSelf, ClipboardBuffer[i]);
            }
        }
    }

    if (Char != 0 && Char != '\b' && !Get0xE0Down && Char!='\033') {
        NotepadInsertChar(TextSelf, ShellSelf, Char);
    }

    if (Char == '\b') {
        u32 total_bytes = GetRealBufferLength(TextSelf);
        if (total_bytes > 0 && (TextSelf->CursorX > 0 || TextSelf->CursorY > 0)) {
            
            if (TextSelf->CursorX == 0) {
                TextSelf->CursorY--;
                TextSelf->CursorX = GetLineLength(TextSelf, TextSelf->CursorY);
            } else {
                TextSelf->CursorX--;
            }
            
            u32 delete_idx = 0;
            u32 cur_y = 0;
            u32 cur_col = 0;
            
            while (delete_idx < total_bytes) {
                char c = TextSelf->Buffer[delete_idx];
                if (cur_y == TextSelf->CursorY && cur_col == TextSelf->CursorX) {
                    break;
                }
                if (c == '\n') {
                    cur_y++;
                    cur_col = 0;
                } else if (c == '\t') {
                    cur_col += 4;
                    if (cur_col >= 76) { cur_y++; cur_col = 0; }
                } else {
                    cur_col++;
                    if (cur_col >= 76) { cur_y++; cur_col = 0; }
                }
                delete_idx++;
            }

            for (u32 i = delete_idx; i < total_bytes; i++) {
                TextSelf->Buffer[i] = TextSelf->Buffer[i + 1];
            }
            
            TextSelf->Buffer[total_bytes - 1] = '\0';

            u32 new_len = GetRealBufferLength(TextSelf);
            u32 max_y = 0, max_x = 0;
            cur_y = 0; cur_col = 0;
            for (u32 i = 0; i < new_len; i++) {
                char c = TextSelf->Buffer[i];
                if (c == '\n') {
                    max_y++;
                    max_x = 0;
                    cur_y++;
                    cur_col = 0;
                } else {
                    cur_col++;
                    if (cur_col > max_x && cur_y == max_y) max_x = cur_col;
                    if (cur_col >= 76) {
                        cur_y++;
                        cur_col = 0;
                    }
                }
            }
            TextSelf->ContentMaxY = max_y;
            TextSelf->ContentMaxX = max_x;
        }
    }
}

void GlobalNotepadDraw(void* Self) {
    Window* WinSelf = Self;
    NotepadText* TextSelf = Self + sizeof(Window);

    GOPRectFill(
        (u32[2]){(u32)WinSelf->Base.x, (u32)WinSelf->Base.x + (u32)WinSelf->Base.w},
        (u32[2]){(u32)WinSelf->Base.y, (u32)WinSelf->Base.y + 22},
        TextSelf->IsCommandMode ? PACK_COLOR(0x88, 0x00, 0x88, 0xFF) : PACK_COLOR(0x00, 0xAA, 0xAA, 0xFF)
    );

    char title_buf[64];
    if (TextSelf->IsCommandMode) {
        const char cmd_title[] = "Notepad [NORMAL] i:Insert q:Quit c:Clear hjkl:Move";
        MemCopy(title_buf, (void*)cmd_title, sizeof(cmd_title));
    } else {
        const char normal_title[] = "Notepad [INSERT] Esc:Normal Mode (F7/F8)";
        MemCopy(title_buf, (void*)normal_title, sizeof(normal_title));
    }

    GOPStringWH(
        (u32)WinSelf->Base.x + 4,
        (u32)WinSelf->Base.y + 1,
        (u32)WinSelf->Base.w,
        (u32)WinSelf->Base.h,
        title_buf,
        PACK_COLOR(0x0000, 0x0000, 0x0000, 0x0000),
        PACK_COLOR(0xFF, 0xFF, 0xFF, 0xFF),
        KernelFonts,
        0
    );

    u32 EditorHeight = (u32)WinSelf->Base.h - 38;
    GOPRectFill(
        (u32[2]){(u32)WinSelf->Base.x, (u32)WinSelf->Base.x + (u32)WinSelf->Base.w},
        (u32[2]){(u32)WinSelf->Base.y + 20, (u32)WinSelf->Base.y + 20 + EditorHeight},
        PACK_COLOR(0xFF, 0xFF, 0xFF, 0xFF)
    );

    u32 LineNumX = (u32)WinSelf->Base.x + 4;
    u32 LineNumY = (u32)WinSelf->Base.y + 20 + 2;
    u32 VisibleRows = 20;
    char line_num_str[8];

    for (u32 i = 0; i < VisibleRows; i++) {
        u32 line = TextSelf->ScrollOffset + i + 1;
        int len = 0;
        u32 num = line;
        if (num == 0) {
            line_num_str[len++] = '0';
        } else {
            char tmp[8];
            int t = 0;
            while (num > 0 && t < 7) {
                tmp[t++] = '0' + (num % 10);
                num /= 10;
            }
            while (t > 0) {
                line_num_str[len++] = tmp[--t];
            }
        }
        line_num_str[len] = '\0';

        GOPStringWH(
            LineNumX,
            LineNumY + i * 19,
            30,
            19,
            line_num_str,
            PACK_COLOR(0xFF, 0xFF, 0xFF, 0xFF),
            PACK_COLOR(0x88, 0x88, 0x88, 0xFF),
            KernelFonts,
        0
        );
    }

    u32 TextX = (u32)WinSelf->Base.x + 8 * 3 + 30;
    u32 TextY = (u32)WinSelf->Base.y + 20 + 2;

    u32 buf_total = TextSelf->ContentMaxY * 76 + TextSelf->ContentMaxX;
    if (buf_total > (1 << 20)) buf_total = (1 << 20);

    u32 start_idx = 0;
    u32 line_count = 0;
    u32 col_count = 0;

    for (u32 i = 0; i < buf_total && line_count < TextSelf->ScrollOffset; i++) {
        if (TextSelf->Buffer[i] == '\n') {
            line_count++;
            col_count = 0;
        } else if (TextSelf->Buffer[i] == '\t') {
            col_count += 4;
            if (col_count >= 76) { line_count++; col_count = 0; }
        } else {
            col_count++;
            if (col_count >= 76) { line_count++; col_count = 0; }
        }
        start_idx = i + 1;
    }

    char display_buf[79 * 20 + 1];
    u32 buf_idx = 0;
    u32 lines_displayed = 0;
    u32 cur_col = 0;

    for (u32 i = start_idx; i < buf_total && lines_displayed < VisibleRows; i++) {
        if (buf_idx >= sizeof(display_buf) - 1) break;

        char c = TextSelf->Buffer[i];
        if (c == '\0') continue;

        if (c == '\n') {
            display_buf[buf_idx++] = '\n';
            lines_displayed++;
            cur_col = 0;
        } else if (c == '\t') {
            display_buf[buf_idx++] = '\t';
            cur_col += 4;
            if (cur_col >= 76) {
                if (buf_idx < sizeof(display_buf) - 1) display_buf[buf_idx++] = '\n';
                lines_displayed++;
                cur_col = 0;
            }
        } else {
            display_buf[buf_idx++] = c;
            cur_col++;
            if (cur_col >= 76) {
                if (buf_idx < sizeof(display_buf) - 1) display_buf[buf_idx++] = '\n';
                lines_displayed++;
                cur_col = 0;
            }
        }
    }
    display_buf[buf_idx] = '\0';

    GOPStringWH(
        TextX,
        TextY,
        (u32)WinSelf->Base.w - 30,
        (u32)WinSelf->Base.h - 38,
        display_buf,
        PACK_COLOR(0xFF, 0xFF, 0xFF, 0xFF),
        PACK_COLOR(0x00, 0x00, 0x00, 0xFF),
        KernelFonts,
        0
    );

    if (TextSelf->CursorVisible || TextSelf->IsCommandMode) {
        s32 cursor_row = (s32)TextSelf->CursorY - (s32)TextSelf->ScrollOffset;
        if (cursor_row >= 0 && cursor_row < 20) {
            u32 CursorX = TextX + (TextSelf->CursorX % 76) * 8;
            u32 CursorY = TextY + (u32)cursor_row * 19;
            u32 CursorWidth = TextSelf->IsCommandMode ? 8 : 2;

            GOPRectFill(
                (u32[2]){CursorX, CursorX + CursorWidth},
                (u32[2]){CursorY, CursorY + 19},
                PACK_COLOR(0x00, 0x00, 0x00, 0xFF)
            );
        }
    }

    u32 StatusY = (u32)WinSelf->Base.y + (u32)WinSelf->Base.h - 18;
    GOPRectFill(
        (u32[2]){(u32)WinSelf->Base.x, (u32)WinSelf->Base.x + (u32)WinSelf->Base.w},
        (u32[2]){StatusY, (u32)WinSelf->Base.y + (u32)WinSelf->Base.h + 2},
        PACK_COLOR(0xDD, 0xDD, 0xDD, 0xFF)
    );

    char status_str[64];
    u32 u16_count = GetRealBufferLength(TextSelf);
    
    const char s_ln[] = " Ln: ";
    u32 idx = 0;
    for (u32 i = 0; s_ln[i]; i++) status_str[idx++] = s_ln[i];
    
    u32 num = TextSelf->CursorY + 1;
    char tmp[8]; u32 t = 0;
    if (num == 0) tmp[t++] = '0';
    while (num > 0 && t < 7) { tmp[t++] = '0' + (num % 10); num /= 10; }
    while (t > 0) status_str[idx++] = tmp[--t];

    const char s_col[] = ", Col: ";
    for (u32 i = 0; s_col[i]; i++) status_str[idx++] = s_col[i];

    num = TextSelf->CursorX + 1; t = 0;
    if (num == 0) tmp[t++] = '0';
    while (num > 0 && t < 7) { tmp[t++] = '0' + (num % 10); num /= 10; }
    while (t > 0) status_str[idx++] = tmp[--t];

    const char s_len[] = " | Total: ";
    for (u32 i = 0; s_len[i]; i++) status_str[idx++] = s_len[i];

    num = u16_count; t = 0;
    if (num == 0) tmp[t++] = '0';
    while (num > 0 && t < 7) { tmp[t++] = '0' + (num % 10); num /= 10; }
    while (t > 0) status_str[idx++] = tmp[--t];

    status_str[idx] = '\0';

    GOPStringWH(
        (u32)WinSelf->Base.x + 8,
        StatusY + 1,
        (u32)WinSelf->Base.w - 16,
        18,
        status_str,
        PACK_COLOR(0xDD, 0xDD, 0xDD, 0xFF),
        PACK_COLOR(0x33, 0x33, 0x33, 0xFF),
        KernelFonts,
        0
    );
}

void NewNotepadWindow(void) {
    Window* NewNotepad = Alloc(&KernelPool, sizeof(Window) + sizeof(NotepadText) + sizeof(NotepadShell));
    NewNotepad->SomeID = 0x44415045544F4E00ULL + NotepadCount;
    NewNotepad->Base.Exit = GlobalNotepadExit;
    NewNotepad->Base.Visible = 1;
    NewNotepad->Base.x = 200;
    NewNotepad->Base.y = 100;
    NewNotepad->Base.w = 8 * 86;
    NewNotepad->Base.h = 19 * 23;
    NewNotepad->Base.Type = 0;
    NewNotepad->Base.Poll = GlobalNotepadPoll;
    NewNotepad->Base.Draw = GlobalNotepadDraw;
    NewNotepad->Hide = 1;
    NewNotepad->Title = "Notepad";

    NotepadText* TextSelf = (NotepadText*)((u8*)NewNotepad + sizeof(Window));
    TextSelf->CursorVisible = 1;
    TextSelf->ScrollOffset = 0;
    TextSelf->TotalLines = 1;
    TextSelf->IsCommandMode = 0;
    TextSelf->CursorX = 0;
    TextSelf->CursorY = 0;
    TextSelf->ContentMaxX = 0;
    TextSelf->ContentMaxY = 0;
    TextSelf->HasSelection = 0;

    WMAddWindow(&KernelGlobalWindowManager, NewNotepad);
    NotepadCount++;
}