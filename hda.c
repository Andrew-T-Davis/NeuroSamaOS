#include "hda.h"
#include "int.h"
#include "kmalloc.h"
#include "debug.h"
#include "clock.h"
#include "Audio.h"
#include "port.h"

extern AllocPool KernelPool;

volatile HDAudioRegSDOBDLBase GlobalAudioBDL[10]={0};

#pragma clang optimize off
static u32 HDAudioPCIReadDWORD(u8 Bus, u8 Slot, u8 Func, u8 Offset) {
    u32 addr = (u32)((1U << 31) | ((u32)Bus << 16) | ((Slot & 0x1FUL) << 11) | 
                     ((Func & 0x07UL) << 8) | (Offset & 0xFCUL));
    outl(0xCF8, addr);
    return inl(0xCFC);
}

static u16 HDAudioPCIReadWORD(u8 Bus, u8 Slot, u8 Func, u8 Offset) {
    u32 dword = HDAudioPCIReadDWORD(Bus, Slot, Func, Offset);
    if (Offset & 0x2) {
        return (dword >> 16) & 0xFFFF;
    }
    return dword & 0xFFFF;
}

static void HDAudioPCIWriteWORD(u8 Bus, u8 Slot, u8 Func, u8 Offset, u16 Value) {
    u32 addr = (u32)((1UL << 31) | ((u32)Bus << 16) | ((Slot & 0x1FUL) << 11) | 
                     ((Func & 0x07UL) << 8) | (Offset & 0xFCUL));
    outl(0xCF8, addr);
    outw(0xCFC, Value);
}

static void HDAudioPCIEnableDevice(u8 Bus, u8 Slot, u8 Func) {
    u16 cmd = HDAudioPCIReadWORD(Bus, Slot, Func, 0x04);
    cmd |= (1 << 0) | (1 << 1) | (1 << 2);
    HDAudioPCIWriteWORD(Bus, Slot, Func, 0x04, cmd);
    DebugStr("HDAudio PCI Command: 0x");
    DebugU16(HDAudioPCIReadWORD(Bus, Slot, Func, 0x04));
    DebugChar('\n');
}

static u64 HDAudioPCIGetBAR(u8 Bus, u8 Slot, u8 Func, int BarIndex) {
    u32 bar_low = HDAudioPCIReadDWORD(Bus, Slot, Func, (u8)(0x10 + BarIndex * 4));
    
    if (bar_low & 0x1) {
        return (u64)(bar_low & ~0x3UL);
    }
    
    if ((bar_low & 0x6) == 0x4) {
        u32 bar_high = HDAudioPCIReadDWORD(Bus, Slot, Func, (u8)(0x10 + (BarIndex + 1) * 4));
        return ((u64)bar_high << 32) | ((u64)bar_low & ~0xFULL);
    }
    
    return (u64)(bar_low & ~0xFULL);
}

u64 HDAudioScanDevices(HDAudioDeviceInfo* devices, int max_devices) {
    u64 count = 0;
    
    DebugStr("Scanning for HDAudio devices...\n");
    
    for (u16 Bus = 0; Bus < 256 && count < (u64)max_devices; Bus++) {
        for (u16 Slot = 0; Slot < 32 && count < (u64)max_devices; Slot++) {
            u32 header = HDAudioPCIReadDWORD((u8)Bus, (u8)Slot, 0, 0x0C);
            u8 header_type = (header >> 16) & 0xFF;
            u16 max_func = (header_type & 0x80) ? 8 : 1;
            
            for (u16 Func = 0; Func < max_func && count < (u64)max_devices; Func++) {
                u16 vendor = HDAudioPCIReadWORD((u8)Bus, (u8)Slot, (u8)Func, 0x00);
                if (vendor == 0xFFFF) {
                    if (Func == 0) break;
                    continue;
                }
                
                u32 class_rev = HDAudioPCIReadDWORD((u8)Bus, (u8)Slot, (u8)Func, 0x08);
                u8 class_code = (class_rev >> 24) & 0xFF;
                u8 subclass = (class_rev >> 16) & 0xFF;
                u8 prog_if = (class_rev >> 8) & 0xFF;
                
                if (class_code == 0x04 && subclass == 0x03 && prog_if == 0x00) {
                    u16 device = HDAudioPCIReadWORD((u8)Bus, (u8)Slot, (u8)Func, 0x02);
                    u64 bar0 = HDAudioPCIGetBAR((u8)Bus, (u8)Slot, (u8)Func, 0);
                    
                    devices[count].Bus = (u8)Bus;
                    devices[count].Slot = (u8)Slot;
                    devices[count].Func = (u8)Func;
                    devices[count].Vendor = vendor;
                    devices[count].Device = device;
                    devices[count].Bar0 = bar0;
                    count++;
                    
                    DebugStr("  HDAudio #");
                    DebugU8((u8)count);
                    DebugStr(": Bus=");
                    DebugU8((u8)Bus);
                    DebugStr(" Slot=");
                    DebugU8((u8)Slot);
                    DebugStr(" Func=");
                    DebugU8((u8)Func);
                    DebugStr(" VID=0x");
                    DebugU16(vendor);
                    DebugStr(" DID=0x");
                    DebugU16(device);
                    DebugStr(" BAR0=0x");
                    DebugU64(bar0);
                    DebugChar('\n');
                }
            }
        }
    }
    
    DebugStr("Total HDAudio devices: ");
    DebugU8((u8)count);
    DebugChar('\n');
    
    return count;
}

u64 HDAudioGetMMIOByIndex(u64 index) {
    HDAudioDeviceInfo devices[4];
    u64 count = HDAudioScanDevices(devices, 4);
    
    if (count == 0) {
        DebugStr("No HDAudio devices found!\n");
        return 0;
    }
    
    if (index >= count) {
        DebugStr("HDAudio index ");
        DebugU8((u8)index);
        DebugStr(" out of range (0-");
        DebugU8((u8)count - 1);
        DebugStr(")\n");
        return 0;
    }
    
    HDAudioPCIEnableDevice(devices[index].Bus, devices[index].Slot, devices[index].Func);
    
    return devices[index].Bar0;
}

