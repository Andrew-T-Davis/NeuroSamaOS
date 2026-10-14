#include "AC97.h"
#include "debug.h"
#include "int.h"
#include "kmalloc.h"
#include "pci.h"
#include "port.h"
#include "clock.h"
#include "Audio.h"
#include "str.h"

extern AllocPool KernelPool;

AC97BDLEntry* bdl = NULL;

void PingPongSize(Size64* a, Size64* b)
{
    Size64 tmp = *a;
    *a = *b;
    *b = tmp;
}

u64 AC97MMIOBase(u32* Bar1, u16* DmaPort)
{
    u8 Bus, Slot, Func;
    PCIFindDeviceByClass(0x04, 0x01, 0x00, &Bus, &Slot, &Func);
    if (Slot == 0xFF) {
        DebugStr("AC97 not found!\n");
        return 0;
    }

    u16 cmd = PCIReadWORD(Bus, Slot, Func, 0x04);
    cmd |= 0x0007;
    PCIWriteWORD(Bus, Slot, Func, 0x04, cmd);

    u32 bar0 = PCIReadDWORD((u8)Bus, Slot, Func, 0x10);
    u32 bar1 = PCIReadDWORD((u8)Bus, Slot, Func, 0x14);

    u16 codec_port = bar0 & 0xFFFC;
    u16 dma_port = bar1 & 0xFFFC;

    *Bar1 = bar1 & ~1UL;
    *DmaPort = dma_port;

    return (u64)codec_port;
}

u16 AC97ReadIO(volatile void* Ptr, u16 Offset)
{
    u64 addr = (u64)Ptr + Offset;
    return inw((u16)addr);
}

void AC97WriteIO(volatile void* Ptr, u16 Val, u16 Offset)
{
    u64 addr = (u64)Ptr + Offset;
    outw((u16)addr, Val);
}

AC97Controller* InitAC97(void)
{
    u32 Bar1;
    u16 DmaPort;
    u64 Base = AC97MMIOBase(&Bar1, &DmaPort);
    if (Base == 0) {
        DebugStr("AC97 not found!\n");
        return NULL;
    }

    AC97Controller* AC97 = Alloc(&KernelPool, sizeof(AC97Controller));
    if (!AC97) {
        DebugStr("Alloc AC97 Is NULL\n");
        return NULL;
    }

    AC97->CodecPort = (u16)Base;
    AC97->DmaPort   = DmaPort;
    AC97->Regs      = (AC97Regs*)(u64)Base;

    outl(AC97->DmaPort + 0x2C, 0x00000002);
    SystemBusySleepUs(1000);
    outl(AC97->DmaPort + 0x2C, 0x00000000);
    SystemBusySleepUs(1000);

    u32 Status = inl(AC97->DmaPort + 0x30);
    if (!(Status & (1 << 8))) {
        DebugStr("Error: Codec Ready bit did not come up!\n");
        Free(&KernelPool, AC97);
        return NULL;
    }

    AC97WriteIO(&AC97->Regs->Reset, 0x0000, 0);
    SystemBusySleepUs(1000);

    AC97WriteIO(&AC97->Regs->PowerdownCtrlOrStat, 0x0000, 0);

    AC97WriteIO(&AC97->Regs->MasterVolume, 0x0000, 0);
    AC97WriteIO(&AC97->Regs->HeadphoneVolume, 0x0000, 0);
    AC97WriteIO(&AC97->Regs->PCMOutVol, 0x0000, 0);

    u16 ext_id = AC97ReadIO(&AC97->Regs->ExtendedAudioID, 0);
    if (ext_id & (1 << 0)) {
        u16 ext_ctrl = AC97ReadIO(&AC97->Regs->ExtendedAudioStatOrCtrl, 0);
        ext_ctrl |= (1 << 0);
        AC97WriteIO(&AC97->Regs->ExtendedAudioStatOrCtrl, ext_ctrl, 0);
    }

    outl(AC97->DmaPort + 0x2C, 0x00000002);

    bdl = AlignedAlloc(&KernelPool, sizeof(AC97BDLEntry) * 32, 16);

    ext_id = AC97ReadIO(&AC97->Regs->ExtendedAudioID, 0);
    DebugStr("ExtendedAudioID: "); DebugU16(ext_id); DebugChar('\n');

    u16 ext_ctrl = AC97ReadIO(&AC97->Regs->ExtendedAudioStatOrCtrl, 0);
    ext_ctrl |= (1 << 0);
    AC97WriteIO(&AC97->Regs->ExtendedAudioStatOrCtrl, ext_ctrl, 0);

    AC97WriteIO(&AC97->Regs->PCMFrontDACRate, 48000, 0);

    u16 rate = AC97ReadIO(&AC97->Regs->PCMFrontDACRate, 0);
    DebugStr("PCMFrontDACRate: "); DebugU16(rate); DebugChar('\n');
    if (rate != 48000) {
        DebugStr("WARNING: Sample rate not set correctly!\n");
    }

    ext_ctrl = AC97ReadIO(&AC97->Regs->ExtendedAudioStatOrCtrl, 0);
    ext_ctrl |= (1 << 3);
    AC97WriteIO(&AC97->Regs->ExtendedAudioStatOrCtrl, ext_ctrl, 0);

    DebugStr("AC97 Init Complete!\n");
    return AC97;
}

