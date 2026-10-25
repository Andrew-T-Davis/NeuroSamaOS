#include "Context.h"
#include "clock.h"
#include "efi.h"
#include "int.h"
#include "acpi.h"
#include "port.h"
#include "debug.h"
#include "ioapic.h"
#include "event.h"

static u64 HPETBase=DefineHPETBase;
u64 HPETFreqHz=0;
u64 HPET10MsTicks=0;

extern RegContext MainGlobalContext;

u64 GetAcpiAddrUEFI(EFI_BOOT_SERVICES* bs, u64 *Size, u64 *FADTAddr, u64 *DSDTAddr, u64* HPETAddr, u64* MADTAddr, u64* DMARAddr){
    UINTN MemoryMapSize = 0;
    EFI_MEMORY_DESCRIPTOR *MemMap = NULL;
    UINTN MapKey, DescriptorSize;
    u32 DescriptorVersion;
    bs->GetMemoryMap(&MemoryMapSize, NULL, &MapKey, &DescriptorSize, &DescriptorVersion);
    MemoryMapSize += 2 * DescriptorSize;
    bs->AllocatePool(EfiLoaderData, MemoryMapSize, (void**)&MemMap);
    bs->GetMemoryMap(&MemoryMapSize, (void*)MemMap, &MapKey, &DescriptorSize, &DescriptorVersion);

    UINTN EntryCount = MemoryMapSize / DescriptorSize;

    u64 AcpiAddr=0;
    *Size=0;
    u64 HPETAddrDebug=0;

    for (UINTN i = 0; i < EntryCount; i++) {
        EFI_MEMORY_DESCRIPTOR *desc = (EFI_MEMORY_DESCRIPTOR*)((u8*)MemMap + (i * DescriptorSize));

        if (desc->Type == EfiAcpiReclaimMemory) {
            AcpiAddr=desc->PhysicalStart;
            *Size=(desc->NumberOfPages * 4096);
            
            for (u64 Idx = 0; Idx < (*Size); Idx += 4){
                if (*(((u32*)(AcpiAddr+Idx))) == 0x50434146) {
                    *FADTAddr = (u64)(AcpiAddr+Idx);
                }
                if (*(((u32*)(AcpiAddr+Idx))) == 0x54445344) {
                    *DSDTAddr = (u64)(AcpiAddr+Idx);
                }
                if (*(((u32*)(AcpiAddr+Idx))) == 0x54455048) {
                    u8* Header = (u8*)(AcpiAddr+Idx);
                    u32 TableLength = *(u32*)(Header + 4);
                    if (TableLength >= 56 && TableLength < 1024) {
                        *HPETAddr = AcpiAddr+Idx;
                        HPETAddrDebug = AcpiAddr+Idx;
                    }
                }
                if (*(((u32*)(AcpiAddr+Idx))) == 0x43495041) {
                    *MADTAddr = (u64)(AcpiAddr+Idx);
                }
                if (*(((u32*)(AcpiAddr+Idx))) == 0x52414D44) {
                    *DMARAddr = (u64)(AcpiAddr+Idx);
                }
            }
        }
    }
    DebugStr("HPETAddr at 0x");
    DebugU64(HPETAddrDebug);
    DebugChar('\n');
    return AcpiAddr;
}

void StopOtherCores(void) {
    Bool is_x2apic = (rdmsr(0x1B) & (3ULL << 10)) == (3ULL << 10);
    
    u32 BspApicId;
    if (is_x2apic) {
        BspApicId = (u32)(rdmsr(0x802) & 0xFFFFFFFF);
    } else {
        volatile u32* apic_id_reg = (volatile u32*)0xFEE00020;
        BspApicId = (*apic_id_reg >> 24) & 0xFF;
    }
    
    DebugStr("StopOtherCores: BSP APIC ID=0x");
    DebugU32(BspApicId);
    DebugStr(" x2APIC=");
    DebugU8(is_x2apic);
    DebugChar('\n');
    
    if (is_x2apic) {
        wrmsr(0x830, 0x000C4500ULL); 
        
        __asm__ volatile("mfence" ::: "memory");
        
    } else {
        volatile u32* IcrLow = (volatile u32*)0xFEE00300;
        
        u32 timeout = 100000;
        while ((*IcrLow & (1 << 12)) && timeout) {
            __asm__("pause");
            timeout--;
        }
        
        *IcrLow = 0x000C4500;
        
        timeout = 100000;
        while ((*IcrLow & (1 << 12)) && timeout) {
            __asm__("pause");
            timeout--;
        }
    }
    
    SystemBusySleepMs(10);
    
    DebugStr("StopOtherCores: Broadcast INIT sent\n");
}


