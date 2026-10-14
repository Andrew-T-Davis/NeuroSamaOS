#pragma once
#include "int.h"
#include "ahci.h"

Bool WriteDisk(AHCIController* Ctrl, u8 Idx, void* Buffer, Size64 Size, u64 OffSet);
u8* ReadDisk(AHCIController* Ctrl, u8 Idx, Size64 Size, u64 OffSet);