void AC97StartPlayback(AC97Controller* AC97, u16 Rate)
{
    if (!GlobalAudioPCM || (u32)GlobalAudioPCMSizeCountOfBytePlay == 0) return;

    MemSet(bdl, 0, sizeof(AC97BDLEntry) * 32);

    u8* pcm_ptr = (u8*)GlobalAudioPCM;

    u32 max_bdl_bytes = 0xFFFC;
    u32 total_bytes = (u32)GlobalAudioPCMSizeCountOfBytePlay;
    u32 num_bdls = (total_bytes + max_bdl_bytes - 1) / max_bdl_bytes;
    if (num_bdls > 32) num_bdls = 32;

    u32 bytes_remaining = total_bytes;
    u32 offset = 0;

    for (int i = 0; i < (s32)num_bdls; i++) {
        u32 bytes_this_bdl = (bytes_remaining > max_bdl_bytes) ? max_bdl_bytes : bytes_remaining;
        
        bdl[i].BufferPointer = (u32)(u64)(pcm_ptr + offset);
        bdl[i].BufferLength = (u16)(bytes_this_bdl / 2);
        bdl[i].Flags = 0x8000;
        
        offset += bytes_this_bdl;
        bytes_remaining -= bytes_this_bdl;
    }
    
    if (num_bdls > 0) {
        bdl[num_bdls - 1].Flags = 0x8001;
    }

    __asm__ volatile("sfence" ::: "memory");

    AC97WriteIO(&AC97->Regs->PCMFrontDACRate, (u16)Rate, 0);

    outb(AC97->DmaPort + AC97_PO_CR, 0);
    outw(AC97->DmaPort + AC97_PO_SR, 0x001F);
    outb(AC97->DmaPort + AC97_PO_CR, AC97_PO_CR_RR);
    while (inb(AC97->DmaPort + AC97_PO_CR) & AC97_PO_CR_RR);
    
    outl(AC97->DmaPort + AC97_PO_BDBAR, (u32)(u64)bdl);
    outb(AC97->DmaPort + AC97_PO_LVI, (u8)num_bdls - 1);
    
    outb(AC97->DmaPort + AC97_PO_CR, AC97_PO_CR_RUN | 0x01);

    u32 picb = inl(AC97->DmaPort + AC97_PO_PICB);
    DebugStr("PICB: "); DebugU32(picb); DebugChar('\n');

    u8 lvi = inb(AC97->DmaPort + AC97_PO_LVI);
    u8 civ = inb(AC97->DmaPort + AC97_PO_CIV);
    DebugStr("LVI="); DebugU8(lvi); DebugStr(" CIV="); DebugU8(civ); DebugChar('\n');
    
    DebugStr("AC97 playback started!\n");
}

