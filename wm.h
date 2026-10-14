#pragma once

#include "int.h"
#include "gui.h"

typedef struct WMWindow WMWindow;
struct WMWindow{
    Window* Pre;
    Window* Next;
};

typedef struct {
    Window* Head;
    Window* Tail;
} WM;

void WMAddWindow(WM* WinM,Window* Win);
void WMDelWindow(WM *WinM, const char *Title, u64 SomeID);
void WMUpWindow(WM *WinM, const char* Title, u64 SomeID);
void WMPollDraw(WM* WinM);
void WMPollPoll(WM* WinM);
