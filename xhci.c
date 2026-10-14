#include "xhci.h"
#include "int.h"
#include "pci.h"
#include "kmalloc.h"
#include "debug.h"
#include "ioapic.h"
#include "str.h"

extern AllocPool KernelPool;

xHCIController* GlobalXHCI=NULL;

u64 GetXHCIMMIO(){
    u8 Bus,Dev,Func;
    PCIFindDeviceByClass(0x0C, 0x03, 0x30, &Bus, &Dev, &Func);
    if (Dev!=0xFF) return PCIGetBARAddress(Bus, Dev, Func, 0);
    return 0;
}

void PrintRegsAddr(xHCIController* Controller){
    DebugStr("Regs Addr:\n");
    DebugStr("\tvolatile xHCIRegsCapability*  CapabilityRegs =0x");DebugU64((u64)Controller->CapabilityRegs); DebugStr(";\n");
    DebugStr("\tvolatile xHCIRegsOperational* OperationalRegs=0x");DebugU64((u64)Controller->OperationalRegs);DebugStr(";\n");
    DebugStr("\tvolatile xHCIRegsRuntime*     RuntimeRegs    =0x");DebugU64((u64)Controller->RuntimeRegs);    DebugStr(";\n");
    DebugStr("\tvolatile xHCIRegsDoorbell*    DoorbellRegs   =0x");DebugU64((u64)Controller->DoorbellRegs);   DebugStr(";\n");
    DebugStr("CapabilityRegs:\n");
    DebugStr("\tCAPLENGTH =0x");DebugU8(Controller->CapabilityRegs->CAPLENGTH);     DebugChar('\n');
    DebugStr("\tHCIVERSION=0x");DebugU16(Controller->CapabilityRegs->HCIVERSION);   DebugChar('\n');
    DebugStr("\tHCSPARAMS1=0x");DebugU32(Controller->CapabilityRegs->HCSPARAMS1.U);DebugChar('\n');
    DebugStr("\tHCSPARAMS2=0x");DebugU32(Controller->CapabilityRegs->HCSPARAMS2.U);DebugChar('\n');
    DebugStr("\tHCSPARAMS3=0x");DebugU32(Controller->CapabilityRegs->HCSPARAMS3.U);DebugChar('\n');
    DebugStr("\tHCCPARAMS1=0x");DebugU32(Controller->CapabilityRegs->HCCPARAMS1.U);DebugChar('\n');
    DebugStr("\tDBOFF     =0x");DebugU32(Controller->CapabilityRegs->DBOFF.U);     DebugChar('\n');
    DebugStr("\tRTSOFF    =0x");DebugU32(Controller->CapabilityRegs->RTSOFF.U);    DebugChar('\n');
    DebugStr("\tHCCPARAMS2=0x");DebugU32(Controller->CapabilityRegs->HCCPARAMS2.U);DebugChar('\n');
    DebugStr("OperationalRegs:\n");
    DebugStr("\tUSBCMD  =0x");  DebugU32(Controller->OperationalRegs->USBCMD.U);  DebugChar('\n');
    DebugStr("\tUSBSTS  =0x");  DebugU32(Controller->OperationalRegs->USBSTS.U);  DebugChar('\n');
    DebugStr("\tPAGESIZE=0x");  DebugU32(Controller->OperationalRegs->PAGESIZE.U);DebugChar('\n');
    DebugStr("\tDNCTRL  =0x");  DebugU32(Controller->OperationalRegs->DNCTRL.U);  DebugChar('\n');
    DebugStr("\tCRCR    =0x");  DebugU64(Controller->OperationalRegs->CRCR.U);   DebugChar('\n');
    DebugStr("\tDCBAAP  =0x");  DebugU64(Controller->OperationalRegs->DCBAAP.U); DebugChar('\n');
    DebugStr("\tCONFIG  =0x");  DebugU32(Controller->OperationalRegs->CONFIG.U);  DebugChar('\n');
    DebugStr("\tPORTS   = {\n");
    for (u32 Idx=0;Idx<Controller->CapabilityRegs->HCSPARAMS1.roNumberOfPorts;Idx++){
        DebugStr("\t\t{ \n");
        DebugStr("\t\t\tPORTSC   =0x");DebugU32(Controller->OperationalRegs->PORTS[Idx].PORTSC.U);   DebugChar('\n');
        DebugStr("\t\t\tPORTPMSC =0x");DebugU32(Controller->OperationalRegs->PORTS[Idx].PORTPMSC.U); DebugChar('\n');
        DebugStr("\t\t\tPORTLI   =0x");DebugU32(Controller->OperationalRegs->PORTS[Idx].PORTLI.U);   DebugChar('\n');
        DebugStr("\t\t\tPORTHLPMC=0x");DebugU32(Controller->OperationalRegs->PORTS[Idx].PORTHLPMC.U);DebugChar('\n');
        if (Idx!=(Controller->CapabilityRegs->HCSPARAMS1.roNumberOfPorts-1)) DebugStr("\t\t},\n");
        else DebugStr("\t\t}\n");
    }
    DebugStr("\t}\n");
    DebugStr("RuntimeRegs:\n");
    DebugStr("\tMFINDEX=0x");DebugU32(Controller->RuntimeRegs->MFINDEX.U);DebugChar('\n');
    DebugStr("\tIMAN   =0x");DebugU32(Controller->RuntimeRegs->Interrupter[0].IMAN.U);   DebugChar('\n');
    DebugStr("\tIMOD   =0x");DebugU32(Controller->RuntimeRegs->Interrupter[0].IMOD.U);   DebugChar('\n');
    DebugStr("\tERSTSZ =0x");DebugU32(Controller->RuntimeRegs->Interrupter[0].ERSTSZ.U); DebugChar('\n');
    DebugStr("\tERSTBA =0x");DebugU64(Controller->RuntimeRegs->Interrupter[0].ERSTBA.U);DebugChar('\n');
    DebugStr("\tERDP   =0x");DebugU64(Controller->RuntimeRegs->Interrupter[0].ERDP.U);  DebugChar('\n');
    DebugStr("DoorbellRegs:\n");
    DebugStr("\tDoorbellRegs=0x");DebugU32(Controller->DoorbellRegs->U);DebugChar('\n');
}

