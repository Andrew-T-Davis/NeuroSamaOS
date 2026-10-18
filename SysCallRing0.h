#pragma once

#include "int.h"

void InitSysCallRing0();
u64 SysCall(u64 Func, u64 SubFunc, void *Args, u64 ArgCount);