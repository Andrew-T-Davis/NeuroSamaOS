#pragma once

#include "int.h"

u64 HolyNeuroJIT(char* Code, u8* buffer, u64 buffer_size);
void FreeHolyNeuroJIT(u64 entry);