u64 HDAudioMMIO(){
    return (u64)HDAudioGetMMIOByIndex(0);
}


Bool HDAudioAddCORB(HDAudio *NewHDAudio, u8 ValidCodecAddr, u8 NodeId, u16 VerbId, u8 Parameter){
    if (!NewHDAudio) return 0;
    if (NewHDAudio->DmaAlloc.CorbWritePtr->rwWritePtr+1 >= NewHDAudio->NumCORB) return 0;
    NewHDAudio->Regs->RIRBCTL.U = 0;
    NewHDAudio->Regs->RINTCNT.rwResponseInterruptCount++;
    NewHDAudio->Regs->RIRBCTL.rwDMAEnable=1;
    NewHDAudio->Regs->RIRBCTL.rwResponseInterruptControl=1;
    NewHDAudio->DmaAlloc.Corb[NewHDAudio->DmaAlloc.CorbWritePtr->rwWritePtr].rwCodecAddr = ValidCodecAddr&0x0F;
    NewHDAudio->DmaAlloc.Corb[NewHDAudio->DmaAlloc.CorbWritePtr->rwWritePtr].rwNodeId = NodeId;
    NewHDAudio->DmaAlloc.Corb[NewHDAudio->DmaAlloc.CorbWritePtr->rwWritePtr].rwVerbId = VerbId&0xFFF;
    NewHDAudio->DmaAlloc.Corb[NewHDAudio->DmaAlloc.CorbWritePtr->rwWritePtr].rwParameter = Parameter;
    NewHDAudio->DmaAlloc.CorbWritePtr->rwWritePtr++;
    return 1;
}

Bool HDAudioAddCORBList(HDAudio *NewHDAudio, u8 ValidCodecAddr[], u8 NodeId[], u16 VerbId[], u8 Parameter[], Size64 Size){
    if (!NewHDAudio) return 0;
    if (NewHDAudio->DmaAlloc.CorbWritePtr->rwWritePtr+Size+(Size/8) >= NewHDAudio->NumCORB || ((((u16)NewHDAudio->Regs->RINTCNT.rwResponseInterruptCount)+Size+(Size/8))>255)) return 0;
    NewHDAudio->Regs->RIRBCTL.U = 0;
    NewHDAudio->Regs->RINTCNT.rwResponseInterruptCount+=Size+(Size/8)+1;
    NewHDAudio->Regs->RIRBCTL.rwDMAEnable=1;
    NewHDAudio->Regs->RIRBCTL.rwResponseInterruptControl=1;
    for (u64 Idx=0;Idx<Size;Idx++){
        NewHDAudio->DmaAlloc.Corb[NewHDAudio->DmaAlloc.CorbWritePtr->rwWritePtr].rwCodecAddr = ValidCodecAddr[Idx]&0x0F;
        NewHDAudio->DmaAlloc.Corb[NewHDAudio->DmaAlloc.CorbWritePtr->rwWritePtr].rwNodeId = NodeId[Idx];
        NewHDAudio->DmaAlloc.Corb[NewHDAudio->DmaAlloc.CorbWritePtr->rwWritePtr].rwVerbId = VerbId[Idx]&0xFFF;
        NewHDAudio->DmaAlloc.Corb[NewHDAudio->DmaAlloc.CorbWritePtr->rwWritePtr].rwParameter = Parameter[Idx];
        NewHDAudio->DmaAlloc.CorbWritePtr->rwWritePtr++;
        if ((Idx%8)==0){
            NewHDAudio->DmaAlloc.Corb[NewHDAudio->DmaAlloc.CorbWritePtr->rwWritePtr].rwCodecAddr = 0x0;
            NewHDAudio->DmaAlloc.Corb[NewHDAudio->DmaAlloc.CorbWritePtr->rwWritePtr].rwNodeId = 0x0;
            NewHDAudio->DmaAlloc.Corb[NewHDAudio->DmaAlloc.CorbWritePtr->rwWritePtr].rwVerbId = 0x0;
            NewHDAudio->DmaAlloc.Corb[NewHDAudio->DmaAlloc.CorbWritePtr->rwWritePtr].rwParameter = 0x0;
            NewHDAudio->DmaAlloc.CorbWritePtr->rwWritePtr++;
        }
    }
    if (NewHDAudio->DmaAlloc.CorbWritePtr->rwWritePtr+1 <= NewHDAudio->NumCORB){
        NewHDAudio->DmaAlloc.Corb[NewHDAudio->DmaAlloc.CorbWritePtr->rwWritePtr].rwCodecAddr = 0x0;
        NewHDAudio->DmaAlloc.Corb[NewHDAudio->DmaAlloc.CorbWritePtr->rwWritePtr].rwNodeId = 0x0;
        NewHDAudio->DmaAlloc.Corb[NewHDAudio->DmaAlloc.CorbWritePtr->rwWritePtr].rwVerbId = 0x0;
        NewHDAudio->DmaAlloc.Corb[NewHDAudio->DmaAlloc.CorbWritePtr->rwWritePtr].rwParameter = 0x0;
        NewHDAudio->DmaAlloc.CorbWritePtr->rwWritePtr++;
    }
    return 1;
}

void HDAudioDiscoverCodec(HDAudio* hda, u64 codec_addr) {
    HDAudioAddCORB(hda, (u8)codec_addr, 0x00, 0xF00, 0x00);
    HDAudioAddCORB(hda, (u8)codec_addr, 0x00, 0xF20, 0x00);
    HDAudioAddCORB(hda, (u8)codec_addr, 0x00, 0xF10, 0x00);
}

