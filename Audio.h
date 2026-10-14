#pragma once

#include "int.h"

#define SAMPLE_RATE 48000

extern s16 *GlobalAudioPCM;
extern Size64 GlobalAudioPCMSizeCountOfByte;
extern Size64 GlobalAudioPCMSizeCountOfBytePlay;
extern s16 *GlobalAudioPCMNext;
extern Size64 GlobalAudioPCMNextSizeCountOfByte;
extern Size64 GlobalAudioPCMNextSizeCountOfBytePlay;
extern u16 GlobalAudioPCMRate;

void InitAudioBuff(void);