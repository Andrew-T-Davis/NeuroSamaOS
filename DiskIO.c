#include "DiskIO.h"
#include "int.h"
#include "ahci.h"
#include "debug.h"
#include "kmalloc.h"
#include "str.h"

extern AllocPool KernelPool;

Bool WriteDisk(AHCIController* Ctrl, u8 Idx, void* Buffer, Size64 Size, u64 OffSet) {
    if (!Ctrl || !Buffer || Size == 0) return 0;

    u64 lba = OffSet;
    u16 count = (u16)Size;

    u8 slot = Ctrl->PortFISSlot[Idx] % 32;

    if (AHCISendMessage(Ctrl, Idx, slot, lba, count, Buffer, 1) == -1) {
        DebugStr("[AHCI Write] Failed to write disk on Port ");
        DebugU8(Idx);
        DebugChar('\n');
        return 0;
    }

    return 1;
}

u8* ReadDisk(AHCIController* Ctrl, u8 Idx, Size64 Size, u64 OffSet) {
    if (!Ctrl || !Size || Size == 0) return NULL;

    u64 lba = OffSet;
    u16 count = (u16)(Size);
    Size64 byte_size = (Size64)count * 512;

    u8* buffer = (u8*)AlignedAlloc(&KernelPool, byte_size, 256);
    if (!buffer) {
        DebugStr("[AHCI Read] Failed to allocate DMA buffer!\n");
        return NULL;
    }
    MemSet(buffer, 0, byte_size);

    u8 slot = Ctrl->PortFISSlot[Idx] % 32;

    if (AHCISendMessage(Ctrl, Idx, slot, lba, count, buffer, 0) == -1) {
        DebugStr("[AHCI Read] Read operation failed on Port ");
        DebugU8(Idx);
        DebugChar('\n');
        Free(&KernelPool, buffer);
        return NULL;
    }

    return buffer;
}