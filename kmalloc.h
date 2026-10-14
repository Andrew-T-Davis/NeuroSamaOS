#pragma once

#include "int.h"

typedef struct AllocBlock {
    struct AllocBlock* next;
    struct AllocBlock* prev;
    Size64 Size;
    u8 is_free;
    u8 is_aligned;
    u8 padding[7];
} __attribute__((aligned(64))) AllocBlock;

typedef struct {
    AllocBlock* Head;
} __attribute__((aligned(64))) AllocPool;

void PoolAddBlock(AllocPool* Pool, AllocBlock* Block);
void* Alloc(AllocPool* Pool, Size64 Size);
void* AlignedAlloc(AllocPool* Pool, Size64 Size, Size64 align);
void* MaxAlloc(AllocPool* Pool, Size64 Size, u64 MaxPos);
void* MaxAlignedAlloc(AllocPool* Pool, Size64 Size, Size64 align, u64 MaxPos);
void Free(AllocPool* Pool, void* ptr);
void MergeFreeBlocks(AllocBlock* block);
