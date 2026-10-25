#include "ahci.h"
#include "clock.h"
#include "debug.h"
#include "int.h"
#include "ioapic.h"
#include "pci.h"
#include "kmalloc.h"
#include "str.h"

extern AllocPool KernelPool;
AHCIController* GlobalAHCICtrl=NULL;

u64 GetAHCIMMIO() {
    u8 Bus, Dev, Func;
    PCIFindDeviceByClass(0x01, 0x06, 0x01, &Bus, &Dev, &Func);
    return PCIGetBARAddress(Bus, Dev, Func, 5);
}

void AHCIIntFunc() {
    if (!GlobalAHCICtrl || !GlobalAHCICtrl->Regs) {
        LocalApicEOI();
        return;
    }

    AHCIRegs* regs = GlobalAHCICtrl->Regs;

    u32 ghc_is = regs->GenericHostControl.InterruptStatus.rwcInterruptPendingStatus;
    if (!ghc_is) {
        LocalApicEOI();
        return;
    }

    for (u8 port_idx = 0; port_idx < 32; port_idx++) {
        if (ghc_is & (1U << port_idx)) {
            volatile AHCIPortRegister* port = &regs->PortRegisters[port_idx];

            u32 px_is = port->PortInterruptStatus.U;

            port->PortInterruptStatus.U = px_is;
            __asm__ volatile("mfence" ::: "memory");

            if (px_is & (1U << 30)) {
                u32 serr = port->PortSerialATAError.U;
                port->PortSerialATAError.U = serr;
                DebugStr("[AHCI Interrupt] Error on Port ");
                DebugU8(port_idx);
                DebugChar('\n');
            }

            if (px_is & (1U << 0)) {
                AHCIProcessReceivedFIS(GlobalAHCICtrl, port_idx);
            }
        }
    }

    regs->GenericHostControl.InterruptStatus.rwcInterruptPendingStatus = ghc_is;
    __asm__ volatile("mfence" ::: "memory");
    LocalApicEOI();
}

__attribute__((naked)) void AHCIInt() {
    __asm__ volatile(
        "pushq %%rax\n\t" "pushq %%rcx\n\t" "pushq %%rdx\n\t" "pushq %%rbx\n\t"
        "pushq %%rbp\n\t" "pushq %%rsi\n\t" "pushq %%rdi\n\t"
        "pushq %%r8\n\t" "pushq %%r9\n\t" "pushq %%r10\n\t" "pushq %%r11\n\t"
        "pushq %%r12\n\t" "pushq %%r13\n\t" "pushq %%r14\n\t" "pushq %%r15\n\t"
        "call AHCIIntFunc\n\t"
        "popq %%r15\n\t" "popq %%r14\n\t" "popq %%r13\n\t" "popq %%r12\n\t"
        "popq %%r11\n\t" "popq %%r10\n\t" "popq %%r9\n\t" "popq %%r8\n\t"
        "popq %%rdi\n\t" "popq %%rsi\n\t" "popq %%rbp\n\t" "popq %%rbx\n\t"
        "popq %%rdx\n\t" "popq %%rcx\n\t" "popq %%rax\n\t"
        "iretq\n\t"
        ::: "memory"
    );
}

void AHCIProcessReceivedFIS(AHCIController* controller, u8 port_idx) {
    if (!controller || port_idx >= 32) return;
    volatile AHCIPortRegister* port = &controller->Regs->PortRegisters[port_idx];

    u64 Fb = ((u64)port->PortFISBaseAddressUpper32Bits.U << 32) | (u64)port->PortFISBaseAddress.U;
    if (!Fb) return;

    volatile u8* d2h_fis = (volatile u8*)(Fb + 0x40);

    if (d2h_fis[0] == 0x34) {
        u8 status = d2h_fis[2];
        u8 error  = d2h_fis[3];
        (void)status;
        (void)error;
    }
}

