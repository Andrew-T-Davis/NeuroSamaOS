#include "Audio.h"
#include "debug.h"
#include "kmalloc.h"
#include "int.h"
#include "str.h"

extern AllocPool KernelPool;

s16 *GlobalAudioPCM = NULL;
Size64 GlobalAudioPCMSizeCountOfByte = 0;
Size64 GlobalAudioPCMSizeCountOfBytePlay = 0;
s16 *GlobalAudioPCMNext = NULL;
Size64 GlobalAudioPCMNextSizeCountOfByte = 0;
Size64 GlobalAudioPCMNextSizeCountOfBytePlay = 0;
u16 GlobalAudioPCMRate=48000;

void InitAudioBuff(void) {
    #define BUFFER_TOTAL_BYTES 8388352
    
    GlobalAudioPCM = (s16*)AlignedAlloc(&KernelPool, BUFFER_TOTAL_BYTES, 64);
    GlobalAudioPCMNext = (s16*)AlignedAlloc(&KernelPool, BUFFER_TOTAL_BYTES, 64);
    
    if (!GlobalAudioPCM || !GlobalAudioPCMNext) {
        DebugStr("Failed to allocate audio buffers!\n");
        return;
    }
    
    GlobalAudioPCMSizeCountOfByte = BUFFER_TOTAL_BYTES;
    GlobalAudioPCMNextSizeCountOfByte = BUFFER_TOTAL_BYTES;
    
    u32 total_samples = BUFFER_TOTAL_BYTES / 4;
    
    u32 period = SAMPLE_RATE / 440;
    period = 120;
    
    s16 max_amplitude = 10000;
    
    u32 num_cycles = total_samples / period;
    u32 actual_samples = num_cycles * period;
    
    for (u32 i = 0; i < actual_samples; i++) {
        u32 pos = i % period;
        s16 val;
        if (pos < period / 2) {
            val = (s16)((2 * (s16)max_amplitude * (s16)pos) / (s16)(period / 2) - (s16)max_amplitude);
        } else {
            u32 down_pos = pos - period / 2;
            val = (s16)((s16)max_amplitude - (2 * (s16)max_amplitude * (s16)down_pos) / (s16)(period / 2));
        }
        
        GlobalAudioPCM[i * 2]     = val;
        GlobalAudioPCM[i * 2 + 1] = val;
        GlobalAudioPCMNext[i * 2]     = val;
        GlobalAudioPCMNext[i * 2 + 1] = val;
    }
    MemSet(GlobalAudioPCM, 0, BUFFER_TOTAL_BYTES);
    MemSet(GlobalAudioPCMNext, 0, BUFFER_TOTAL_BYTES);
    
    GlobalAudioPCMSizeCountOfBytePlay = actual_samples * 4; 
    GlobalAudioPCMNextSizeCountOfBytePlay = actual_samples * 4;
    
    __asm__ volatile("sfence" ::: "memory");
    
    DebugStr("Audio buffers initialized with full capacity and seamless loop!\n");
}