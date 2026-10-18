#include "flash.h"

void CompilerBarrier(void) {
    __asm__ volatile("" ::: "memory");
}

void MemFlash(void) {
    __asm__ volatile("sfence" ::: "memory");
}

void LoadFlash(void) {
    __asm__ volatile("lfence" ::: "memory");
}

void MemFullFlash(void) {
    __asm__ volatile("sfence" ::: "memory");
    __asm__ volatile("lfence" ::: "memory");
    __asm__ volatile("mfence" ::: "memory");
}