AHCIController* InitAHCI() {
    u8 Bus, Dev, Func;
    PCIFindDeviceByClass(0x01, 0x06, 0x01, &Bus, &Dev, &Func);
    if (Dev == 0xFF) {
        DebugStr("AHCI controller not found!\n");
        return NULL;
    }
    DebugStr("Found AHCI: Bus="); DebugU8(Bus);
    DebugStr(" Dev="); DebugU8(Dev);
    DebugStr(" Func="); DebugU8(Func); DebugChar('\n');
    u64 Base = PCIGetBARAddress(Bus, Dev, Func, 5);
    if (Base == 0) {
        DebugStr("BAR5 is 0!\n");
        return NULL;
    }

    u8 LastOffsetInPCICap = 0x34;
    u8 IntUse = DefineAHCIMSI;
    while (LastOffsetInPCICap <= 255) {
        u8 Data = PCIReadBYTE(Bus, Dev, Func, LastOffsetInPCICap);
        u8 NextOffsetInPCICap = PCIReadBYTE(Bus, Dev, Func, LastOffsetInPCICap + 1);
        if (NextOffsetInPCICap == 0 || NextOffsetInPCICap == 0xFF) {
            IntUse = DefineAHCINULL;
            break;
        } else {
            LastOffsetInPCICap = NextOffsetInPCICap;
            if (Data == 0x05) {
                IntUse = DefineAHCIMSI;
                break;
            } else if (Data == 0x11) {
                IntUse = DefineAHCIMSIX;
                break;
            }
        }
    }
    if (IntUse == DefineAHCIMSI) {
        u8 Data = PCIReadBYTE(Bus, Dev, Func, LastOffsetInPCICap + 2);
        PCIWriteDWORD(Bus, Dev, Func, LastOffsetInPCICap + 4, 0xFEE00000);
        PCIWriteWORD(Bus, Dev, Func, LastOffsetInPCICap + 8, 0xEC);
        Data |= 0b01;
        PCIWriteBYTE(Bus, Dev, Func, LastOffsetInPCICap + 2, Data);
        DebugStr("MSI configured\n");
    } else if (IntUse == DefineAHCIMSIX) {
        u16 Data = PCIReadWORD(Bus, Dev, Func, LastOffsetInPCICap + 2);
        u32 TableInfo = PCIReadDWORD(Bus, Dev, Func, LastOffsetInPCICap + 4);
        u8 Bir = (u8)(TableInfo & 0x7);
        u32 TableOffset = TableInfo & ~0x7U;

        u8 BarRegisterOffset = 0x10 + (Bir * 4);
        u32 BarBaseAddress = PCIReadDWORD(Bus, Dev, Func, BarRegisterOffset) & ~0xFU;
        
        volatile u32* MsixEntry = (volatile u32*)(u64)(BarBaseAddress + TableOffset);
        MsixEntry[0] = 0xFEE00000;
        MsixEntry[1] = 0x00000000;
        MsixEntry[2] = 0x000000EC;
        MsixEntry[3] = 0x00000000;
        PCIWriteDWORD(Bus, Dev, Func, LastOffsetInPCICap + 4, 0);
        Data |= (1 << 15);
        PCIWriteWORD(Bus, Dev, Func, LastOffsetInPCICap + 2, Data);
        DebugStr("MSI-X configured\n");
    }

    AHCIController* Ctrl = Alloc(&KernelPool, sizeof(AHCIController));
    if (!Ctrl) {
        DebugStr("Failed to allocate AHCIController!\n");
        return NULL;
    }
    Ctrl->Base = Base;
    Ctrl->Regs = (AHCIRegs*)((u64)Base);
    Ctrl->SoftPortImplemented = 0;

    for (int i = 0; i < 32; i++) {
        Ctrl->PortFISSlot[i] = 0;
        Ctrl->PortSlotWork[i] = 0;
    }

    DebugStr("BAR5: 0x"); DebugU64(Base); DebugChar('\n');

    DebugStr("Debug: Print All AHCI Reg Offset:\n");
    DebugStr("GenericHostControl Start: ");
    DebugU64((u64)&Ctrl->Regs->GenericHostControl);
    DebugChar('\n');
    DebugStr("\t{ HostCapabilities: ");
    DebugU16((u16)(((u64)&Ctrl->Regs->GenericHostControl.HostCapabilities) - ((u64)&Ctrl->Regs->GenericHostControl)));
    DebugStr(", GlobalHBAControl: ");
    DebugU16((u16)(((u64)&Ctrl->Regs->GenericHostControl.GlobalHBAControl) - ((u64)&Ctrl->Regs->GenericHostControl)));
    DebugStr(", InterruptStatus: ");
    DebugU16((u16)(((u64)&Ctrl->Regs->GenericHostControl.InterruptStatus) - ((u64)&Ctrl->Regs->GenericHostControl)));
    DebugStr(", PortsImplemented: ");
    DebugU16((u16)(((u64)&Ctrl->Regs->GenericHostControl.PortsImplemented) - ((u64)&Ctrl->Regs->GenericHostControl)));
    DebugChar('\n');
    DebugStr("\t, AHCIVersion: ");
    DebugU16((u16)(((u64)&Ctrl->Regs->GenericHostControl.AHCIVersion) - ((u64)&Ctrl->Regs->GenericHostControl)));
    DebugStr(", CCC Control: ");
    DebugU16((u16)(((u64)&Ctrl->Regs->GenericHostControl.CommandCompletionCoalescingControl) - ((u64)&Ctrl->Regs->GenericHostControl)));
    DebugStr(", CCC Ports: ");
    DebugU16((u16)(((u64)&Ctrl->Regs->GenericHostControl.CommandCompletionCoalescingPorts) - ((u64)&Ctrl->Regs->GenericHostControl)));
    DebugStr(", EM Location: ");
    DebugU16((u16)(((u64)&Ctrl->Regs->GenericHostControl.EnclosureManagementLocation) - ((u64)&Ctrl->Regs->GenericHostControl)));
    DebugChar('\n');
    DebugStr("\t, EM Control: ");
    DebugU16((u16)(((u64)&Ctrl->Regs->GenericHostControl.EnclosureManagementControl) - ((u64)&Ctrl->Regs->GenericHostControl)));
    DebugStr(", CAP2: ");
    DebugU16((u16)(((u64)&Ctrl->Regs->GenericHostControl.HBACapabilitiesExtended) - ((u64)&Ctrl->Regs->GenericHostControl)));
    DebugStr(", BOHC: ");
    DebugU16((u16)(((u64)&Ctrl->Regs->GenericHostControl.BIOSOrOSHandoffControlAndStatus) - ((u64)&Ctrl->Regs->GenericHostControl)));
    DebugStr(", Reserved0: ");
    DebugU16((u16)(((u64)&Ctrl->Regs->GenericHostControl.roReserved0) - ((u64)&Ctrl->Regs->GenericHostControl)));
    DebugChar('\n');
    DebugStr("\t, Vendor: ");
    DebugU16((u16)(((u64)&Ctrl->Regs->GenericHostControl.Vendor) - ((u64)&Ctrl->Regs->GenericHostControl)));
    DebugStr(" }\n\n");
    DebugStr("PortRegisters[0] Offset: ");
    DebugU16((u16)((u64)&Ctrl->Regs->PortRegisters[0] - ((u64)Ctrl->Regs)));
    DebugChar('\n');
    DebugStr("\t{ CLB: ");
    DebugU16((u16)(((u64)&Ctrl->Regs->PortRegisters[0].PortCommandListBaseAddress) - ((u64)Ctrl->Regs)));
    DebugStr(", CLBU: ");
    DebugU16((u16)(((u64)&Ctrl->Regs->PortRegisters[0].PortCommandListBaseAddressUpper32Bits) - ((u64)Ctrl->Regs)));
    DebugStr(", FB: ");
    DebugU16((u16)(((u64)&Ctrl->Regs->PortRegisters[0].PortFISBaseAddress) - ((u64)Ctrl->Regs)));
    DebugStr(", FBU: ");
    DebugU16((u16)(((u64)&Ctrl->Regs->PortRegisters[0].PortFISBaseAddressUpper32Bits) - ((u64)Ctrl->Regs)));
    DebugChar('\n');
    DebugStr("\t, IS: ");
    DebugU16((u16)(((u64)&Ctrl->Regs->PortRegisters[0].PortInterruptStatus) - ((u64)Ctrl->Regs)));
    DebugStr(", IE: ");
    DebugU16((u16)(((u64)&Ctrl->Regs->PortRegisters[0].PortInterruptEnable) - ((u64)Ctrl->Regs)));
    DebugStr(", CMD: ");
    DebugU16((u16)(((u64)&Ctrl->Regs->PortRegisters[0].PortCommandAndStatus) - ((u64)Ctrl->Regs)));
    DebugStr(", Reserved0: ");
    DebugU16((u16)(((u64)&Ctrl->Regs->PortRegisters[0].Reserved0) - ((u64)Ctrl->Regs)));
    DebugChar('\n');
    DebugStr("\t, TFD: ");
    DebugU16((u16)(((u64)&Ctrl->Regs->PortRegisters[0].PortTaskFileData) - ((u64)Ctrl->Regs)));
    DebugStr(", SIG: ");
    DebugU16((u16)(((u64)&Ctrl->Regs->PortRegisters[0].PortSignature) - ((u64)Ctrl->Regs)));
    DebugStr(", SSTS: ");
    DebugU16((u16)(((u64)&Ctrl->Regs->PortRegisters[0].PortSerialATAStatus) - ((u64)Ctrl->Regs)));
    DebugStr(", SCTL: ");
    DebugU16((u16)(((u64)&Ctrl->Regs->PortRegisters[0].PortSerialATAControl) - ((u64)Ctrl->Regs)));
    DebugChar('\n');
    DebugStr("\t, SERR: ");
    DebugU16((u16)(((u64)&Ctrl->Regs->PortRegisters[0].PortSerialATAError) - ((u64)Ctrl->Regs)));
    DebugStr(", SACT: ");
    DebugU16((u16)(((u64)&Ctrl->Regs->PortRegisters[0].PortSerialATAActive) - ((u64)Ctrl->Regs)));
    DebugStr(", CI: ");
    DebugU16((u16)(((u64)&Ctrl->Regs->PortRegisters[0].PortCommandIssue) - ((u64)Ctrl->Regs)));
    DebugStr(", SNTF: ");
    DebugU16((u16)(((u64)&Ctrl->Regs->PortRegisters[0].PortSerialATANotification) - ((u64)Ctrl->Regs)));
    DebugChar('\n');
    DebugStr("\t, FBS: ");
    DebugU16((u16)(((u64)&Ctrl->Regs->PortRegisters[0].PortFISBasedSwitchingControl) - ((u64)Ctrl->Regs)));
    DebugStr(", DEVSLP: ");
    DebugU16((u16)(((u64)&Ctrl->Regs->PortRegisters[0].PortDeviceSleep) - ((u64)Ctrl->Regs)));
    DebugStr(" }\n\n");
    DebugStr("PortRegisters[1] Start: ");
    DebugU64((u64)&Ctrl->Regs->PortRegisters[1]);
    DebugStr(" (should be Port0 + 0x80)\n");
    DebugStr("Delta Port1-Port0: 0x");
    DebugU16((u16)(((u64)&Ctrl->Regs->PortRegisters[1]) - ((u64)&Ctrl->Regs->PortRegisters[0])));
    DebugChar('\n');

    DebugStr("Performing HBA reset...\n");
    for (u8 i = 0; i < 32; i++) {
        Ctrl->Regs->PortRegisters[i].PortInterruptEnable.U = 0;
    }
    Ctrl->Regs->GenericHostControl.GlobalHBAControl.rwAHCIEnable = 1;
    __asm__ volatile("mfence" ::: "memory");
    SystemBusySleepMs(10);
    Ctrl->Regs->GenericHostControl.GlobalHBAControl.rwHBAReset = 1;
    __asm__ volatile("mfence" ::: "memory");
    SystemBusySleepMs(10);
    DebugStr("Wait For TimeOut(int timeout=100727;(WYSI!!!!! 727!!!!)\n");
    int timeout = 100727;
    while (Ctrl->Regs->GenericHostControl.GlobalHBAControl.rwHBAReset && timeout--) {
        __asm__ volatile("pause" ::: "memory");
    }
    if (timeout == 0) {
        DebugStr("HBA Reset Timeout!\n");
        return NULL;
    }
    Ctrl->Regs->GenericHostControl.GlobalHBAControl.rwAHCIEnable = 1;
    __asm__ volatile("mfence" ::: "memory");
    SystemBusySleepMs(100);
    DebugStr(" GHC after reset: 0x");
    DebugU32(Ctrl->Regs->GenericHostControl.GlobalHBAControl.U);
    DebugChar('\n');
    DebugStr(" CAP: 0x");
    DebugU32(Ctrl->Regs->GenericHostControl.HostCapabilities.U);
    DebugChar('\n');

    u32 ports_impl = Ctrl->Regs->GenericHostControl.PortsImplemented.roPortImplemented;
    DebugStr("Ports Implemented: 0x"); DebugU32(ports_impl); DebugChar('\n');

    u8 ActivePortIdx = 0xFF;

    for (u8 Idx = 0; Idx < 32; Idx++) {
        if (!(ports_impl & (1 << Idx))) {
            continue;
        }
        DebugStr("Initializing Port "); DebugU16(Idx); DebugStr("...\n");

        Ctrl->Regs->PortRegisters[Idx].PortCommandAndStatus.rwStart = 0;
        Ctrl->Regs->PortRegisters[Idx].PortCommandAndStatus.rwFISReceiveEnable = 0;
        __asm__ volatile("mfence" ::: "memory");
        SystemBusySleepMs(1);
        timeout = 100727;
        while ((Ctrl->Regs->PortRegisters[Idx].PortCommandAndStatus.roCommandListRunning ||
                Ctrl->Regs->PortRegisters[Idx].PortCommandAndStatus.roFISReceiveRunning) && timeout--) {
            __asm__ volatile("pause" ::: "memory");
            SystemBusySleepUs(10);
        }
        if (timeout <= 0) {
            DebugStr(" Stop timeout, force continue...\n");
        }

        u64 fis_p = (u64)AlignedAlloc(&KernelPool, 256, 256);
        u64 cmd_p = (u64)AlignedAlloc(&KernelPool, 1024, 1024);
        if (!fis_p || !cmd_p) {
            DebugStr(" Alloc failed for Port "); DebugU16(Idx); DebugChar('\n');
            continue;
        }
        MemSet((u8*)fis_p, 0, 256);
        MemSet((u8*)cmd_p, 0, 1024);

        volatile u32* slots_dw = (volatile u32*)cmd_p;
        for (int s = 0; s < 32; s++) {
            u64 cmd_table_phys = (u64)AlignedAlloc(&KernelPool, 256, 128);
            if (cmd_table_phys) {
                MemSet((void*)cmd_table_phys, 0, 256);
                slots_dw[s * 8 + 2] = (u32)cmd_table_phys;
                slots_dw[s * 8 + 3] = (u32)(cmd_table_phys >> 32);
            }
        }

        Ctrl->Regs->PortRegisters[Idx].PortCommandListBaseAddress.U = (u32)cmd_p;
        Ctrl->Regs->PortRegisters[Idx].PortCommandListBaseAddressUpper32Bits.U = (u32)(cmd_p >> 32);
        Ctrl->Regs->PortRegisters[Idx].PortFISBaseAddress.U = (u32)fis_p;
        Ctrl->Regs->PortRegisters[Idx].PortFISBaseAddressUpper32Bits.U = (u32)(fis_p >> 32);
        __asm__ volatile("mfence" ::: "memory");
        DebugStr(" cmd_p: 0x"); DebugU64(cmd_p); DebugChar('\n');
        DebugStr(" fis_p: 0x"); DebugU64(fis_p); DebugChar('\n');

        Ctrl->Regs->PortRegisters[Idx].PortSerialATAError.U = 0xFFFFFFFF;
        Ctrl->Regs->PortRegisters[Idx].PortInterruptStatus.U = 0xFFFFFFFF;
        Ctrl->Regs->PortRegisters[Idx].PortInterruptEnable.U = (1U << 0) | (1U << 30);
        __asm__ volatile("mfence" ::: "memory");

        DebugStr(" Enabling FRE...\n");
        Ctrl->Regs->PortRegisters[Idx].PortCommandAndStatus.rwFISReceiveEnable = 1;
        __asm__ volatile("mfence" ::: "memory");
        timeout = 200727;
        while (!Ctrl->Regs->PortRegisters[Idx].PortCommandAndStatus.roFISReceiveRunning && timeout--) {
            __asm__ volatile("pause" ::: "memory");
            SystemBusySleepUs(10);
        }
        if (timeout == 0) {
            DebugStr(" FRE timeout!\n");
            continue;
        }
        DebugStr(" FRE OK\n");

        DebugStr(" Issuing COMRESET...\n");
        Ctrl->Regs->PortRegisters[Idx].PortSerialATAControl.rwDeviceDetectionInitialization = 0x1;
        __asm__ volatile("mfence" ::: "memory");
        SystemBusySleepMs(10);
        Ctrl->Regs->PortRegisters[Idx].PortSerialATAControl.rwDeviceDetectionInitialization = 0x0;
        __asm__ volatile("mfence" ::: "memory");
        SystemBusySleepMs(50);

        Ctrl->Regs->PortRegisters[Idx].PortInterruptStatus.U = 0xFFFFFFFF;
        __asm__ volatile("mfence" ::: "memory");

        SystemBusySleepMs(10);
        
        u32 ssts_check = Ctrl->Regs->PortRegisters[Idx].PortSerialATAStatus.U;
        if ((ssts_check & 0x0F) == 0) {
            SystemBusySleepMs(5);
            ssts_check = Ctrl->Regs->PortRegisters[Idx].PortSerialATAStatus.U;
            if ((ssts_check & 0x0F) == 0) {
                DebugStr(" Port empty, skip.\n");
                continue;
            }
        }

        timeout = 100727;
        u32 ssts = 0;
        u32 det = 0, ipm = 0, spd = 0;
        DebugStr(" Waiting for PhyReady...\n");
        int phy_ready = 0;
        while (timeout--) {
            ssts = Ctrl->Regs->PortRegisters[Idx].PortSerialATAStatus.U;
            det = ssts & 0x0F;
            ipm = (ssts >> 8) & 0x0F;
            spd = (ssts >> 4) & 0x0F;
            if (det == 3 && ipm == 1) {
                DebugStr(" PHY Ready! DET=3 IPM=1 SPD=");
                DebugU32(spd); DebugStr("\n");
                phy_ready = 1;
                break;
            }
            if (Ctrl->Regs->PortRegisters[Idx].PortInterruptStatus.U & 0x01) {
                Ctrl->Regs->PortRegisters[Idx].PortInterruptStatus.U = 0x01;
                __asm__ volatile("mfence" ::: "memory");
            }
            if ((timeout % 100000) == 0) {
                DebugStr(" Waiting... DET="); DebugU32(det);
                DebugStr(" IPM="); DebugU32(ipm); DebugChar('\n');
            }
            __asm__ volatile("pause" ::: "memory");
            SystemBusySleepUs(10);
        }
        DebugStr(" SSTS after wait: 0x"); DebugU32(ssts); DebugChar('\n');
        if (!phy_ready) {
            DebugStr(" PhyReady timeout! DET="); DebugU32(det);
            DebugStr(" IPM="); DebugU32(ipm); DebugStr("\n");
            continue;
        }

        Ctrl->Regs->PortRegisters[Idx].PortSerialATAError.U = 0xFFFFFFFF;
        Ctrl->Regs->PortRegisters[Idx].PortInterruptStatus.U = 0xFFFFFFFF;
        __asm__ volatile("mfence" ::: "memory");
        SystemBusySleepMs(5);

        DebugStr(" Waiting for BSY/DRQ clear...\n");
        timeout = 727727;
        while ((Ctrl->Regs->PortRegisters[Idx].PortTaskFileData.U & 0x88) && timeout--) {
            __asm__ volatile("pause" ::: "memory");
            SystemBusySleepUs(10);
        }
        u32 tfd = Ctrl->Regs->PortRegisters[Idx].PortTaskFileData.U;
        DebugStr(" PxTFD after wait: 0x"); DebugU32(tfd); DebugChar('\n');
        if (timeout <= 0) {
            DebugStr(" BSY/DRQ timeout after COMRESET!\n");
            continue;
        }
        DebugStr(" BSY/DRQ cleared\n");

        DebugStr(" Enabling ST...\n");
        Ctrl->Regs->PortRegisters[Idx].PortCommandAndStatus.rwStart = 1;
        __asm__ volatile("mfence" ::: "memory");
        timeout = 20727;
        while (!Ctrl->Regs->PortRegisters[Idx].PortCommandAndStatus.roCommandListRunning && timeout--) {
            __asm__ volatile("pause" ::: "memory");
            SystemBusySleepUs(10);
        }
        if (timeout == 0) {
            DebugStr(" ST timeout!\n");
            continue;
        }
        DebugStr(" ST OK\n");

        u32 sig = Ctrl->Regs->PortRegisters[Idx].PortSignature.U;
        DebugStr(" SIG: 0x"); DebugU32(sig); DebugStr(" (");
        if (sig==0x00000101){
            DebugStr("ATA");
            ActivePortIdx = Idx;
        }
        else if (sig==0xEB140101){
            DebugStr("ATAPI");
        }
        else if (sig==0x96690101){
            DebugStr("PM");
        }
        else if (sig==0xC33C0101){
            DebugStr("Enclosure");
        }
        else{
            DebugStr("Unknown");
        }
        DebugStr(")\n");
        DebugStr(" PxSERR: 0x"); DebugU32(Ctrl->Regs->PortRegisters[Idx].PortSerialATAError.U); DebugChar('\n');
        DebugStr(" PxIS: 0x"); DebugU32(Ctrl->Regs->PortRegisters[Idx].PortInterruptStatus.U); DebugChar('\n');
        DebugStr(" PxTFD: 0x"); DebugU32(Ctrl->Regs->PortRegisters[Idx].PortTaskFileData.U); DebugChar('\n');
        sig = Ctrl->Regs->PortRegisters[Idx].PortSignature.U;
        if (sig == 0x00000101) {
            ActivePortIdx = Idx;
            Ctrl->PortSlotWork[Idx] = 1;
        }
        Ctrl->SoftPortImplemented++;
    }
    DebugStr("SoftPortImplemented: "); DebugU64(Ctrl->SoftPortImplemented); DebugChar('\n');

    if (ActivePortIdx != 0xFF) {
        u8 p = ActivePortIdx;
        DebugStr("\n--- Starting IDENTIFY on Port "); DebugU16(p); DebugStr(" ---\n");
        SystemBusySleepMs(20);
        
        u8* BufferTmp = (u8*)AlignedAlloc(&KernelPool, 512, 256);
        if (!BufferTmp) {
            DebugStr("IDENTIFY alloc failed!\n");
            GlobalAHCICtrl = Ctrl;
            return Ctrl;
        }
        MemSet(BufferTmp, 0, 512);

        u64 cmd_list = (u64)Ctrl->Regs->PortRegisters[p].PortCommandListBaseAddress.U |
                            ((u64)Ctrl->Regs->PortRegisters[p].PortCommandListBaseAddressUpper32Bits.U << 32);
        volatile u32* slot_dw = (volatile u32*)cmd_list;

        u64 CmdTablePhys = ((u64)slot_dw[3] << 32) | slot_dw[2];

        slot_dw[0] = 5U | (1U << 16);
        slot_dw[1] = 0;

        volatile u8* fis_raw = (volatile u8*)CmdTablePhys;
        fis_raw[0] = 0x27;
        fis_raw[1] = 0x80;
        fis_raw[2] = 0xEC;
        fis_raw[7] = 0x40;

        volatile u32* prdt_dw = (volatile u32*)(CmdTablePhys + 0x80);
        prdt_dw[0] = (u32)(u64)BufferTmp;
        prdt_dw[1] = (u32)((u64)BufferTmp >> 32);
        prdt_dw[2] = 0;
        prdt_dw[3] = ((512 - 1) & 0x3FFFFF) | (1U << 31);

        Ctrl->Regs->PortRegisters[p].PortSerialATAError.U = 0xFFFFFFFF;
        Ctrl->Regs->PortRegisters[p].PortInterruptStatus.U = 0xFFFFFFFF;
        
        DebugStr("Issuing IDENTIFY command...\n");
        Ctrl->Regs->PortRegisters[p].PortCommandIssue.rw1CommandsIssued = (1U << 0);
        __asm__ volatile("mfence" ::: "memory");
        
        timeout = 200727;
        while ((Ctrl->Regs->PortRegisters[p].PortCommandIssue.rw1CommandsIssued & 0x1) && timeout--) {
            __asm__ volatile("pause" ::: "memory");
            SystemBusySleepUs(10);
        }

        if (timeout <= 0) {
            DebugStr("PxCI Timeout!\n");
        } else {
            DebugStr("Command Executed successfully.\n");
        }
        u32 is = Ctrl->Regs->PortRegisters[p].PortInterruptStatus.U;
        DebugStr("PxIS: 0x"); DebugU32(is); DebugChar('\n');
        Ctrl->Regs->PortRegisters[p].PortInterruptStatus.U = is;
        __asm__ volatile("mfence" ::: "memory");
        u32 tfd_after = Ctrl->Regs->PortRegisters[p].PortTaskFileData.U;
        DebugStr("PxTFD: 0x"); DebugU32(tfd_after); DebugChar('\n');
        if (tfd_after & 0x01) {
            DebugStr("Task File Error!\n");
        }
        DebugStr("PRDBC: 0x"); DebugU32(slot_dw[1]); DebugChar('\n');
        DebugStr("Model Name: ");
        u16* IdWords = (u16*)BufferTmp;
        for (u64 i = 27; i < 47; i++) {
            u16 Word = IdWords[i];
            
            char c1 = (char)(Word >> 8);
            char c2 = (char)(Word & 0xFF);

            if (c1 >= 0x20 && c1 <= 0x7E) {
                DebugChar(c1);
            }
            if (c2 >= 0x20 && c2 <= 0x7E) {
                DebugChar(c2);
            }
        }
        DebugChar('\n');
        DebugStr("--- IDENTIFY Complete ---\n");

        AHCIProcessReceivedFIS(Ctrl, p);
    } else {
        DebugStr("No ATA device found.\n");
    }

    GlobalAHCICtrl = Ctrl;
    return Ctrl;
}

