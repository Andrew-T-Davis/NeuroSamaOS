#include "RandomSomething.h"
#include "clock.h"
#include "int.h"
#include "kmalloc.h"
#include "debug.h"
#include "str.h"

extern AllocPool KernelPool;

const char *Text="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890 !@#$%^&*()-=_+`~,./<>?;':\"[]{}\\|\n\t\b\r\a\e\f\v";

char* WordsInFishsBrain[32]={
    "FemBoy",
    "Hatsune MiKu",
    "Neuro-Sama",
    "Kasane TeTo",
    "KaiTo",
    "MeiKo",
    "NiKo",
    "OneShot",
    "Word",
    "TempleOS",
    "Terry A. Davis",
    "MoNiKa",
    "SaYoRi",
    "Adachi Rei",
    "Gumi",
    "NeKo",
    "CatGirl",
    "OSU World Cup",
    "FIFA World Cup",
    "Erling Braut Haaland",
    "Steven Paul Jobs",
    "Michael Jackson",
    "William Henry Gates III",
    "Linus Torvalds",
    "Windows LongHorn",
    "Windows Vista",
    "Windows 7",
    "Windows 10",
    "Linux",
    "NixOS",
    "MacOS",
    "MacOS X"
};

char* SomeQuestions[16]={
    "You Play OSU?",
    "How Are You?",
    "Happy~ Happy~ Happy~~~~(Happy Cat Meme)",
    "Listen To Music----Night Of Nigths, Goto Youtube!",
    "Vocaloid Is Nice, Right?",
    "TempleOS Is Nice Too, Right?",
    "Xiaomi YYDS!",
    "I Dont Like ASMR btw.",
    "MoNiKa! KaWaI! MoNiKa! KaWaI! ",
    "!!!Neuro-Sama Number 1!!! !!!Neuro-Sama St1!!!",
    "Im Crazy!",
    "Evil-Neuro KaWaI!",
    "How To Make A Map:\nFirst: Make A Map",
    "OneShot Is Fun, But It Make Me Sad Too,NiKo!!!!!!!!No!!!!!!!!!",
    "NightCore Is Fucking Very Nice And Good!!!!!!!",
    "PS/2 And xHCI Is Good, I Think, How About You?",
};
char TmpBuffer[256]={0};

char** StringList;
Size64 WordCount = 0;

u64 rdrand(void) {
    u64 val;
    Bool ok;
    __asm__ volatile(
        "rdrand %0; setc %1"
        : "=r"(val), "=qm"(ok)
        :
        : "cc"
    );
    if (!ok) {
        val = rdtsc();
    }
    return val;
}

char RandomChar(){
    return Text[rdrand() % (sizeof("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890 !@#$%^&*()-=_+`~,./<>?;':\"[]{}\\|\n\t\b\r\a\e\f\v")-1)];
}

void InitRandomString(void) {
    char *Data = _binary_words_bin_start;
    Size64 Size = (Size64)&_binary_words_bin_size;

    WordCount = 0;
    for (Size64 i = 0; i < Size; i++) if (Data[i] == '\n') WordCount++;

    DebugStr("WordCount="); DebugU64(WordCount); DebugChar('\n');
    DebugStr("Size="); DebugU64(Size); DebugChar('\n');
    DebugStr("Data[0]="); DebugChar(Data[0]); DebugChar('\n');

    Size64 alloc_size = WordCount * sizeof(char*);
    DebugStr("Alloc size="); DebugU64(alloc_size); DebugStr(" bytes\n");

    StringList = Alloc(&KernelPool, WordCount * sizeof(char*));
    if (!StringList) {
        DebugStr("StringList Is NULL\n");
        return;
    }

    Size64 idx = 0;
    StringList[idx++] = &Data[0];
    for (Size64 i = 0; i < Size; i++) {
        if (Data[i] == '\n') {
            Data[i] = '\0';
            if (i + 1 < Size && idx < WordCount) {
                StringList[idx++] = &Data[i + 1];
            }
        }
        if (i % 10000 == 0) {
            DebugStr(".");
        }
    }
}

char* RandomString(){
    if (!WordCount) return NULL;
    return StringList[rdrand() % WordCount];
}

char* RandomIDKWhatTheFuckingThisWord(){
    return WordsInFishsBrain[rdrand() % 32];
}

char* RandomQuestions(){
    u64 Idx=((u8)rdrand() % 32);
    if (Idx>=24) {
        const char TmpBufferStack[256]="What This Word Mean In Chinese? : ";
        MemCopy(TmpBuffer,TmpBufferStack,StrLen(TmpBufferStack));
        char* RandomWord=RandomString();
        MemCopy(TmpBuffer+StrLen(TmpBufferStack),RandomWord,StrLen(RandomWord));
        TmpBuffer[StrLen(TmpBufferStack)+StrLen(RandomWord)]='\0';
        return TmpBuffer;
    }
    else return SomeQuestions[rdrand() % 16];
}