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

void PrintRegsValue(xHCIController* Controller){
    DebugStr("Regs Value:\n");
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
    DebugStr("\tDoorbellRegs=0x");DebugU32(Controller->DoorbellRegs->rwDBStreamID);DebugChar('\n');
}

void PrintRegsAddr(xHCIController* Controller){
    DebugStr("Regs Addr:\n");
    DebugStr("\tvolatile xHCIRegsCapability*  CapabilityRegs =0x");DebugU64((u64)Controller->CapabilityRegs); DebugStr(";\n");
    DebugStr("\tvolatile xHCIRegsOperational* OperationalRegs=0x");DebugU64((u64)Controller->OperationalRegs);DebugStr(";\n");
    DebugStr("\tvolatile xHCIRegsRuntime*     RuntimeRegs    =0x");DebugU64((u64)Controller->RuntimeRegs);    DebugStr(";\n");
    DebugStr("\tvolatile xHCIRegsDoorbell*    DoorbellRegs   =0x");DebugU64((u64)Controller->DoorbellRegs);   DebugStr(";\n");
    DebugStr("CapabilityRegs:\n");
    DebugStr("\tCAPLENGTH =0x");DebugU64((u64)&Controller->CapabilityRegs->CAPLENGTH-(u64)Controller->CapabilityRegs); DebugChar('\n');
    DebugStr("\tHCIVERSION=0x");DebugU64((u64)&Controller->CapabilityRegs->HCIVERSION-(u64)Controller->CapabilityRegs);DebugChar('\n');
    DebugStr("\tHCSPARAMS1=0x");DebugU64((u64)&Controller->CapabilityRegs->HCSPARAMS1-(u64)Controller->CapabilityRegs);DebugChar('\n');
    DebugStr("\tHCSPARAMS2=0x");DebugU64((u64)&Controller->CapabilityRegs->HCSPARAMS2-(u64)Controller->CapabilityRegs);DebugChar('\n');
    DebugStr("\tHCSPARAMS3=0x");DebugU64((u64)&Controller->CapabilityRegs->HCSPARAMS3-(u64)Controller->CapabilityRegs);DebugChar('\n');
    DebugStr("\tHCCPARAMS1=0x");DebugU64((u64)&Controller->CapabilityRegs->HCCPARAMS1-(u64)Controller->CapabilityRegs);DebugChar('\n');
    DebugStr("\tDBOFF     =0x");DebugU64((u64)&Controller->CapabilityRegs->DBOFF-(u64)Controller->CapabilityRegs);     DebugChar('\n');
    DebugStr("\tRTSOFF    =0x");DebugU64((u64)&Controller->CapabilityRegs->RTSOFF-(u64)Controller->CapabilityRegs);    DebugChar('\n');
    DebugStr("\tHCCPARAMS2=0x");DebugU64((u64)&Controller->CapabilityRegs->HCCPARAMS2-(u64)Controller->CapabilityRegs);DebugChar('\n');
    DebugStr("OperationalRegs:\n");
    DebugStr("\tUSBCMD  =0x");  DebugU64((u64)&Controller->OperationalRegs->USBCMD-(u64)Controller->OperationalRegs);  DebugChar('\n');
    DebugStr("\tUSBSTS  =0x");  DebugU64((u64)&Controller->OperationalRegs->USBSTS-(u64)Controller->OperationalRegs);  DebugChar('\n');
    DebugStr("\tPAGESIZE=0x");  DebugU64((u64)&Controller->OperationalRegs->PAGESIZE-(u64)Controller->OperationalRegs);DebugChar('\n');
    DebugStr("\tDNCTRL  =0x");  DebugU64((u64)&Controller->OperationalRegs->DNCTRL-(u64)Controller->OperationalRegs);  DebugChar('\n');
    DebugStr("\tCRCR    =0x");  DebugU64((u64)&Controller->OperationalRegs->CRCR-(u64)Controller->OperationalRegs);    DebugChar('\n');
    DebugStr("\tDCBAAP  =0x");  DebugU64((u64)&Controller->OperationalRegs->DCBAAP-(u64)Controller->OperationalRegs);  DebugChar('\n');
    DebugStr("\tCONFIG  =0x");  DebugU64((u64)&Controller->OperationalRegs->CONFIG-(u64)Controller->OperationalRegs);  DebugChar('\n');
    DebugStr("\tPORTS   =0x");  DebugU64((u64)&Controller->OperationalRegs->PORTS-(u64)Controller->OperationalRegs);   DebugStr("{\n");
    for (u32 Idx=0;Idx<Controller->CapabilityRegs->HCSPARAMS1.roNumberOfPorts;Idx++){
        DebugStr("\t\t");DebugU64((u64)&Controller->OperationalRegs->PORTS[Idx]-(u64)Controller->OperationalRegs);                                    DebugStr("{\n");
        DebugStr("\t\t\tPORTSC   =0x");DebugU64((u64)&Controller->OperationalRegs->PORTS[Idx].PORTSC-(u64)&Controller->OperationalRegs->PORTS[Idx]);   DebugChar('\n');
        DebugStr("\t\t\tPORTPMSC =0x");DebugU64((u64)&Controller->OperationalRegs->PORTS[Idx].PORTPMSC-(u64)&Controller->OperationalRegs->PORTS[Idx]); DebugChar('\n');
        DebugStr("\t\t\tPORTLI   =0x");DebugU64((u64)&Controller->OperationalRegs->PORTS[Idx].PORTLI-(u64)&Controller->OperationalRegs->PORTS[Idx]);   DebugChar('\n');
        DebugStr("\t\t\tPORTHLPMC=0x");DebugU64((u64)&Controller->OperationalRegs->PORTS[Idx].PORTHLPMC-(u64)&Controller->OperationalRegs->PORTS[Idx]);DebugChar('\n');
        if (Idx!=(Controller->CapabilityRegs->HCSPARAMS1.roNumberOfPorts-1)) DebugStr("\t\t},\n");
        else DebugStr("\t\t}\n");
    }
    DebugStr("\t}\n");
    DebugStr("RuntimeRegs:\n");
    DebugStr("\tMFINDEX=0x");DebugU64((u64)&Controller->RuntimeRegs->MFINDEX-(u64)Controller->RuntimeRegs);              DebugChar('\n');
    DebugStr("\tIMAN   =0x");DebugU64((u64)&Controller->RuntimeRegs->Interrupter[0].IMAN-(u64)Controller->RuntimeRegs);  DebugChar('\n');
    DebugStr("\tIMOD   =0x");DebugU64((u64)&Controller->RuntimeRegs->Interrupter[0].IMOD-(u64)Controller->RuntimeRegs);  DebugChar('\n');
    DebugStr("\tERSTSZ =0x");DebugU64((u64)&Controller->RuntimeRegs->Interrupter[0].ERSTSZ-(u64)Controller->RuntimeRegs);DebugChar('\n');
    DebugStr("\tERSTBA =0x");DebugU64((u64)&Controller->RuntimeRegs->Interrupter[0].ERSTBA-(u64)Controller->RuntimeRegs);DebugChar('\n');
    DebugStr("\tERDP   =0x");DebugU64((u64)&Controller->RuntimeRegs->Interrupter[0].ERDP-(u64)Controller->RuntimeRegs);  DebugChar('\n');
}

