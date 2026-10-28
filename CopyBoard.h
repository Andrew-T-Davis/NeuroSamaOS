#pragma once

#include "int.h"

typedef struct {
    struct CopyObj *Next;
    struct CopyObj *Pre;
    u64 Len;
    char Text[];
} CopyBoardObj;

typedef struct {
    CopyBoardObj *Tail;
    CopyBoardObj *Head;
} CopyBoardObjRoot;