#pragma clang optimize off
HDAudio* HDAudioInit(){
    u64 HDAudioBase = HDAudioMMIO();
    if (!HDAudioBase){
        DebugStr("Cannot Found HDAudio.\n");
        return NULL;
    }else {
        DebugStr("HDAudio MMIO:");DebugU64(HDAudioBase);DebugChar('\n');
    }

    HDAudio* NewHDAudio = AlignedAlloc(&KernelPool, sizeof(HDAudio), 64);
    if (!NewHDAudio) {
        DebugStr("ERROR: Failed to allocate HDAudio struct\n");
        return NULL;
    }

    DebugStr("Wassup Bro\n");

    NewHDAudio->Bar0=HDAudioBase;
    NewHDAudio->Regs=(HDAudioRegs*)((u64*)NewHDAudio->Bar0);
    NewHDAudio->State=HDA_STATE_UNINITIALIZED;
    NewHDAudio->DmaAlloc.CorbWritePtr=&NewHDAudio->Regs->CORBWritePtr;
    NewHDAudio->DmaAlloc.RirbWritePtr=&NewHDAudio->Regs->RIRBWritePtr;
    NewHDAudio->CodecVendorId=0;
    NewHDAudio->CodecDeviceId=0;

    DebugStr("Debug: Print All Reg Offset:\n");
    DebugStr("GCAP Start:");DebugU64((u64)&NewHDAudio->Regs->GCAP);DebugChar(':');DebugChar('\n');
    DebugStr("\t{ GCAP:");DebugU16((u16)(((u64)&NewHDAudio->Regs->GCAP)-((u64)&NewHDAudio->Regs->GCAP)));
    DebugStr(", VMIN:");DebugU16((u16)(((u64)&NewHDAudio->Regs->VMIN)-((u64)&NewHDAudio->Regs->GCAP)));
    DebugStr(", VMAJ:");DebugU16((u16)(((u64)&NewHDAudio->Regs->VMAJ)-((u64)&NewHDAudio->Regs->GCAP)));
    DebugStr(", OUTPAY:");DebugU16((u16)(((u64)&NewHDAudio->Regs->OUTPAY)-((u64)&NewHDAudio->Regs->GCAP)));DebugChar('\n');
    DebugStr("\t, INPAY:");DebugU16((u16)(((u64)&NewHDAudio->Regs->INPAY)-((u64)&NewHDAudio->Regs->GCAP)));
    DebugStr(", GCTL:");DebugU16((u16)(((u64)&NewHDAudio->Regs->GCTL)-((u64)&NewHDAudio->Regs->GCAP)));
    DebugStr(", WAKEEN:");DebugU16((u16)(((u64)&NewHDAudio->Regs->WAKEEN)-((u64)&NewHDAudio->Regs->GCAP)));
    DebugStr(", STATESTS:");DebugU16((u16)(((u64)&NewHDAudio->Regs->STATESTS)-((u64)&NewHDAudio->Regs->GCAP)));DebugChar('\n');
    DebugStr("\t, GSTS:");DebugU16((u16)(((u64)&NewHDAudio->Regs->GSTS)-((u64)&NewHDAudio->Regs->GCAP)));
    DebugStr(", OUTSTRMPAY:");DebugU16((u16)(((u64)&NewHDAudio->Regs->OUTSTRMPAY)-((u64)&NewHDAudio->Regs->GCAP)));
    DebugStr(", INSTRMPAY:");DebugU16((u16)(((u64)&NewHDAudio->Regs->INSTRMPAY)-((u64)&NewHDAudio->Regs->GCAP)));
    DebugStr(", INTCTL:");DebugU16((u16)(((u64)&NewHDAudio->Regs->INTCTL)-((u64)&NewHDAudio->Regs->GCAP)));DebugChar('\n');
    DebugStr("\t, INTSTS:");DebugU16((u16)(((u64)&NewHDAudio->Regs->INTSTS)-((u64)&NewHDAudio->Regs->GCAP)));
    DebugStr(", WALCLK:");DebugU16((u16)(((u64)&NewHDAudio->Regs->WALCLK)-((u64)&NewHDAudio->Regs->GCAP)));
    DebugStr(", SSYNC:");DebugU16((u16)(((u64)&NewHDAudio->Regs->SSYNC)-((u64)&NewHDAudio->Regs->GCAP)));
    DebugStr(", CORBBase:");DebugU16((u16)(((u64)&NewHDAudio->Regs->CORBBase)-((u64)&NewHDAudio->Regs->GCAP)));DebugChar('\n');
    DebugStr("\t, CORBWritePtr:");DebugU16((u16)(((u64)&NewHDAudio->Regs->CORBWritePtr)-((u64)&NewHDAudio->Regs->GCAP)));
    DebugStr(", CORBReadPtr:");DebugU16((u16)(((u64)&NewHDAudio->Regs->CORBReadPtr)-((u64)&NewHDAudio->Regs->GCAP)));
    DebugStr(", CORBCTL:");DebugU16((u16)(((u64)&NewHDAudio->Regs->CORBCTL)-((u64)&NewHDAudio->Regs->GCAP)));
    DebugStr(", CORBSTS:");DebugU16((u16)(((u64)&NewHDAudio->Regs->CORBSTS)-((u64)&NewHDAudio->Regs->GCAP)));DebugChar('\n');
    DebugStr("\t, CORBSize:");DebugU16((u16)(((u64)&NewHDAudio->Regs->CORBSize)-((u64)&NewHDAudio->Regs->GCAP)));
    DebugStr(", RIRBBase:");DebugU16((u16)(((u64)&NewHDAudio->Regs->RIRBBase)-((u64)&NewHDAudio->Regs->GCAP)));
    DebugStr(", RIRBWritePtr:");DebugU16((u16)(((u64)&NewHDAudio->Regs->RIRBWritePtr)-((u64)&NewHDAudio->Regs->GCAP)));
    DebugStr(", RIRBCTL:");DebugU16((u16)(((u64)&NewHDAudio->Regs->RIRBCTL)-((u64)&NewHDAudio->Regs->GCAP)));DebugChar('\n');
    DebugStr("\t, RIRBSTS:");DebugU16((u16)(((u64)&NewHDAudio->Regs->RIRBSTS)-((u64)&NewHDAudio->Regs->GCAP)));
    DebugStr(", RIRBSize:");DebugU16((u16)(((u64)&NewHDAudio->Regs->RIRBSize)-((u64)&NewHDAudio->Regs->GCAP)));
    DebugStr(", ICOI:");DebugU16((u16)(((u64)&NewHDAudio->Regs->ICOI)-((u64)&NewHDAudio->Regs->GCAP)));
    DebugStr(", ICII:");DebugU16((u16)(((u64)&NewHDAudio->Regs->ICII)-((u64)&NewHDAudio->Regs->GCAP)));DebugChar('\n');
    DebugStr("\t, ICIS:");DebugU16((u16)(((u64)&NewHDAudio->Regs->ICIS)-((u64)&NewHDAudio->Regs->GCAP)));
    DebugStr(", DPBase:");DebugU16((u16)(((u64)&NewHDAudio->Regs->DPBase)-((u64)&NewHDAudio->Regs->GCAP)));;
    DebugStr(", Channels:");DebugU16((u16)(((u64)&NewHDAudio->Regs->Channels)-((u64)&NewHDAudio->Regs->GCAP)));
    DebugStr(", WALCLKA:");DebugU16((u16)(((u64)&NewHDAudio->Regs->WALCLKA)-((u64)&NewHDAudio->Regs->GCAP)));DebugChar('\n');
    DebugStr("\t, LPIBA:");DebugU16((u16)(((u64)&NewHDAudio->Regs->LPIBA)-((u64)&NewHDAudio->Regs->GCAP)));DebugChar(' ');DebugChar('}');
    DebugChar('\n');DebugChar('\n');

    DebugStr("Before Wake Up,GCTL.rwsControllerReset:");DebugU32(NewHDAudio->Regs->GCTL.rwsControllerReset);DebugChar('\n');
    NewHDAudio->Regs->GCTL.rwsControllerReset=0b0;
    while(NewHDAudio->Regs->GCTL.rwsControllerReset) __asm__ volatile("pause\n\t");
    NewHDAudio->Regs->GCTL.rwsControllerReset=0b1;
    while(!NewHDAudio->Regs->GCTL.rwsControllerReset) __asm__ volatile("pause\n\t");
    DebugStr("After Wake Up, Wait 727us(727pp!WYSI(When you see it)!!!!!)\n");
    SystemBusySleepUs(727);
    DebugStr("After Wake Up,GCTL.rwsControllerReset:");DebugU32(NewHDAudio->Regs->GCTL.rwsControllerReset);DebugChar('\n');
    NewHDAudio->Regs->GCTL.rwFlushControl=1;
    while (NewHDAudio->Regs->GSTS.U & 0x01) {
        __asm__ volatile("pause");
    }
    DebugStr("After Wake Up,Set 1 GCTL.rwFlushControl:");DebugU32(NewHDAudio->Regs->GCTL.rwFlushControl);DebugChar('\n');
    DebugStr("GCTL:");DebugU32(NewHDAudio->Regs->GCTL.U);DebugChar('\n');
    if (NewHDAudio->Regs->STATESTS.U == 0) NewHDAudio->Regs->STATESTS.U = 0xFFFF;
    DebugStr("STATESTS:");DebugU64(NewHDAudio->Regs->STATESTS.U);DebugChar('\n');
    DebugStr("WAKEEN:");DebugU64(NewHDAudio->Regs->WAKEEN.U);DebugChar('\n');
    DebugStr("GCAP:\n");
    DebugStr("\t.roOSSCount:");DebugU16(NewHDAudio->Regs->GCAP.roOSSCount);DebugChar('\n');
    DebugStr("\t.roISSCount:");DebugU16(NewHDAudio->Regs->GCAP.roISSCount);DebugChar('\n');
    DebugStr("\t.roBSSCount:");DebugU16(NewHDAudio->Regs->GCAP.roBSSCount);DebugChar('\n');
    DebugStr("\t.roSDOLineCount:");DebugU16(NewHDAudio->Regs->GCAP.roSDOLineCount);DebugChar('\n');
    DebugStr("\t.roHave64bit:");DebugU16(NewHDAudio->Regs->GCAP.roHave64bit);DebugChar('\n');
    DebugStr("Version(JMAJ.VMIN): ");DebugU8(NewHDAudio->Regs->VMAJ.U);DebugChar('.');DebugU8(NewHDAudio->Regs->VMIN.U);DebugChar('\n');

    NewHDAudio->NumStreams=NewHDAudio->Regs->GCAP.roSDOLineCount;

    NewHDAudio->Regs->CORBCTL.rwEnableDMAEngine=0b0;
    NewHDAudio->Regs->RIRBCTL.rwDMAEnable=0b0;

    NewHDAudio->NumCORB=
        NewHDAudio->Regs->CORBSize.roSize==CORB_2Entry ? 2 :
        NewHDAudio->Regs->CORBSize.roSize==CORB_16Entry ? 16 :
        NewHDAudio->Regs->CORBSize.roSize==CORB_256Entry ? 256 :
        CORB_NoWayBro_Its_RESERVED_And_ERROR
    ;

    NewHDAudio->NumRIRB=
        NewHDAudio->Regs->RIRBSize.roSize==CORB_2Entry ? 2 :
        NewHDAudio->Regs->RIRBSize.roSize==CORB_16Entry ? 16 :
        NewHDAudio->Regs->RIRBSize.roSize==CORB_256Entry ? 256 :
        CORB_NoWayBro_Its_RESERVED_And_ERROR
    ;

    DebugStr("Num CORB:");DebugU16(NewHDAudio->NumCORB);DebugChar('\n');
    DebugStr("Num RIRB:");DebugU16(NewHDAudio->NumRIRB);DebugChar('\n');

    NewHDAudio->DmaAlloc.Corb=AlignedAlloc(&KernelPool,NewHDAudio->NumCORB*sizeof(HDAudioCorbEntry),128);
    NewHDAudio->DmaAlloc.Rirb=AlignedAlloc(&KernelPool,NewHDAudio->NumRIRB*sizeof(HDAudioRirbEntry),128);
    u64 DPBase=(u64)AlignedAlloc(&KernelPool,4096,128);
    NewHDAudio->Regs->DPBase.U=DPBase;
    NewHDAudio->Regs->DPBase.rsvdzRsvd1=0;
    NewHDAudio->Regs->DPBase.rwPositionBufferEnable=0b1;

    DebugStr("Alloc CORB:");DebugU64((u64)NewHDAudio->DmaAlloc.Corb);DebugChar('\n');
    DebugStr("Alloc RIRB:");DebugU64((u64)NewHDAudio->DmaAlloc.Rirb);DebugChar('\n');
    DebugStr("Alloc DP:");DebugU64(((u64)NewHDAudio->Regs->DPBase.rwHighBase<<32)|((u64)NewHDAudio->Regs->DPBase.rwLowBase<<7));DebugChar('\n');

    NewHDAudio->Regs->CORBBase.U=(u64)NewHDAudio->DmaAlloc.Corb;
    NewHDAudio->Regs->RIRBBase.U=(u64)NewHDAudio->DmaAlloc.Rirb;

    NewHDAudio->Regs->CORBCTL.rwMemoryErrorInterruptEnable=0b0;
    NewHDAudio->Regs->RIRBCTL.rwResponseInterruptControl=0b0;

    NewHDAudio->Regs->RIRBWritePtr.rwWritePtrReset=0b1;
    SystemBusySleepUs(10);
    NewHDAudio->Regs->RIRBWritePtr.rwWritePtrReset=0b0;

    u32 WaitTimeout = 100000;
    while (NewHDAudio->Regs->RIRBWritePtr.roWritePtr != 0 && --WaitTimeout) {
        __asm__ volatile("pause");
    }
    if (WaitTimeout == 0) {
        DebugStr("RIRB reset Timeout, WP="); DebugU16(NewHDAudio->Regs->RIRBWritePtr.roWritePtr); DebugChar('\n');
    }

    for (u16 i = 0; i < NewHDAudio->NumRIRB; i++) {
        NewHDAudio->DmaAlloc.Rirb[i].U = 0;
    }
    __asm__ volatile("sfence" ::: "memory");

    DebugStr("After reset, RIRBWP: "); DebugU16(NewHDAudio->Regs->RIRBWritePtr.roWritePtr); DebugChar('\n');

    NewHDAudio->Regs->CORBReadPtr.rwReset=0b1;
    while (!(NewHDAudio->Regs->CORBReadPtr.U & 0x8000)) {
        __asm__ volatile("pause");
    }
    NewHDAudio->Regs->CORBReadPtr.rwReset=0b0;
    while (NewHDAudio->Regs->CORBReadPtr.U & 0x8000) {
        __asm__ volatile("pause");
    }
    NewHDAudio->Regs->CORBWritePtr.rwWritePtr=0;

    DebugStr("CORBBase:");DebugU64(NewHDAudio->Regs->CORBBase.U);DebugChar('\n');
    DebugStr("RIRBBase:");DebugU64(NewHDAudio->Regs->RIRBBase.U);DebugChar('\n');
    DebugStr("DPBase:");DebugU64(((u64)NewHDAudio->Regs->DPBase.rwHighBase<<32)|((u64)NewHDAudio->Regs->DPBase.rwLowBase<<7));DebugChar('\n');

    DebugStr("RIRBWP: ");DebugU16(NewHDAudio->Regs->RIRBWritePtr.roWritePtr);DebugChar('\n');

    for (u16 i = 0; i < NewHDAudio->NumRIRB; i++) {
        NewHDAudio->DmaAlloc.Rirb[i].U = 0;
    }
    __asm__ volatile("sfence" ::: "memory");
    NewHDAudio->Regs->RIRBCTL.U = 0;
    NewHDAudio->Regs->CORBCTL.U = 0;

    NewHDAudio->Regs->RIRBCTL.rwDMAEnable=1;
    NewHDAudio->Regs->RIRBCTL.rwResponseInterruptControl=1;
    NewHDAudio->Regs->CORBCTL.rwEnableDMAEngine=1;

    DebugStr("DMA+INT Enabled - CORBCTL:"); DebugU8(NewHDAudio->Regs->CORBCTL.U);
    DebugStr(" RIRBCTL:"); DebugU8(NewHDAudio->Regs->RIRBCTL.U); DebugChar('\n');

    NewHDAudio->Regs->RIRBSTS.U = 0xFF;

    u16 StateSTS = NewHDAudio->Regs->STATESTS.U & 0xFFFF;
    u8 ValidCodecAddr = 0;
    for (u8 i = 0; i < 16; i++) {
        if (StateSTS & (1 << i)) {
            ValidCodecAddr = i;
            break;
        }
    }
    DebugStr("Valid Codec: "); DebugU8(ValidCodecAddr); DebugChar('\n');

    if (NewHDAudio->Regs->RIRBWritePtr.roWritePtr != 0) {
        DebugStr("Dropping existing RIRB response, WP="); 
        DebugU16(NewHDAudio->Regs->RIRBWritePtr.roWritePtr); 
        DebugChar('\n');
        u16 DropIdx = (NewHDAudio->Regs->RIRBWritePtr.roWritePtr - 1) & (NewHDAudio->NumRIRB - 1);
        NewHDAudio->DmaAlloc.Rirb[DropIdx].U = 0;
        NewHDAudio->Regs->RIRBWritePtr.rwWritePtrReset = 1;
        SystemBusySleepUs(10);
        NewHDAudio->Regs->RIRBWritePtr.rwWritePtrReset = 0;
        NewHDAudio->Regs->RIRBSTS.U = 0xFF;
        DebugStr("After clearing, RIRBWP: "); 
        DebugU16(NewHDAudio->Regs->RIRBWritePtr.roWritePtr); 
        DebugChar('\n');
    }

    SystemBusySleepUs(100);
    NewHDAudio->DmaAlloc.CorbWritePtr->rwWritePtr=1;

    u16 OldRirb = NewHDAudio->DmaAlloc.RirbWritePtr->roWritePtr;

    HDAudioAddCORB(NewHDAudio, ValidCodecAddr, 0x00, 0xF00, 0x00);

    __asm__ volatile("sfence" ::: "memory");

    DebugStr("Send Corb\n");
    DebugStr("CORBWP before wait: "); DebugU16(NewHDAudio->Regs->CORBWritePtr.rwWritePtr); DebugChar('\n');
    DebugStr("RIRBWP before wait: "); DebugU16(NewHDAudio->Regs->RIRBWritePtr.roWritePtr); DebugChar('\n');
    DebugStr("CORBCTL: "); DebugU8(NewHDAudio->Regs->CORBCTL.U); DebugChar('\n');
    DebugStr("RIRBCTL: "); DebugU8(NewHDAudio->Regs->RIRBCTL.U); DebugChar('\n');
    DebugStr("CORBSTS: "); DebugU8(NewHDAudio->Regs->CORBSTS.U); DebugChar('\n');
    DebugStr("RIRBSTS: "); DebugU8(NewHDAudio->Regs->RIRBSTS.U); DebugChar('\n');
    DebugStr("RINTCNT: "); DebugU16(NewHDAudio->Regs->RINTCNT.U); DebugChar('\n');
    DebugStr("CORBWP full: ");
    DebugU16(*(volatile u16*)((u64)NewHDAudio->Bar0 + 0x48));
    DebugChar('\n');
    DebugStr("CORBRP full: ");
    DebugU16(*(volatile u16*)((u64)NewHDAudio->Bar0 + 0x4A));
    DebugChar('\n');

    u32 Timeout = 1000000;
    while (OldRirb == NewHDAudio->Regs->RIRBWritePtr.roWritePtr){
        __asm__ volatile("pause\n\t");
        if (--Timeout == 0) {
            DebugStr("TIMEOUT! RIRBCTL="); DebugU8(NewHDAudio->Regs->RIRBCTL.U);
            DebugStr(" RIRBSTS="); DebugU8(NewHDAudio->Regs->RIRBSTS.U);
            DebugStr(" CORBRP="); DebugU16(NewHDAudio->Regs->CORBReadPtr.roReadPtr);
            DebugChar('\n');
            break;
        }
    }

    if (Timeout > 0) {
        u16 RirbWp = NewHDAudio->Regs->RIRBWritePtr.roWritePtr;
        DebugStr("RirbWp: ");DebugU16(RirbWp);DebugChar('\n');
        u16 idx = RirbWp;
        DebugStr("Get RIRB\n");
        DebugStr("RIRB Get:");DebugU64(NewHDAudio->DmaAlloc.Rirb[idx].U);DebugChar('\n');
        DebugStr("RIRB:\n\t.roCodecAddr:");DebugU32(NewHDAudio->DmaAlloc.Rirb[idx].roCodecAddr);
        DebugStr("\n\t.roRsvdEX:");DebugU32(NewHDAudio->DmaAlloc.Rirb[idx].roRsvdEX);
        DebugStr("\n\t.roUnsolicited:");DebugU32(NewHDAudio->DmaAlloc.Rirb[idx].roUnsolicited);
        DebugStr("\n\t.roResponse:");DebugU32(NewHDAudio->DmaAlloc.Rirb[idx].roResponse);
        DebugStr("\n\t.roResponseEx:");DebugU32(NewHDAudio->DmaAlloc.Rirb[idx].roResponseEx);DebugChar('\n');
    } else {
        DebugStr("Failed to receive RIRB response.\n");
    }

    return NewHDAudio;
}