void XHCIFuncInt(){
    DebugStr("Entry XHCIFuncInt\n");
    xHCIController* Ctrl=GlobalXHCI;
    if (!Ctrl->RuntimeRegs->Interrupter->IMAN.rw1cInterruptPending) {
        return;
    }
    Ctrl->RuntimeRegs->Interrupter[0].IMAN.rw1cInterruptPending=1;
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

void SendCommandRing(xHCIController* Ctrl, xHCITransferRequestBlock TRB, u64 Idx){
    Ctrl->CommandRingTRBs[Idx].NormalTRBField0.U = TRB.NormalTRBField0.U;
    Ctrl->CommandRingTRBs[Idx].NormalTRBField1.U = TRB.NormalTRBField1.U;
    Ctrl->CommandRingTRBs[Idx].NormalTRBField2.U = TRB.NormalTRBField2.U;

    Ctrl->DoorbellRegs[0].U = (u32)Idx;
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
    Ctrl->RuntimeRegs     = (xHCIRegsRuntime*)(Base + Ctrl->CapabilityRegs->RTSOFF.U);
    Ctrl->DoorbellRegs    = (xHCIRegsDoorbell*)(Base + Ctrl->CapabilityRegs->DBOFF.U);

    while (Ctrl->OperationalRegs->USBSTS.roControllerNotReady) __asm__("pause");

    DebugStr("ControllerReady:\n");
    PrintRegsValue(Ctrl);

    Ctrl->OperationalRegs->USBCMD.rwRunStop = 0;
    Ctrl->OperationalRegs->USBCMD.rwHostControllerReset = 1;
    while (Ctrl->OperationalRegs->USBCMD.U & (1 << 1)) __asm__("pause");
    while (Ctrl->OperationalRegs->USBSTS.roControllerNotReady) __asm__("pause");

    DebugStr("Controller Ready & Reset Completed:\n");

    Ctrl->OperationalRegs->CONFIG.rwMaxDeviceSlotsEnabled = Ctrl->CapabilityRegs->HCSPARAMS1.roNumberOfDeviceSlots;
    Ctrl->OperationalRegs->CONFIG.rwU3EntryEnable=0;
    Ctrl->OperationalRegs->CONFIG.rwConfigurationInformationEnable=0;
    Ctrl->OperationalRegs->CONFIG.U=0x00000000|Ctrl->CapabilityRegs->HCSPARAMS1.roNumberOfDeviceSlots;

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
    PrintRegsValue(Ctrl);
    PrintRegsAddr(Ctrl);

    Ctrl->OperationalRegs->USBCMD.rwRunStop = 1;
    while (Ctrl->OperationalRegs->USBSTS.roHCHalted) __asm__("pause");

    DebugStr("Run xHCI!!!!!\n");

    for (u64 Idx = 0; Idx < Ctrl->CapabilityRegs->HCSPARAMS1.roNumberOfPorts; Idx++) {
        if (Ctrl->OperationalRegs->PORTS[Idx].PORTSC.rosCurrentConnectStatus == 0) continue; 

        Ctrl->OperationalRegs->PORTS[Idx].PORTSC.rw1csConnectStatusChange = 1;
        Ctrl->OperationalRegs->PORTS[Idx].PORTSC.rw1sPortReset = 1;
        
        while (Ctrl->OperationalRegs->PORTS[Idx].PORTSC.rw1sPortReset) __asm__("pause");
        while (!Ctrl->OperationalRegs->PORTS[Idx].PORTSC.rw1csPortResetChange) __asm__("pause");
        
        Ctrl->OperationalRegs->PORTS[Idx].PORTSC.rw1csPortResetChange = 1;
    }
    Ctrl->OperationalRegs->USBCMD.rwInterrupterEnable=1;
    Ctrl->RuntimeRegs->Interrupter[0].IMAN.rwInterruptEnable=1;
    xHCITransferRequestBlock TRBs={{.U=0},{.U=0},{.CycleBit=1,.TRBType=9}};
    SendCommandRing(Ctrl,TRBs,0);

    PrintRegsValue(Ctrl);

    DebugStr("xHCI Started Successfully!\n");
    return Ctrl;
}