void ReBoot(u64 FADTAddr){
    DebugStr("See You Next Time :D 73\n");
    StopOtherCores();
    __asm__ volatile("cli");
    AcpiReset* Reset=(AcpiReset*)(FADTAddr+116);
    if (Reset->Info.AddressSpaceID==0){
        if (Reset->Info.AccessSize==1) outb((u16)Reset->Addr, (u8)(Reset->ResetValue << Reset->Info.RegisterBitOffset));
        else if (Reset->Info.AccessSize==2) outw((u16)Reset->Addr, (u16)(Reset->ResetValue << Reset->Info.RegisterBitOffset));
        else if (Reset->Info.AccessSize==3) outl((u16)Reset->Addr, (u32)(Reset->ResetValue << Reset->Info.RegisterBitOffset));
        else if (Reset->Info.AccessSize==4) {
            *((u64*)(u64)Reset->Addr)=((u64)Reset->ResetValue << Reset->Info.RegisterBitOffset);
        }
        if (Reset->Info.AccessSize==1) *((u8*)(u64)Reset->Addr)=(u8)(Reset->ResetValue << Reset->Info.RegisterBitOffset);
        else if (Reset->Info.AccessSize==2) *((u16*)(u64)Reset->Addr)=(u16)(Reset->ResetValue << Reset->Info.RegisterBitOffset);
        else if (Reset->Info.AccessSize==3) *((u32*)(u64)Reset->Addr)=(u32)(Reset->ResetValue << Reset->Info.RegisterBitOffset);
        else if (Reset->Info.AccessSize==4) {
            *((u64*)(u64)Reset->Addr)=((u64)Reset->ResetValue << Reset->Info.RegisterBitOffset);
        }
    }else{
        if (Reset->Info.AccessSize==1) *((u8*)(u64)Reset->Addr)=(u8)(Reset->ResetValue << Reset->Info.RegisterBitOffset);
        else if (Reset->Info.AccessSize==2) *((u16*)(u64)Reset->Addr)=(u16)(Reset->ResetValue << Reset->Info.RegisterBitOffset);
        else if (Reset->Info.AccessSize==3) *((u32*)(u64)Reset->Addr)=(u32)(Reset->ResetValue << Reset->Info.RegisterBitOffset);
        else if (Reset->Info.AccessSize==4) {
            *((u64*)(u64)Reset->Addr)=((u64)Reset->ResetValue << Reset->Info.RegisterBitOffset);
        }
        if (Reset->Info.AccessSize==1) outb((u16)Reset->Addr, (u8)(Reset->ResetValue << Reset->Info.RegisterBitOffset));
        else if (Reset->Info.AccessSize==2) outw((u16)Reset->Addr, (u16)(Reset->ResetValue << Reset->Info.RegisterBitOffset));
        else if (Reset->Info.AccessSize==3) outl((u16)Reset->Addr, (u32)(Reset->ResetValue << Reset->Info.RegisterBitOffset));
        else if (Reset->Info.AccessSize==4) {
            *((u64*)(u64)Reset->Addr)=((u64)Reset->ResetValue << Reset->Info.RegisterBitOffset);
        }
    }
    SystemBusySleepUs(500);
    outb(0x64, 0xFE);
    SystemBusySleepUs(500);
    outb(0xCF9, 0x04);
    outb(0xCF9, 0x06);
    outb(0xCF9, 0x0E);
    SystemBusySleepUs(500);
    struct {
        u16 limit;
        u64 base;
    } __attribute__((packed)) zero_idt = { 0, 0 };
    __asm__("lidt %0" : : "m"(zero_idt));
    __asm__("int $0");
    SystemBusySleepUs(500);
    DebugStr("Oh No :( SOS CQD(WTF? NoWay!Else You Not x86)\n");
    while(1) __asm__ volatile("hlt");
}

