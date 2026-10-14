#pragma once

#include "int.h"

typedef struct Obj{
    u64 w,h,x,y;
    Bool Visible;
    u64 Type;
    void (*Draw)(void* self);
    void (*Poll)(void* self);
    void (*Exit)(void* self);
    void (*Size)(void* self);
} Obj;

typedef struct Window Window;
struct Window{
    Obj Base;
    Window* Pre;
    Window* Next;
    u64 SomeID;
    char* Title;
    Bool Hide;
    u32 DragOffsetX;
    u32 DragOffsetY;
    Bool IsDragging;
    u64 Count;
    void *Objs[];
};

typedef struct TextBox{
    Obj Base;
    u64 Length;
    char* Text;
} TextBox;

typedef struct Button{
    Obj Base;
    Bool Pressed;
    void (*Click)(void);
    void (*Hover)(void);
} Button;
