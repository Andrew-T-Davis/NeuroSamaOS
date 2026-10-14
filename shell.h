#pragma once

#include "int.h"

typedef struct {
    u32 X,XTmp;
    char Text[5920];
} LineShell;

void NewShellWindow(void);