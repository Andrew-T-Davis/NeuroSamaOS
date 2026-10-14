#include "kmalloc.h"
#include "int.h"

static void* DoSplit(AllocPool* Pool, AllocBlock* curr, void* aligned_ptr, Size64 Size, int is_aligned) {
    if (!Pool || !curr || !aligned_ptr || Size == 0) return NULL;
    
    u64 block_start = (u64)curr + sizeof(AllocBlock);
    u64 data_start = (u64)aligned_ptr;
    
    if (data_start > block_start) {
        Size64 padding = data_start - block_start;
        AllocBlock* prefix = curr;
        
        curr = (AllocBlock*)(data_start - sizeof(AllocBlock));
        curr->Size = prefix->Size - padding;
        curr->is_free = 1;
        curr->is_aligned = 0;
        
        prefix->Size = padding - sizeof(AllocBlock);
        
        curr->next = prefix->next;
        curr->prev = prefix;
        if (prefix->next) prefix->next->prev = curr;
        prefix->next = curr;
    }
    
    Size64 remaining = curr->Size - Size;
    if (remaining >= sizeof(AllocBlock) + 8) {
        AllocBlock* new_block = (AllocBlock*)(data_start + Size);
        new_block->Size = remaining - sizeof(AllocBlock);
        new_block->is_free = 1;
        new_block->is_aligned = 0;
        
        new_block->next = curr->next;
        new_block->prev = curr;
        if (curr->next) curr->next->prev = new_block;
        
        curr->Size = Size;
        curr->next = new_block;
    }
    
    curr->is_free = 0;
    curr->is_aligned = (Bool)is_aligned;
    return (void*)data_start;
}

void* Alloc(AllocPool* Pool, Size64 Size) {
    if (!Pool || !Size) return NULL;
    Size = (Size + 7) & ~7ULL;
    
    AllocBlock* curr = Pool->Head;
    while (curr) {
        if (curr->is_free && curr->Size >= Size) {
            void* result = (void*)((u8*)curr + sizeof(AllocBlock));
            return DoSplit(Pool, curr, result, Size, 0);
        }
        curr = curr->next;
    }
    return NULL;
}

void* AlignedAlloc(AllocPool* Pool, Size64 Size, Size64 align) {
    if (!Pool || !Size) return NULL;
    if (align < 8) align = 8;
    Size = (Size + 7) & ~7ULL;

    AllocBlock* curr = Pool->Head;
    while (curr) {
        
        if (curr->is_free && curr->Size >= Size) {
            u64 block_start = (u64)curr + sizeof(AllocBlock);
            u64 aligned_ptr = (block_start + align - 1) & ~(align - 1);
            
            while (aligned_ptr > block_start && (aligned_ptr - block_start) < (sizeof(AllocBlock) + 8)) {
                aligned_ptr += align;
            }
            
            Size64 total_needed = (aligned_ptr - block_start) + Size;
            if (curr->Size >= total_needed) {
                return DoSplit(Pool, curr, (void*)aligned_ptr, Size, 1);
            }
        }
        curr = curr->next;
        if ((u64)curr < 0x1000) {
            break;
        }
    }
    return NULL;
}

void MergeFreeBlocks(AllocBlock* block) {
    if (!block || !block->is_free || (u64)block < (4 << 10)) return;
    
    AllocBlock* next = block->next;
    while (next > (AllocBlock*)0xFFF && next->is_free) {
        block->Size += sizeof(AllocBlock) + next->Size;
        block->next = next->next;
        if (block->next > (AllocBlock*)0xFFF) block->next->prev = block;
        next = block->next;
    }
    
    AllocBlock* prev = block->prev;
    if (prev > (AllocBlock*)0xFFF && prev->is_free) {
        prev->Size += sizeof(AllocBlock) + block->Size;
        prev->next = block->next;
        if (block->next > (AllocBlock*)0xFFF) block->next->prev = prev;
    }
}

void* MaxAlloc(AllocPool* Pool, Size64 Size, u64 MaxPos) {
    if (!Pool || !Size) return NULL;
    Size = (Size + 7) & ~7ULL;
    AllocBlock* curr = Pool->Head;
    AllocBlock* best = NULL;
    u64 best_addr = ~0ULL;
    while (curr) {
        if (curr->is_free && curr->Size >= Size) {
            u64 block_start = (u64)curr + sizeof(AllocBlock);
            if (block_start <= MaxPos && block_start + Size <= MaxPos) {
                if (block_start < best_addr) {
                    best_addr = block_start;
                    best = curr;
                }
            }
        }
        curr = curr->next;
    }
    if (best) {
        void* result = (void*)((u8*)best + sizeof(AllocBlock));
        return DoSplit(Pool, best, result, Size, 0);
    }
    return NULL;
}

void* MaxAlignedAlloc(AllocPool* Pool, Size64 Size, Size64 align, u64 MaxPos) {
    if (!Pool || !Size) return NULL;
    if (align < 8) align = 8;
    Size = (Size + 7) & ~7ULL;
    AllocBlock* curr = Pool->Head;
    AllocBlock* best = NULL;
    u64 best_aligned_ptr = 0;
    u64 best_addr = ~0ULL;
    while (curr) {
        if (curr->is_free && curr->Size >= Size) {
            u64 block_start = (u64)curr + sizeof(AllocBlock);
            u64 aligned_ptr = (block_start + align - 1) & ~(align - 1);
            
            while (aligned_ptr > block_start && (aligned_ptr - block_start) < (sizeof(AllocBlock) + 8)) {
                aligned_ptr += align;
            }
            
            Size64 total_needed = (aligned_ptr - block_start) + Size;
            if (curr->Size >= total_needed && aligned_ptr <= MaxPos && aligned_ptr + Size <= MaxPos) {
                if (aligned_ptr < best_addr) {
                    best_addr = aligned_ptr;
                    best = curr;
                    best_aligned_ptr = aligned_ptr;
                }
            }
        }
        curr = curr->next;
    }
    if (best) {
        return DoSplit(Pool, best, (void*)best_aligned_ptr, Size, 1);
    }
    return NULL;
}

void* AllocUC(AllocPool* Pool, Size64 Size){
    if (!Pool || !Size) return NULL;
    Size = (Size + 7) & ~7ULL;
    
    AllocBlock* curr = Pool->Head;
    while (curr) {
        if (curr->is_free && curr->Size >= Size) {
            void* result = (void*)((u8*)curr + sizeof(AllocBlock));
            
            return DoSplit(Pool, curr, result, Size, 0);
        }
        curr = curr->next;
    }
    return NULL;
}

void Free(AllocPool* Pool, void* ptr) {
    if (!ptr || !Pool || (u64)ptr < (4 << 10)) return;
    
    AllocBlock* block = (AllocBlock*)((u8*)ptr - sizeof(AllocBlock));
    
    if (block->is_free) return;
    
    block->is_free = 1;
    block->is_aligned = 0;
    
    MergeFreeBlocks(block);
}

void PoolAddBlock(AllocPool* Pool, AllocBlock* Block) {
    if (!Pool || !Block) return;
    Block->next = Pool->Head;
    Block->prev = NULL;
    Block->is_free = 1;
    Block->is_aligned = 0;
    if (Pool->Head) Pool->Head->prev = Block;
    Pool->Head = Block;
}