void HDAudioPlay(HDAudio* HDAudio, Bool Double) {
    if (HDAudio->NumStreams == 0) {
        DebugStr("No Stream.\n");
        return;
    }
    if (GlobalAudioPCMSizeCountOfBytePlay == 0) {
        DebugStr("Size=0.\n");
        return;
    }

    u8 CodecAddr = 0;
    u8 OutConv = 0;
    u8 OutPin = 0;
    u8 ConvNodes[16];
    u8 PinNodes[16];
    int ConvCount = 0;
    int PinCount = 0;

    u16 Old = HDAudio->Regs->RIRBWritePtr.roWritePtr;
    HDAudioAddCORB(HDAudio, CodecAddr, 0, 0xF00, 0x00);
    u32 Timeout = 1000000;
    while (Old == HDAudio->Regs->RIRBWritePtr.roWritePtr){
        __asm__ volatile("pause\n\t");
        if (--Timeout == 0) {
            DebugStr("TIMEOUT! RIRBCTL="); DebugU8(HDAudio->Regs->RIRBCTL.U);
            DebugStr(" RIRBSTS="); DebugU8(HDAudio->Regs->RIRBSTS.U);
            DebugStr(" CORBRP="); DebugU16(HDAudio->Regs->CORBReadPtr.roReadPtr);
            DebugChar('\n');
            break;
        }
    }
    u32 VendorId = HDAudio->DmaAlloc.Rirb[HDAudio->Regs->RIRBWritePtr.roWritePtr].roResponse;
    DebugStr("Codec Vendor ID: 0x"); DebugU32(VendorId); DebugChar('\n');

    Old = HDAudio->Regs->RIRBWritePtr.roWritePtr;
    HDAudioAddCORB(HDAudio, CodecAddr, 0, 0xF00, 0x09);
    Timeout = 1000000;
    while (Old == HDAudio->Regs->RIRBWritePtr.roWritePtr){
        __asm__ volatile("pause\n\t");
        if (--Timeout == 0) {
            DebugStr("TIMEOUT! RIRBCTL="); DebugU8(HDAudio->Regs->RIRBCTL.U);
            DebugStr(" RIRBSTS="); DebugU8(HDAudio->Regs->RIRBSTS.U);
            DebugStr(" CORBRP="); DebugU16(HDAudio->Regs->CORBReadPtr.roReadPtr);
            DebugChar('\n');
            break;
        }
    }
    u32 FgType = HDAudio->DmaAlloc.Rirb[HDAudio->Regs->RIRBWritePtr.roWritePtr].roResponse;
    DebugStr("FgType RAW: 0x"); DebugU32(FgType); DebugChar('\n');

    DebugStr("  byte3: "); DebugU8((FgType >> 24) & 0xFF); DebugChar('\n');
    DebugStr("  byte2: "); DebugU8((FgType >> 16) & 0xFF); DebugChar('\n');
    DebugStr("  byte1: "); DebugU8((FgType >> 8) & 0xFF);  DebugChar('\n');
    DebugStr("  byte0: "); DebugU8(FgType & 0xFF);         DebugChar('\n');

    u8 NumNodes = (FgType >> 24) & 0xFF;
    DebugStr("Number of nodes: "); DebugU8(NumNodes); DebugChar('\n');

    Old = HDAudio->Regs->RIRBWritePtr.roWritePtr;
    HDAudioAddCORB(HDAudio, CodecAddr, 0, 0xF20, 0x00);
    Timeout = 1000000;
    while (Old == HDAudio->Regs->RIRBWritePtr.roWritePtr){
        __asm__ volatile("pause");
        if (--Timeout == 0) break;
    }

    if (Timeout > 0) {
        u16 Idx = HDAudio->Regs->RIRBWritePtr.roWritePtr;
        u32 NodeInfo = HDAudio->DmaAlloc.Rirb[Idx+1].roResponse;
        DebugStr("Node Info (F20): 0x"); DebugU32(NodeInfo); DebugChar('\n');
        NumNodes = (NodeInfo >> 16) & 0xFF;
        DebugStr("Nodes: "); DebugU8(NumNodes); DebugChar('\n');
    }

    DebugStr("Enumerating widgets...\n");
    
    for (u8 Node = 1; Node <= NumNodes && Node < 255; Node++) {
        Old = HDAudio->Regs->RIRBWritePtr.roWritePtr;
        HDAudioAddCORB(HDAudio, CodecAddr, Node, 0xF00, 0x09);
        Timeout = 1000000;
        while (HDAudio->Regs->RIRBWritePtr.roWritePtr == Old) {
            __asm__ volatile("pause");
            if (--Timeout == 0) break;
        }
        if (Timeout == 0) continue;
        
        u16 Idx = HDAudio->Regs->RIRBWritePtr.roWritePtr;
        u32 Resp = HDAudio->DmaAlloc.Rirb[Idx].roResponse;
        if (Resp == 0) continue;
        
        u8 Type = (Resp >> 20) & 0x0F;
        
        DebugStr("  NID "); DebugU8(Node); 
        DebugStr(" Type="); DebugU8(Type);
        if (Type == 0) DebugStr(" (Output Converter)");
        else if (Type == 1) DebugStr(" (Input Converter)");
        else if (Type == 4) DebugStr(" (Pin Widget)");
        DebugChar('\n');
        
        if (Type == 0) {
            ConvNodes[ConvCount++] = Node;
            if (OutConv == 0) OutConv = Node;
            DebugStr("Found Out\n");
        }
        if (Type == 4) {
            PinNodes[PinCount++] = Node;
            DebugStr("Found Pin\n");
        }
    }

    if (OutConv == 0 || PinCount == 0) {
        DebugStr("ERROR: No Output Converter or Pin Widget found!\n");
        return;
    }

    DebugStr("\nSearching for Pin connected to Output Converter...\n");
    for (int i = 0; i < PinCount; i++) {
        u8 Pin = PinNodes[i];
        
        Old = HDAudio->Regs->RIRBWritePtr.roWritePtr;
        HDAudioAddCORB(HDAudio, CodecAddr, Pin, 0xF00, 0x0E);
        Timeout = 1000000;
        while (HDAudio->Regs->RIRBWritePtr.roWritePtr == Old) {
            __asm__ volatile("pause");
            if (--Timeout == 0) break;
        }
        if (Timeout == 0) continue;
        
        u16 Idx = HDAudio->Regs->RIRBWritePtr.roWritePtr;
        u32 LenResp = HDAudio->DmaAlloc.Rirb[Idx].roResponse;
        u8 ConnLen = LenResp & 0x7F;
        
        for (u8 entry = 0; entry < ConnLen && entry < 8; entry++) {
            u16 Old2 = HDAudio->Regs->RIRBWritePtr.roWritePtr;
            HDAudioAddCORB(HDAudio, CodecAddr, Pin, 0xF02, entry);
            Timeout = 1000000;
            while (HDAudio->Regs->RIRBWritePtr.roWritePtr == Old2) {
                __asm__ volatile("pause");
                if (--Timeout == 0) break;
            }
            if (Timeout == 0) continue;
            
            u16 Idx2 = HDAudio->Regs->RIRBWritePtr.roWritePtr;
            u32 ConnResp = HDAudio->DmaAlloc.Rirb[Idx2].roResponse;
            u8 ConnNode = ConnResp & 0xFF;
            
            if (ConnNode == OutConv) {
                OutPin = Pin;
                DebugStr("    *** Found Output Pin: NID "); DebugU8(Pin); 
                DebugStr(" connected to Output Converter NID "); DebugU8(OutConv); DebugChar('\n');
                break;
            }
        }
        if (OutPin != 0) break;
    }

    if (OutPin == 0 && PinCount > 0) {
        OutPin = PinNodes[0];
        DebugStr("WARNING: Using first Pin Widget NID "); DebugU8(OutPin); DebugStr(" as output (fallback)\n");
    }

    DebugStr("\nFinal: Output Converter NID "); DebugU8(OutConv);
    DebugStr(", Output Pin NID "); DebugU8(OutPin); DebugChar('\n');

    Old = HDAudio->Regs->RIRBWritePtr.roWritePtr;
    HDAudioAddCORB(HDAudio, CodecAddr, OutConv, 0x002, 0x10);
    Timeout = 1000000;
    while (HDAudio->Regs->RIRBWritePtr.roWritePtr == Old) {
        __asm__ volatile("pause");
        if (--Timeout == 0) break;
    }

    Old = HDAudio->Regs->RIRBWritePtr.roWritePtr;
    HDAudioAddCORB(HDAudio, CodecAddr, OutConv, 0x002, 0x00);
    Timeout = 1000000;
    while (HDAudio->Regs->RIRBWritePtr.roWritePtr == Old) {
        __asm__ volatile("pause");
        if (--Timeout == 0) break;
    }

    Old = HDAudio->Regs->RIRBWritePtr.roWritePtr;
    HDAudioAddCORB(HDAudio, CodecAddr, OutConv, 0x706, (1 << 4) | 0);
    Timeout = 1000000;
    while (HDAudio->Regs->RIRBWritePtr.roWritePtr == Old) {
        __asm__ volatile("pause");
        if (--Timeout == 0) break;
    }

    Old = HDAudio->Regs->RIRBWritePtr.roWritePtr;
    HDAudioAddCORB(HDAudio, CodecAddr, OutPin, 0x701, 0x00);
    Timeout = 1000000;
    while (HDAudio->Regs->RIRBWritePtr.roWritePtr == Old) {
        __asm__ volatile("pause");
        if (--Timeout == 0) break;
    }

    Old = HDAudio->Regs->RIRBWritePtr.roWritePtr;
    HDAudioAddCORB(HDAudio, CodecAddr, OutPin, 0x707, 0x40);
    Timeout = 1000000;
    while (HDAudio->Regs->RIRBWritePtr.roWritePtr == Old) {
        __asm__ volatile("pause");
        if (--Timeout == 0) break;
    }

    u16 Wp = HDAudio->Regs->RIRBWritePtr.roWritePtr;
    DebugStr("RIRB responses: ");
    for (u16 i = 0; i < Wp && i < 16; i++) {
        DebugU32(HDAudio->DmaAlloc.Rirb[i].roResponse);
        DebugChar(' ');
    }
    DebugChar('\n');

    SystemBusySleepUs(10000);

    volatile HDAudioRegsStreamChannel* Stream = &HDAudio->Regs->Channels[4];

    DebugStr("BDPL offset: 0x"); DebugU64((u64)&Stream->BDPL - (u64)Stream); DebugChar('\n');
    DebugStr("BDPU offset: 0x"); DebugU64((u64)&Stream->BDPU - (u64)Stream); DebugChar('\n');

    DebugStr("volatile HDAudioRegsStreamChannel* Stream = (volatile HDAudioRegsStreamChannel*)((u64)HDAudio->Regs + 0x100); =");DebugU64((u64)Stream);DebugChar('\n');

    Stream->CTL.U[0] = 0;
    Stream->CTL.U[1] = 0;
    Stream->CTL.U[2] = 0;
    __asm__ volatile("sfence" ::: "memory");

    Stream->FMT.rwBaseRate = 0;
    Stream->FMT.rwRateMultiplier = 0;
    Stream->FMT.rwRateDivisor = 0;
    Stream->FMT.rwBitDepth = 1;
    Stream->FMT.rwChannelCount = Double ? 1 : 0;

    Stream->BDPL = (u32)(u64)GlobalAudioBDL;
    Stream->BDPU = (u64)GlobalAudioBDL >> 32;
    DebugStr("BDL address: 0x"); DebugU64((u64)GlobalAudioBDL); DebugChar('\n');
    DebugStr("BDL written: 0x"); DebugU32(Stream->BDPU); DebugU32(Stream->BDPL); DebugChar('\n');
    Size64 TotalBufferSize = GlobalAudioPCMSizeCountOfBytePlay * (1 + Double);
    GlobalAudioBDL[0].Address = (u64)GlobalAudioPCM;
    GlobalAudioBDL[0].Length = (u32)TotalBufferSize;
    GlobalAudioBDL[0].Flags = 0x01;

    Stream->CBL.rwCyclicBufferLength = (u32)TotalBufferSize;
    Stream->LVI.rwLastValidIndex = 0;

    __asm__ volatile("sfence" ::: "memory");

    Stream->CTL.U[0] = (1 << 1) | (1 << 2);
    Stream->CTL.U[2] = (1 << 4);

    __asm__ volatile("sfence" ::: "memory");

    int Timeout4 = 100000;
    while (!Stream->STS.roFIFOReady && Timeout4--) {
        __asm__ volatile("pause");
    }

    if (Timeout4 == 0) {
        DebugStr("FIFO not ready!\n");
        return;
    }

    DebugStr("HDAudio Play Started.\n");
    DebugStr("LPIB: "); DebugU32(Stream->LPIB.roLinkPositionInBuffer); DebugChar('\n');
}

