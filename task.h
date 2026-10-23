#pragma once

#include "int.h"

#define MaxTaskCount 256
#define UserTaskStart 50
#define SystemTaskStart 0
#define SystemTaskEnd 49

#define ERROR(x) (x>=MaxTaskCount)

extern Bool InTask;

typedef struct{
    void* (*CallFunc)(void* ArgList);
    void* (*DelFunc)(void* ArgList);
    void *Arg;
    Bool Active;
    const u16* Name;
    u64 NextWaitNs;
    u64 IntervalNs;
    u64 CoreID;
    void* Target;
} Task;

u16 TaskAdd(Task AddTask,Bool SystemTask);
void TaskDel(u16 IdxTask);
void TaskPoll();
void TaskInit();