int AHCISendMessage(AHCIController* controller, u8 port_idx, u8 slot, u64 lba, u16 count, void* buffer, u8 is_write) {
    if (!controller || !controller->Regs || port_idx >= 32) {
        DebugStr("[AHCI Error] (!controller || !controller->Regs || port_idx >= 32)==1\n");
        return -1;
    }

    volatile AHCIPortRegister* port = &controller->Regs->PortRegisters[port_idx];

    u64 Clb = ((u64)port->PortCommandListBaseAddressUpper32Bits.roCommandListBaseAddressUpper << 32) |
              ((u64)port->PortCommandListBaseAddress.rwCommandListBaseAddress << 10);
    
    volatile AHCICommandSlotEntry* slot_entry = &((volatile AHCICommandSlotEntry*)Clb)[slot];
    u64 cmd_table_base = ((u64)slot_entry->CommandTableBaseAddressUpper32Bits << 32) | slot_entry->CommandTableBaseAddress;

    if (!cmd_table_base) {
        DebugStr("[AHCI Error] Command Table Base is NULL for Slot ");
        DebugU8(slot);
        DebugChar('\n');
        return -1;
    }

    MemSet((u8*)cmd_table_base, 0, 128);

    AHCIFISEntry* fis = (AHCIFISEntry*)cmd_table_base;
    fis->FISType = 0x27;
    fis->C = 1;
    fis->Command = is_write ? 0x35 : 0x25;
    
    fis->LBALow     = (u8)(lba & 0xFF);
    fis->LBAMid     = (u8)((lba >> 8) & 0xFF);
    fis->LBAHigh    = (u8)((lba >> 16) & 0xFF);
    fis->Device     = 1 << 6;
    fis->LBALowExp  = (u8)((lba >> 24) & 0xFF);
    fis->LBAMidExp  = (u8)((lba >> 32) & 0xFF);
    fis->LBAHighExp = (u8)((lba >> 40) & 0xFF);

    fis->SectorCount    = (u8)(count & 0xFF);
    fis->SectorCountExp = (u8)((count >> 8) & 0xFF);

    volatile AHCIPRDTEntry* prdt = (volatile AHCIPRDTEntry*)(cmd_table_base + 0x80);
    u64 Buffer = (u64)buffer;
    
    prdt[0].DataBaseAddress = (u32)(Buffer & 0xFFFFFFFF);
    prdt[0].DataBaseAddressUpper32Bits = (u32)(Buffer >> 32);
    prdt[0].DBC = (count * 512) - 1;
    prdt[0].I = 1;

    u32 cfl = sizeof(AHCIFISEntry) / sizeof(u32);
    u32 flags = cfl & 0x1F;
    if (is_write) flags |= (1 << 6);
    flags |= (1 << 16);
    
    slot_entry->DescriptionInformation = flags;
    slot_entry->PRDByteCount = 0;

    u32 spin = 0;
    while ((port->PortTaskFileData.BSY || port->PortTaskFileData.DRQ) && spin < 1000000) {
        spin++;
    }
    if (spin >= 1000000) return -1;

    u64 start_time = SystemGetTimeUs();

    port->PortCommandIssue.rw1CommandsIssued = (1 << slot);

    while (1) {
        if (!(port->PortCommandIssue.rw1CommandsIssued & (1 << slot))) {
            break;
        }

        if (port->PortInterruptStatus.rwcTaskFileErrorStatus) {
            DebugStr("port->PortInterruptStatus.rwcTaskFileErrorStatus == 1\n");
            return -1;
        }

        if (SystemGetTimeUs() - start_time > 500000) {
            DebugStr("[AHCI] Command Timeout! Hardware hung.\n");
            return -1;
        }

        __asm__ volatile("pause");
    }

    controller->PortFISSlot[port_idx] = (controller->PortFISSlot[port_idx] + 1) % 32;
    AHCIProcessReceivedFIS(controller, port_idx);

    return 0;
}