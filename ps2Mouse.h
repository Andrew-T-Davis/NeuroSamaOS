#include "int.h"

typedef struct {
    union {
        u8 U;
        struct {
            u8 ButtonLeft    : 1;
            u8 ButtonRight   : 1;
            u8 ButtonMiddle  : 1;
            u8 AlwaysOne     : 1;
            u8 XAxisSignBit  : 1;
            u8 YAxisSignBit  : 1;
            u8 XAxisOverflow : 1;
            u8 YAxisOverflow : 1;
        };
    } Code;
    u8 XAxisMovementValue;
    u8 YAxisMovementValue;
} __attribute__((packed)) PS2MouseStatus;

typedef struct {
    union {
        u8 U;
        struct {
            u8 ButtonLeft   : 1;
            u8 ButtonRight  : 1;
            u8 ButtonMiddle : 1;
            u8 Rsvd0        : 5;
        };
    } Code;
    u32 X;
    u32 Y;
} __attribute__((packed)) PS2Mouse;

extern PS2Mouse GlobalPS2Mouse;
extern PS2Mouse GlobalOldPS2Mouse;

void           PS2MouseInt();
PS2MouseStatus GetPS2MouseStatus();
PS2Mouse       GetPS2Mouse();
void           SetPS2Mouse(PS2Mouse PS2MouseArg);