void ShutDown(u64 FADTAddr, u64 DSDTAddr){
    DebugStr("GoodBye! :D 73\n");
    __asm__ volatile("cli");
    u8* DSDTByte=(u8*)DSDTAddr;
FindObj:
    while (*DSDTByte!=0x08) DSDTByte++;
    if (*DSDTByte==0x08) DSDTByte++;
    if (*((u32*)(u64)DSDTByte)!=0x5F35535F) goto FindObj;
    DSDTByte+=4;
    if (*DSDTByte!=0x12) goto FindObj;
    DSDTByte++;
    u8 leadByte = *DSDTByte;
    u8 extra = leadByte & 0b11;
    u64 PkgLength = leadByte >> 2;
    u8 bytesRead = 1;
    u8 shift = 6;
    while (extra > 0) {
        DSDTByte++;
        PkgLength |= (u64)(*DSDTByte) << shift;
        extra--;
        bytesRead++;
        shift += 8;
    }
    DSDTByte += bytesRead;
    u16 PM1a=0;
    u16 PM1b=0;

    u16 PM1aCountBLKPort=*(u16*)(FADTAddr + 64);
    u16 PM1bCountBLKPort=*(u16*)(FADTAddr + 68);

    AcpiShutDown* XPM1aCountBLK=(AcpiShutDown*)(FADTAddr + 148);
    AcpiShutDown* XPM1bCountBLK=(AcpiShutDown*)(FADTAddr + 160);

    u64 remaining = PkgLength;
    while (remaining > 0) {
        if (*DSDTByte == 0x0A) {
            DSDTByte++;
            if (PM1a == 0) {
                PM1a = *DSDTByte;
            } else if (PM1b == 0) {
                PM1b = *DSDTByte;
            }
            DSDTByte++;
            remaining -= 2;
        } else if (*DSDTByte == 0x0B) {
            DSDTByte++;
            if (PM1a == 0) {
                PM1a = *(u16*)DSDTByte;
            } else if (PM1b == 0) {
                PM1b = *(u16*)DSDTByte;
            }
            DSDTByte += 2;
            remaining -= 3;
        } else if (*DSDTByte == 0x0C) {
            DSDTByte++;
            if (PM1a == 0) {
                PM1a = *(u32*)DSDTByte & 0xFFFF;
            } else if (PM1b == 0) {
                PM1b = *(u32*)DSDTByte & 0xFFFF;
            }
            DSDTByte += 4;
            remaining -= 5;
        } else if (*DSDTByte == 0xA0) {
            DSDTByte++;
            u8 ifLead = *DSDTByte;
            u8 ifExtra = ifLead & 0b11;
            u64 ifPkgLen = ifLead >> 2;
            u8 ifBytes = 1;
            u8 ifShift = 6;
            while (ifExtra > 0) {
                DSDTByte++;
                ifPkgLen |= (u64)(*DSDTByte) << ifShift;
                ifExtra--;
                ifBytes++;
                ifShift += 8;
            }
            DSDTByte += ifBytes;
            u8* ifStart = DSDTByte;
            u64 ifRemain = ifPkgLen;
            while (ifRemain > 0) {
                if (*DSDTByte == 0x0A) {
                    DSDTByte++;
                    if (PM1a == 0) {
                        PM1a = *DSDTByte;
                    } else if (PM1b == 0) {
                        PM1b = *DSDTByte;
                    }
                    DSDTByte++;
                    ifRemain -= 2;
                } else if (*DSDTByte == 0x0B) {
                    DSDTByte++;
                    if (PM1a == 0) {
                        PM1a = *(u16*)DSDTByte;
                    } else if (PM1b == 0) {
                        PM1b = *(u16*)DSDTByte;
                    }
                    DSDTByte += 2;
                    ifRemain -= 3;
                } else if (*DSDTByte == 0x0C) {
                    DSDTByte++;
                    if (PM1a == 0) {
                        PM1a = *(u32*)DSDTByte & 0xFFFF;
                    } else if (PM1b == 0) {
                        PM1b = *(u32*)DSDTByte & 0xFFFF;
                    }
                    DSDTByte += 4;
                    ifRemain -= 5;
                } else {
                    DSDTByte++;
                    ifRemain--;
                }
            }
            DSDTByte = ifStart + ifPkgLen;
            remaining -= (ifBytes + ifPkgLen);
        } else {
            DSDTByte++;
            remaining--;
        }
    }

    if (PM1a) {
        DebugStr("PM1a Found\n");
        outw(PM1aCountBLKPort, PM1a | 0x2000);
        if (XPM1aCountBLK->Info.AddressSpaceID == 1) {
            outw((u16)XPM1aCountBLK->Addr, PM1a | 0x2000);
        } else {
            *((u16*)XPM1aCountBLK->Addr) = PM1a | 0x2000;
        }
    }
    if (PM1b) {
        DebugStr("PM1b Found\n");
        outw(PM1bCountBLKPort, PM1b | 0x2000);
        if (XPM1bCountBLK->Info.AddressSpaceID == 1) {
            outw((u16)XPM1bCountBLK->Addr, PM1b | 0x2000);
        } else {
            *((u16*)XPM1bCountBLK->Addr) = PM1b | 0x2000;
        }
    }
    SystemBusySleepMs(500);

    DebugStr("Oh No :(\n");
    while(1) __asm__ volatile("hlt");
}