void XHCIFuncInt(){
    LocalApicEOI();
}

__attribute__((naked)) void XHCIInt(){
    __asm__ volatile(
        "pushq %%rax\n\t" "pushq %%rcx\n\t" "pushq %%rdx\n\t" "pushq %%rbx\n\t"
        "pushq %%rbp\n\t" "pushq %%rsi\n\t" "pushq %%rdi\n\t"
        "pushq %%r8\n\t"  "pushq %%r9\n\t"  "pushq %%r10\n\t" "pushq %%r11\n\t"
        "pushq %%r12\n\t" "pushq %%r13\n\t" "pushq %%r14\n\t" "pushq %%r15\n\t"
        "call XHCIFuncInt\n\t"
        "popq %%r15\n\t" "popq %%r14\n\t" "popq %%r13\n\t" "popq %%r12\n\t"
        "popq %%r11\n\t" "popq %%r10\n\t" "popq %%r9\n\t"  "popq %%r8\n\t"
        "popq %%rdi\n\t" "popq %%rsi\n\t" "popq %%rbp\n\t" "popq %%rbx\n\t"
        "popq %%rdx\n\t" "popq %%rcx\n\t" "popq %%rax\n\t"
        "iretq\n\t"
        ::: "memory"
    );
}

xHCIController* InitXHCI() {
    u8 Bus, Dev, Func;
    PCIFindDeviceByClass(0x0C, 0x03, 0x30, &Bus, &Dev, &Func);
    if (Dev == 0xFF) return NULL;

    u64 Base = PCIGetBARAddress(Bus, Dev, Func, 0);
    if (Base == 0) return NULL;

    xHCIController* Ctrl = AlignedAlloc(&KernelPool, sizeof(xHCIController), 64);
    Ctrl->CapabilityRegs  = (xHCIRegsCapability*)Base;
    Ctrl->OperationalRegs = (xHCIRegsOperational*)(Base + Ctrl->CapabilityRegs->CAPLENGTH);
    Ctrl->RuntimeRegs     = (xHCIRegsRuntime*)(Base + Ctrl->CapabilityRegs->RTSOFF.roRuntimeRegisterSpaceOffset);
    Ctrl->DoorbellRegs    = (xHCIRegsDoorbell*)(Base + Ctrl->CapabilityRegs->DBOFF.roDoorbellArrayOffset);

    while (Ctrl->OperationalRegs->USBSTS.roControllerNotReady) __asm__("pause");

    DebugStr("ControllerReady:\n");
    PrintRegsAddr(Ctrl);

    Ctrl->OperationalRegs->USBCMD.rwRunStop = 0;
    Ctrl->OperationalRegs->USBCMD.rwHostControllerReset = 1;
    while (Ctrl->OperationalRegs->USBCMD.U & (1 << 1)) __asm__("pause");
    while (Ctrl->OperationalRegs->USBSTS.roControllerNotReady) __asm__("pause");

    DebugStr("Controller Ready & Reset Completed:\n");

    Ctrl->OperationalRegs->CONFIG.rwMaxDeviceSlotsEnabled = Ctrl->CapabilityRegs->HCSPARAMS1.roNumberOfDeviceSlots;
    Ctrl->OperationalRegs->CONFIG.rwU3EntryEnable=0;
    Ctrl->OperationalRegs->CONFIG.rwConfigurationInformationEnable=0;
    Ctrl->OperationalRegs->CONFIG.U=0x00000000|Ctrl->CapabilityRegs->HCSPARAMS1.roNumberOfDeviceSlots;

    DebugStr("CONFIG addr: 0x");
    DebugU64((u64)&Ctrl->OperationalRegs->CONFIG);
    DebugStr("\n");
    DebugStr("Expected:    0x");
    DebugU64(Base + Ctrl->CapabilityRegs->CAPLENGTH + 0x38);
    DebugStr("\n");

    DebugStr("DCBAAP addr: 0x");
    DebugU64((u64)&Ctrl->OperationalRegs->DCBAAP);
    DebugStr("\n");
    DebugStr("Expected:    0x");
    DebugU64(Base + Ctrl->CapabilityRegs->CAPLENGTH + 0x30);
    DebugStr("\n");

    DebugStr("IMAN addr:   0x");
    DebugU64((u64)&Ctrl->RuntimeRegs->Interrupter[0].IMAN);
    DebugStr("\n");
    DebugStr("Expected:    0x");
    DebugU64(Base + Ctrl->CapabilityRegs->RTSOFF.roRuntimeRegisterSpaceOffset + 0x20);
    DebugStr("\n");

    DebugStr("IMON addr:   0x");
    DebugU64((u64)&Ctrl->RuntimeRegs->Interrupter[0].IMOD);
    DebugStr("\n");
    DebugStr("Expected:    0x");
    DebugU64(Base + Ctrl->CapabilityRegs->RTSOFF.roRuntimeRegisterSpaceOffset + 0x24);
    DebugStr("\n");

    DebugStr("ERSTSZ addr: 0x");
    DebugU64((u64)&Ctrl->RuntimeRegs->Interrupter[0].ERSTSZ);
    DebugStr("\n");
    DebugStr("Expected:    0x");
    DebugU64(Base + Ctrl->CapabilityRegs->RTSOFF.roRuntimeRegisterSpaceOffset + 0x28);
    DebugStr("\n");

    DebugStr("ERSTBA addr: 0x");
    DebugU64((u64)&Ctrl->RuntimeRegs->Interrupter[0].ERSTBA);
    DebugStr("\n");
    DebugStr("Expected:    0x");
    DebugU64(Base + Ctrl->CapabilityRegs->RTSOFF.roRuntimeRegisterSpaceOffset + 0x30);
    DebugStr("\n");

    DebugStr("ERDP addr:   0x");
    DebugU64((u64)&Ctrl->RuntimeRegs->Interrupter[0].ERDP);
    DebugStr("\n");
    DebugStr("Expected:    0x");
    DebugU64(Base + Ctrl->CapabilityRegs->RTSOFF.roRuntimeRegisterSpaceOffset + 0x38);
    DebugStr("\n");

    while (!Ctrl->OperationalRegs->USBSTS.roHCHalted) {
        __asm__("pause");
    }

    Ctrl->OperationalRegs->DCBAAP.U = (u64)(Ctrl->xHCIDCBAAList);
    Ctrl->OperationalRegs->CRCR.U = ((u64)&Ctrl->CommandRingTRBs[0]) | 1ULL;

    MemSet((u8*)Ctrl->CommandRingTRBs,0,sizeof(Ctrl->CommandRingTRBs));
    Ctrl->CommandRingTRBs[63].NormalTRBField0.Base = (u64)(&Ctrl->CommandRingTRBs[0]);
    Ctrl->CommandRingTRBs[63].NormalTRBField2.TRBType = 6;
    Ctrl->CommandRingTRBs[63].NormalTRBField2.CycleBit = 1;

    Ctrl->ERST[0].rwEventRingSegmentBaseAddress = (u64)&Ctrl->EventRingTRBs[0];
    Ctrl->ERST[0].EventRingSegmentSize.rwEventRingSegmentSize = 256;
    Ctrl->ERST[0].rwRsvdZ0 = 0;

    Ctrl->RuntimeRegs->Interrupter[0].ERSTSZ.rwEventRingSegmentTableSize = 1;
    Ctrl->RuntimeRegs->Interrupter[0].ERSTBA.rwEventRingSegmentTableBaseAddressRegisteBase = (u64)&Ctrl->ERST[0];
    Ctrl->RuntimeRegs->Interrupter[0].ERDP.U = (u64)&Ctrl->EventRingTRBs[0];

    Ctrl->RuntimeRegs->Interrupter[0].IMAN.rwInterruptEnable = 1;

    u8 CapPtr = PCIReadBYTE(Bus, Dev, Func, 0x34);
    while (CapPtr != 0 && CapPtr != 0xFF) {
        u8 CapID = PCIReadBYTE(Bus, Dev, Func, CapPtr);
        u8 NextCapPtr = PCIReadBYTE(Bus, Dev, Func, CapPtr + 1);

        if (CapID == 0x05) {
            u8 Data = PCIReadBYTE(Bus, Dev, Func, CapPtr + 2);
            PCIWriteDWORD(Bus, Dev, Func, CapPtr + 4, 0xFEE00000);
            PCIWriteWORD(Bus, Dev, Func, CapPtr + 8, 0xEC);
            Data |= 0x01;
            PCIWriteBYTE(Bus, Dev, Func, CapPtr + 2, Data);
            DebugStr("MSI configured\n");
            break;
        } else if (CapID == 0x11) {
            u16 MsgCtrl = PCIReadWORD(Bus, Dev, Func, CapPtr + 2);
            u32 TableInfo = PCIReadDWORD(Bus, Dev, Func, CapPtr + 4);
            u8 Bir = (u8)(TableInfo & 0x7);
            u32 TableOffset = TableInfo & ~0x7U;

            u64 BarBaseAddress = PCIGetBARAddress(Bus, Dev, Func, Bir);
            volatile u32* MsixEntry = (volatile u32*)(BarBaseAddress + TableOffset);
            MsixEntry[0] = 0xFEE00000;
            MsixEntry[1] = 0x00000000;
            MsixEntry[2] = 0x000000EC;
            MsixEntry[3] = 0x00000000;

            MsgCtrl |= (1 << 15);
            PCIWriteWORD(Bus, Dev, Func, CapPtr + 2, MsgCtrl);
            DebugStr("MSI-X configured\n");
            break;
        }
        CapPtr = NextCapPtr;
    }

    __asm__ volatile("mfence" ::: "memory");

    DebugStr("Set All Regs:\n");
    PrintRegsAddr(Ctrl);

    Ctrl->OperationalRegs->USBCMD.rwRunStop = 1;
    while (Ctrl->OperationalRegs->USBSTS.roHCHalted) __asm__("pause");

    DebugStr("xHCI Started Successfully!\n");
    return Ctrl;
}