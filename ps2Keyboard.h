#pragma once

#include "int.h"

extern char Char;

extern Bool LeftCtrl;
extern Bool LeftShift;
extern Bool RightShift;
extern Bool LeftAlt;
extern Bool CapsLock;
extern Bool NumberLock;
extern Bool Escape;
extern Bool Tab;
extern Bool Backspace;
extern Bool Get0xE0Down;
extern u8 Get;
extern u8 TopF1To12Key;

void PS2KeyboardInt();

__attribute__((used)) static inline char GetPS2KeyboardKey(){
    return Char;
}