#pragma once
#include "int.h"

extern const char* Text;

extern char _binary_words_bin_start[];
extern char _binary_words_bin_end[];
extern char _binary_words_bin_size[];

extern char** StringList;
extern Size64 WordCount;

u64 rdrand(void);
char RandomChar();
void InitRandomString();
char* RandomString();
char* RandomIDKWhatTheFuckingThisWord();
char* RandomQuestions();