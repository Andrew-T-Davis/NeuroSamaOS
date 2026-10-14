#include "event.h"
#include "int.h"

static volatile u8 HaveEvent=0;

void MakeEvent(){
    HaveEvent=1;
}

void WaitForEvent() {
    while (1) {
        __asm__ volatile(
            "sti\n\t"
            "hlt\n\t"
            ::: "memory"
        );

        __asm__ volatile("cli" ::: "memory");
        if (HaveEvent) {
            HaveEvent = 0;
            __asm__ volatile("sti" ::: "memory");
            break;
        }
        __asm__ volatile("sti" ::: "memory");
    }
}