#pragma once

#include "int.h"

typedef struct {
    char ChunkID[4];
    u32 ChunkSize;
    char Format[4];
    char Subchunk1ID[4];
    u32 Subchunk1Size;
    u16 AudioFormat;
    u16 NumChannels;
    u32 SampleRate;
    u32 ByteRate;
    u16 BlockAlign;
    u16 BitsPerSample;
    char Subchunk2ID[4];
    u32 Subchunk2Size;
    s16 Data[];
} __attribute__((packed)) WAVHeader;

extern char _binary_AudioData_bin_start[];
extern char _binary_AudioData_bin_end[];
extern char _binary_AudioData_bin_size[];