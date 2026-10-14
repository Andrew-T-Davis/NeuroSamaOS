#include "ps2Keyboard.h"
#include "event.h"
#include "port.h"
#include "int.h"
#include "ioapic.h"

char Char=0;

Bool LeftCtrl=0;
Bool LeftShift=0;
Bool RightShift=0;
Bool LeftAlt=0;
Bool CapsLock=0;
Bool NumberLock=0;
Bool Escape=0;
Bool Tab=0;
Bool Backspace=0;
Bool Get0xE0Down=0;

u8 Get=0;
u8 TopF1To12Key=0;

const char ScanKey2Ascii[] = {
    0x00, 0x1B, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0x00, 0x00,
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0x00, 'a', 's',
    'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0x00, '\\', 'z', 'x', 'c', 'v',
    'b', 'n', 'm', ',', '.', '/', 0x00, '*', 0x00, ' ', 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, '7', '8', '9', '-', '4', '5', '6', '+', '1',
    '2', '3', '0', '.', 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

const char ScanKey2AsciiUp[] = {
    0x00, 0x1B, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0x00, 0x00,
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', 0x00, 'A', 'S',
    'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0x00, '|', 'Z', 'X', 'C', 'V',
    'B', 'N', 'M', '<', '>', '?', 0x00, '*', 0x00, ' ', 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, '7', '8', '9', '-', '4', '5', '6', '+', '1',
    '2', '3', '0', '.', 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

void PS2KeyboardIntFunc(){
    u8 Status = inb(0x64);
    if ((Status & 0x20) && !(Status & 0x01)) {
        LocalApicEOI();
        return;
    }

    Get = inb(0x60);

    if (Get==0xE0) Get0xE0Down=1;
    
    u8 Down = (Get & 0x80) == 0;
    u8 ScanCode = Get & 0x7F;
    
    if (Get == 0xAA) LeftShift = 0;
    else if (Get == 0xB6) RightShift = 0;
    else if (Get == 0x9D) LeftCtrl = 0;
    else if (Get == 0xB8) LeftAlt = 0;
    else if (Get == 0x8F) Tab = 0;
    else if (Get == 0x8E) Backspace = 0;
    else if (Get == 0x81) Escape = 0;
    
    if (!Down) {
        Char = 0;
        LocalApicEOI();
        MakeEvent();
        return;
    }
    
    if ((ScanCode >= 0x02 && ScanCode <= 0x0D) ||
        (ScanCode >= 0x10 && ScanCode <= 0x1C) ||
        (ScanCode >= 0x1E && ScanCode <= 0x29) ||
        (ScanCode == 0x2B || ScanCode == 0x39) ||
        (ScanCode >= 0x2C && ScanCode <= 0x35) ||
        (ScanCode >= 0x47 && ScanCode <= 0x53)) {
        Char = (CapsLock || LeftShift) ? ScanKey2AsciiUp[ScanCode] : ScanKey2Ascii[ScanCode];
    }
    else {
        if (ScanCode == 0x2A) LeftShift = 1;
        else if (ScanCode == 0x36) RightShift = 1;
        else if (ScanCode == 0x1D) LeftCtrl = 1;
        else if (ScanCode == 0x38) LeftAlt = 1;
        else if (ScanCode == 0x3A) CapsLock = !CapsLock;
        else if (ScanCode == 0x0F) { Tab = 1; Char = '\t'; }
        else if (ScanCode == 0x0E) { Backspace = 1; Char = '\b'; }
        else if (ScanCode == 0x01) { Escape = 1; Char = 0x1B; }
        else if (ScanCode >= 0x3B && ScanCode <= 0x44) {TopF1To12Key = ScanCode - 0x3A;Char=0;Down = 0;}
        else if (ScanCode == 0x57) {TopF1To12Key = 11;Char=0;Down = 0;}
        else if (ScanCode == 0x58) {TopF1To12Key = 12;Char=0;Down = 0;}
        else if (Get      == 0x45) NumberLock = !NumberLock;
        else Char = 0;
    }

    MakeEvent();
    
    LocalApicEOI();
}

__attribute__((naked)) void PS2KeyboardInt(){
    __asm__ volatile(
        "pushq %%rax\n\t" "pushq %%rcx\n\t" "pushq %%rdx\n\t" "pushq %%rbx\n\t"
        "pushq %%rbp\n\t" "pushq %%rsi\n\t" "pushq %%rdi\n\t"
        "pushq %%r8\n\t"  "pushq %%r9\n\t"  "pushq %%r10\n\t" "pushq %%r11\n\t"
        "pushq %%r12\n\t" "pushq %%r13\n\t" "pushq %%r14\n\t" "pushq %%r15\n\t"
        "call PS2KeyboardIntFunc\n\t"
        "popq %%r15\n\t" "popq %%r14\n\t" "popq %%r13\n\t" "popq %%r12\n\t"
        "popq %%r11\n\t" "popq %%r10\n\t" "popq %%r9\n\t"  "popq %%r8\n\t"
        "popq %%rdi\n\t" "popq %%rsi\n\t" "popq %%rbp\n\t" "popq %%rbx\n\t"
        "popq %%rdx\n\t" "popq %%rcx\n\t" "popq %%rax\n\t"
        "iretq\n\t"
        ::: "memory"
    );
}