u64 GetHPETMMIO(u64 HPETAddr){
    if (HPETAddr == 0) {
        return DefineHPETBase;
    }
    
    HPETBaseAddress* HPETAddress = (HPETBaseAddress*)(HPETAddr + 40);
    if (HPETAddress->Address != 0) {
        return HPETAddress->Address;
    }
    return DefineHPETBase;
}

void SetHPETMMIO(u64 HPETAddr){
    HPETBaseAddress* HPETAddress = (HPETBaseAddress*)(HPETAddr + 40);
    if (HPETAddress->Address != 0) HPETBase=HPETAddress->Address;
    else HPETBase=DefineHPETBase;
}

void HPETISRFuncInt(void) {
    AcpiHPETRegs *HPET = (AcpiHPETRegs*)HPETBase;

    HPET->rw1cInterruptStatusRegister = 1;

    MakeEvent();

    LocalApicEOI();
}

__attribute__((naked)) void HPETISR(){
    __asm__ volatile(
        "pushq %%rax\n\t" "pushq %%rcx\n\t" "pushq %%rdx\n\t" "pushq %%rbx\n\t"
        "pushq %%rbp\n\t" "pushq %%rsi\n\t" "pushq %%rdi\n\t"
        "pushq %%r8\n\t"  "pushq %%r9\n\t"  "pushq %%r10\n\t" "pushq %%r11\n\t"
        "pushq %%r12\n\t" "pushq %%r13\n\t" "pushq %%r14\n\t" "pushq %%r15\n\t"
        "call HPETISRFuncInt\n\t"
        "popq %%r15\n\t" "popq %%r14\n\t" "popq %%r13\n\t" "popq %%r12\n\t"
        "popq %%r11\n\t" "popq %%r10\n\t" "popq %%r9\n\t"  "popq %%r8\n\t"
        "popq %%rdi\n\t" "popq %%rsi\n\t" "popq %%rbp\n\t" "popq %%rbx\n\t"
        "popq %%rdx\n\t" "popq %%rcx\n\t" "popq %%rax\n\t"
        "iretq\n\t"
        ::: "memory"
    );
}

void CalibrateHPET(void) {
    AcpiHPETRegs *hpet = (AcpiHPETRegs*)HPETBase;

    u32 period_fs = (u32)(hpet->roCapabilitiesAndIDRegister >> 32);

    if (period_fs == 0) {
        return;
    }

    HPET10MsTicks = 1000000000000ULL / period_fs;
    HPETFreqHz = HPET10MsTicks * 100;
    hpet->rwConfigurationRegister |= 1ULL;
}