Bool AC97DMAFinish(AC97Controller* AC97) {
    if (!AC97 || !bdl) return 0;

    u16 sr = inw(AC97->DmaPort + AC97_PO_SR);

    if (sr & 0x001C) {
        outw(AC97->DmaPort + AC97_PO_SR, sr & 0x001F);
        outb(AC97->DmaPort + AC97_PO_CR, 0);

        return 1;
    }

    return 0;
}

void AC97StopPlayback(AC97Controller* AC97)
{
    if (!AC97) return;

    u8 cr = inb(AC97->DmaPort + AC97_PO_CR);
    cr &= ~AC97_PO_CR_RUN;
    outb(AC97->DmaPort + AC97_PO_CR, cr);

    outw(AC97->DmaPort + AC97_PO_SR, 0x001C);

    DebugStr("AC97 Playback Stopped and Memory Freed!\n");
}

void AC97PausePlayback(AC97Controller* AC97)
{
    if (!AC97) return;

    u8 cr = inb(AC97->DmaPort + AC97_PO_CR);
    if (cr & AC97_PO_CR_RUN) {
        cr &= ~AC97_PO_CR_RUN;
        outb(AC97->DmaPort + AC97_PO_CR, cr);
        DebugStr("AC97 Playback Paused!\n");
    }
}

void AC97ResumePlayback(AC97Controller* AC97)
{
    if (!AC97) return;

    u8 cr = inb(AC97->DmaPort + AC97_PO_CR);
    if (!(cr & AC97_PO_CR_RUN)) {
        cr |= AC97_PO_CR_RUN;
        outb(AC97->DmaPort + AC97_PO_CR, cr);
        DebugStr("AC97 Playback Resumed!\n");
    }
}

void AC97SetMasterVolume(AC97Controller* AC97, u8 volume)
{
    if (!AC97) return;

    if (volume > 100) volume = 100;

    u8 attn = (u8)((100 - volume) * 31 / 100);

    u16 val = (u16)((attn << 8) | attn);

    u16 current_reg = AC97ReadIO(&AC97->Regs->MasterVolume, 0);
    if (current_reg & AC97_MUTE_BIT) {
        val |= AC97_MUTE_BIT;
    }

    AC97WriteIO(&AC97->Regs->MasterVolume, val, 0);
    AC97WriteIO(&AC97->Regs->HeadphoneVolume, val, 0);
    DebugStr("AC97 Master Volume Set!\n");
}

void AC97SetPCMVolume(AC97Controller* AC97, u8 volume)
{
    if (!AC97) return;

    if (volume > 100) volume = 100;

    u8 attn = (u8)((100 - volume) * 31 / 100);
    u16 val = (u16)((attn << 8) | attn);

    u16 current_reg = AC97ReadIO(&AC97->Regs->PCMOutVol, 0);
    if (current_reg & AC97_MUTE_BIT) {
        val |= AC97_MUTE_BIT;
    }

    AC97WriteIO(&AC97->Regs->PCMOutVol, val, 0);
    DebugStr("AC97 PCM Volume Set!\n");
}

void AC97SetMute(AC97Controller* AC97, Bool enable_mute)
{
    if (!AC97) return;

    u16 val = AC97ReadIO(&AC97->Regs->MasterVolume, 0);

    if (enable_mute) {
        val |= AC97_MUTE_BIT;
    } else {
        val &= ~AC97_MUTE_BIT;
    }

    AC97WriteIO(&AC97->Regs->MasterVolume, val, 0);
    AC97WriteIO(&AC97->Regs->HeadphoneVolume, val, 0);
    
    if (enable_mute) {
        DebugStr("AC97 Muted!\n");
    } else {
        DebugStr("AC97 Unmuted!\n");
    }
}
