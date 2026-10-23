#pragma once

#include "int.h"

typedef struct {
    struct CopyObj *Next;
    struct CopyObj *Pre;
    u64 Len;
    char Text[];
} CopyObj;

typedef struct {
    CopyObj *Tail;
    CopyObj *Head;
} CopyObjRoot;