void HDAudioContinue(HDAudio* hda) {
    if (!hda || hda->State != HDA_STATE_READY) {
        return;
    }

    volatile HDAudioRegsStreamChannel* stream = &hda->Regs->Channels[4];

    if (!(stream->CTL.U[0] & (1 << 1))) {
        DebugStr("Stream not running.\n");
        return;
    }

    if (stream->STS.rw1cDescriptorError) {
        DebugStr("Descriptor Error! STS=");
        DebugU8(stream->STS.U);
        DebugChar('\n');
        stream->STS.rw1cDescriptorError = 1;
        return;
    }

    if (stream->STS.rw1cFIFOError) {
        DebugStr("FIFO Error! STS=");
        DebugU8(stream->STS.U);
        DebugChar('\n');
        stream->STS.rw1cFIFOError = 1;
        return;
    }

    u32 lpib = stream->LPIB.roLinkPositionInBuffer;
    u32 cbl = stream->CBL.rwCyclicBufferLength;
    
    DebugStr("LPIB: "); DebugU32(lpib); DebugChar('\n');
    DebugStr("CBL: "); DebugU32(cbl); DebugChar('\n');
}

void HDAudioStop(HDAudio* hda) {
    if (!hda || hda->State != HDA_STATE_READY) {
        return;
    }

    volatile HDAudioRegsStreamChannel* stream = &hda->Regs->Channels[4];

    if (!(stream->CTL.U[0] & (1 << 1))) {
        DebugStr("Stream already stopped.\n");
        return;
    }

    stream->CTL.U[0] &= ~(1 << 1);

    int timeout = 100000;
    while (stream->CTL.U[0] & (1 << 1)) {
        __asm__ volatile("pause");
        if (--timeout == 0) break;
    }

    if (timeout == 0) {
        DebugStr("Stream stop timeout!\n");
        stream->CTL.U[0] |= 1 << 0;
        for (volatile int i = 0; i < 1000; i++);
        stream->CTL.U[0] &= ~(1 << 0);
    } else {
        DebugStr("Stream stopped.\n");
    }

    if (stream->STS.rw1cDescriptorError) {
        stream->STS.rw1cDescriptorError = 1;
    }
    if (stream->STS.rw1cFIFOError) {
        stream->STS.rw1cFIFOError = 1;
    }
}

Bool HDAudioStopped(HDAudio* hda) {
    if (!hda || hda->State != HDA_STATE_READY) {
        return 1;
    }

    volatile HDAudioRegsStreamChannel* stream = &hda->Regs->Channels[4];
    return !(stream->CTL.U[0] & (1 << 1));
}

