#include "pci.h"
#include "int.h"
#include "port.h"
#include "debug.h"

u32 PCIAddr(u8 Bus, u8 Slot, u8 Func, u8 Offset)
{
    return (u32)((1 << 31) | ((u8)Bus << 16) | ((Slot & 0x1F) << 11) | ((Func & 0x07) << 8) | (Offset & 0xFC));
}

u32 PCIReadDWORD(u8 Bus, u8 Slot, u8 Func, u8 Offset)
{
    outl(PCI_CONFIG_ADDRESS, PCIAddr((u8)Bus, Slot, Func, Offset));
    return inl(PCI_CONFIG_DATA);
}

void PCIWriteDWORD(u8 Bus, u8 Slot, u8 Func, u8 Offset, u32 Value)
{
    outl(PCI_CONFIG_ADDRESS, PCIAddr((u8)Bus, Slot, Func, Offset));
    outl(PCI_CONFIG_DATA, Value);
}

void PCIWriteWORD(u8 Bus, u8 Slot, u8 Func, u8 Offset, u16 Value)
{
    u32 Addr = PCIAddr((u8)Bus, Slot, Func, Offset);
    outl(PCI_CONFIG_ADDRESS, Addr);
    u32 Old = inl(PCI_CONFIG_DATA);
    u32 Shift = (Offset & 0x2) * 8;
    u32 Mask = ~(0xFFFFU << Shift);
    u32 New = (Old & Mask) | ((u32)Value << Shift);
    outl(PCI_CONFIG_ADDRESS, Addr);
    outl(PCI_CONFIG_DATA, New);
}

u16 PCIReadWORD(u8 Bus, u8 Slot, u8 Func, u8 Offset)
{
    outl(PCI_CONFIG_ADDRESS, PCIAddr((u8)Bus, Slot, Func, Offset));
    u32 dword = inl(PCI_CONFIG_DATA);
    if (Offset & 0x2) {
        return (dword >> 16) & 0xFFFF;
    } else {
        return dword & 0xFFFF;
    }
}

u8 PCIReadBYTE(u8 Bus, u8 Slot, u8 Func, u8 Offset)
{
    outl(PCI_CONFIG_ADDRESS, PCIAddr((u8)Bus, Slot, Func, Offset));
    u32 dword = inl(PCI_CONFIG_DATA);
    u8 shift = (Offset & 0x3) * 8;
    return (dword >> shift) & 0xFF;
}

void PCIWriteBYTE(u8 Bus, u8 Slot, u8 Func, u8 Offset, u8 Value)
{
    u32 Addr = PCIAddr((u8)Bus, Slot, Func, Offset);
    outl(PCI_CONFIG_ADDRESS, Addr);
    u32 Old = inl(PCI_CONFIG_DATA);
    u32 Shift = (Offset & 0x3) * 8;
    u32 Mask = ~(0xFFU << Shift);
    u32 New = (Old & Mask) | ((u32)Value << Shift);
    outl(PCI_CONFIG_ADDRESS, Addr);
    outl(PCI_CONFIG_DATA, New);
}

void PCIEnableDevice(u8 Bus, u8 Slot, u8 Func)
{
    u16 Command = PCIReadWORD((u8)Bus, Slot, Func, 0x04);
    Command |= (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4);
    PCIWriteWORD((u8)Bus, Slot, Func, 0x04, Command);
    u16 CommandAfter = PCIReadWORD((u8)Bus, Slot, Func, 0x04);
    DebugStr("Command register after enable: 0x");
    DebugU16(CommandAfter);
    DebugChar('\n');
}

u64 PCIGetBARAddress(u8 Bus, u8 Slot, u8 Func, u8 BarIndex) {
    u32 BarLow = PCIReadDWORD((u8)Bus, Slot, Func, 0x10 + BarIndex * 4);
    
    if (BarLow & 0x1) {
        return (u64)(BarLow & (u32)(~0x3));
    }
    
    if ((BarLow & 0x6) == 0x4) {
        u32 barHigh = PCIReadDWORD((u8)Bus, Slot, Func, 0x10 + (BarIndex + 1) * 4);
        return ((u64)barHigh << 32) | ((u64)BarLow & ~0xFULL);
    }
    
    return (u64)BarLow & ~0xFULL;
}

u64 PCIGetBARSize64(u8 Bus, u8 Slot, u8 Func, u8 BARIndex)
{
    u32 OriginalBAR_Low = PCIReadDWORD((u8)Bus, Slot, Func, 0x10 + BARIndex * 4);
    u32 OriginalBAR_High = PCIReadDWORD((u8)Bus, Slot, Func, 0x10 + BARIndex * 4 + 4);
    
    PCIWriteDWORD((u8)Bus, Slot, Func, 0x10 + BARIndex * 4, 0xFFFFFFFF);
    PCIWriteDWORD((u8)Bus, Slot, Func, 0x10 + BARIndex * 4 + 4, 0xFFFFFFFF);
    
    u32 SizeMask_Low = PCIReadDWORD((u8)Bus, Slot, Func, 0x10 + BARIndex * 4);
    u32 SizeMask_High = PCIReadDWORD((u8)Bus, Slot, Func, 0x10 + BARIndex * 4 + 4);
    
    PCIWriteDWORD((u8)Bus, Slot, Func, 0x10 + BARIndex * 4, OriginalBAR_Low);
    PCIWriteDWORD((u8)Bus, Slot, Func, 0x10 + BARIndex * 4 + 4, OriginalBAR_High);
    
    u64 SizeMask = ((u64)SizeMask_High << 32) | (SizeMask_Low & 0xFFFFFFF0);
    u64 Size = (~SizeMask + 1) & 0xFFFFFFF0;
    
    return Size;
}

void PCIScanBus(void)
{
    for (u16 Bus = 0; Bus < 256; Bus++)
    {
        for (u8 Dev = 0; Dev < 32; Dev++)
        {
            for (u8 Func = 0; Func < 8; Func++)
            {
                u32 VendorDevice = PCIReadDWORD((u8)Bus, (u8)Dev, (u8)Func, 0x00);
                u16 Vendor = VendorDevice & 0xFFFF;
                
                if (Vendor == 0xFFFF)
                {
                    if (Func == 0)
                        break;
                    continue;
                }
                
                u32 ClassRev = PCIReadDWORD((u8)Bus, (u8)Dev, Func, 0x08);
                u8 ClassCode = (ClassRev >> 24) & 0xFF;
                u8 SubClass = (ClassRev >> 16) & 0xFF;
                u8 ProgIF = (ClassRev >> 8) & 0xFF;
                
                u32 Header = PCIReadDWORD((u8)Bus, (u8)Dev, Func, 0x0C);
                u8 HeaderType = (Header >> 16) & 0xFF;

                DebugStr("Found controller: Bus=");
                DebugU8((u8)Bus);
                DebugStr(" Dev=");
                DebugU8(Dev);
                DebugStr(" Func=");
                DebugU8(Func);
                DebugStr(" Vendor=0x");
                DebugU16(Vendor);
                DebugStr(" Class=0x");
                DebugU16(ClassCode);
                DebugStr(" SubClass=0x");
                DebugU8(SubClass);
                DebugStr(" ProgIF=0x");
                DebugU8(ProgIF);
                DebugStr(" HeaderType=0x");
                DebugU8(HeaderType);
                if (Vendor==0x1234){
                    if (ClassCode==0x0003){
                        if (SubClass==0x00) DebugStr(" Its VGA Controller btw\n");
                        else DebugChar('\n');
                    }else {
                        DebugChar('\n');
                    }
                }else if (Vendor==0x8086){
                    if (ClassCode==0x06){
                        if (SubClass==0x00) DebugStr(" Its Host Bridge btw\n");
                        else if (SubClass==0x01) DebugStr(" Its ISA Bridge btw\n");
                        else DebugChar('\n');
                    }else if (ClassCode==0x01){
                        if (SubClass==0x01) DebugStr(" Its IDE Controller btw\n");
                        else DebugChar('\n');
                    }else if (ClassCode==0x02){
                        if (SubClass==0x01) DebugStr(" Its IDE Controller btw\n");
                        else if (SubClass==0x00) DebugStr(" Its Ethernet Controller btw\n");
                        else DebugChar('\n');
                    }else if (ClassCode==0x04){
                        if (SubClass==0x01) DebugStr(" Its AC'97 btw\n");
                        else if (SubClass==0x03) DebugStr(" Its HDAudio btw\n");
                        else DebugChar('\n');
                    }
                }else if(Vendor==0x1B36){
                    if (ClassCode==0x0C){
                        if (SubClass==0x03){
                            if (ProgIF==0x30) DebugStr(" Its xHCI btw\n");
                            else if (ProgIF==0x20) DebugStr(" Its eHCI btw\n");
                            else if (ProgIF==0x10) DebugStr(" Its oHCI btw\n");
                            else DebugChar('\n');
                        }
                    }
                }else{
                    DebugChar('\n');
                }
                
                if (!(HeaderType & 0x80))
                    break;
            }
        }
    }
    DebugChar('\n');
}

void PCIFindDeviceByClass(u8 TargetClass, u8 TargetSubClass, u8 TargetProgIF, u8 *OutBus, u8 *OutDev, u8 *OutFunc)
{
    *OutBus = 0xFF;
    *OutDev = 0xFF;
    *OutFunc = 0xFF;
    
    for (u16 Bus = 0; Bus < 256; Bus++)
    {
        for (u16 Dev = 0; Dev < 32; Dev++)
        {
            u32 HeaderTypeReg = PCIReadDWORD((u8)Bus, (u8)Dev, 0, 0x0C);
            u8 HeaderType = (HeaderTypeReg >> 16) & 0xFF;
            u16 MaxFunc = (HeaderType & 0x80) ? 8 : 1;

            for (u16 Func = 0; Func < MaxFunc; Func++)
            {
                u32 VendorDevice = PCIReadDWORD((u8)Bus, (u8)Dev, (u8)Func, 0x00);
                u16 Vendor = VendorDevice & 0xFFFF;
                u16 Device = (VendorDevice >> 16) & 0xFFFF;
                
                if (Vendor == 0xFFFF) {
                    continue;
                }
                
                u32 ClassRev = PCIReadDWORD((u8)Bus, (u8)Dev, (u8)Func, 0x08);
                u8 ClassCode = (ClassRev >> 24) & 0xFF;
                u8 SubClass = (ClassRev >> 16) & 0xFF;
                u8 ProgIF = (ClassRev >> 8) & 0xFF;

                if (ClassCode == TargetClass && SubClass == TargetSubClass) {
                    DebugStr("Found controller: Bus=");
                    DebugU8((u8)Bus);
                    DebugStr(" Dev=");
                    DebugU8((u8)Dev);
                    DebugStr(" Func=");
                    DebugU8((u8)Func);
                    DebugStr(" Vendor=0x");
                    DebugU16(Vendor);
                    DebugStr(" Device=0x");
                    DebugU16(Device);
                    DebugStr(" ProgIF=0x");
                    DebugU8(ProgIF);
                    DebugChar('\n');
                    
                    if (ProgIF == TargetProgIF) {
                        PCIEnableDevice((u8)Bus, (u8)Dev, (u8)Func);
                        *OutBus = (u8)Bus;
                        *OutDev = (u8)Dev;
                        *OutFunc = (u8)Func;
                        return;
                    }
                }
            }
        }
    }
}

void PCIFindDeviceByClassList(u8 TargetClass[], u8 TargetSubClass[], u8 TargetProgIF[], u64 ListSize, u8 *OutBus, u8 *OutDev, u8 *OutFunc)
{
    *OutBus = 0xFF;
    *OutDev = 0xFF;
    *OutFunc = 0xFF;
    
    for (u16 Bus = 0; Bus < 256; Bus++)
    {
        for (u16 Dev = 0; Dev < 32; Dev++)
        {
            u32 HeaderTypeReg = PCIReadDWORD((u8)Bus, (u8)Dev, 0, 0x0C);
            u8 HeaderType = (HeaderTypeReg >> 16) & 0xFF;
            u16 MaxFunc = (HeaderType & 0x80) ? 8 : 1;

            for (u16 Func = 0; Func < MaxFunc; Func++)
            {
                u32 VendorDevice = PCIReadDWORD((u8)Bus, (u8)Dev, (u8)Func, 0x00);
                u16 Vendor = VendorDevice & 0xFFFF;
                u16 Device = (VendorDevice >> 16) & 0xFFFF;
                
                if (Vendor == 0xFFFF) {
                    continue;
                }
                
                u32 ClassRev = PCIReadDWORD((u8)Bus, (u8)Dev, (u8)Func, 0x08);
                u8 ClassCode = (ClassRev >> 24) & 0xFF;
                u8 SubClass = (ClassRev >> 16) & 0xFF;
                u8 ProgIF = (ClassRev >> 8) & 0xFF;

                for (u64 Idx=0;Idx<ListSize;Idx++){

                    if (ClassCode == TargetClass[Idx] && SubClass == TargetSubClass[Idx]) {
                        DebugStr("Found controller: Bus=");
                        DebugU8((u8)Bus);
                        DebugStr(" Dev=");
                        DebugU8((u8)Dev);
                        DebugStr(" Func=");
                        DebugU8((u8)Func);
                        DebugStr(" Vendor=0x");
                        DebugU16(Vendor);
                        DebugStr(" Device=0x");
                        DebugU16(Device);
                        DebugStr(" ProgIF=0x");
                        DebugU8(ProgIF);
                        DebugStr(" TargetClass=0x");
                        DebugU8(TargetClass[Idx]);
                        DebugStr(" TargetSubClass=0x");
                        DebugU8(TargetSubClass[Idx]);
                        DebugStr(" TargetProgIF=0x");
                        DebugU8(TargetProgIF[Idx]);
                        DebugChar('\n');
                        
                        if (ProgIF == TargetProgIF[Idx]) {
                            DebugStr("ProgIF==TargetProgIF.Enable.\n");
                            DebugStr("TargetProgIF=0x");
                            DebugU8(TargetProgIF[Idx]);
                            DebugChar('\n');
                            PCIEnableDevice((u8)Bus, (u8)Dev, (u8)Func);
                            *OutBus = (u8)Bus;
                            *OutDev = (u8)Dev;
                            *OutFunc = (u8)Func;
                            return;
                        }
                    }
                }
            }
        }
    }
}

void PCIFindDeviceByID(u16 TargetVendor, u16 TargetDevice, u8 *OutBus, u8 *OutDev, u8 *OutFunc)
{
    *OutBus = 0xFF;
    *OutDev = 0xFF;
    *OutFunc = 0xFF;
    
    for (u16 Bus = 0; Bus < 256; Bus++)
    {
        for (u16 Dev = 0; Dev < 32; Dev++)
        {
            u32 HeaderTypeReg = PCIReadDWORD((u8)Bus, (u8)Dev, 0, 0x0C);
            u8 HeaderType = (HeaderTypeReg >> 16) & 0xFF;
            u16 MaxFunc = (HeaderType & 0x80) ? 8 : 1;

            for (u16 Func = 0; Func < MaxFunc; Func++)
            {
                u32 VendorDevice = PCIReadDWORD((u8)Bus, (u8)Dev, (u8)Func, 0x00);
                u16 Vendor = VendorDevice & 0xFFFF;
                u16 Device = (VendorDevice >> 16) & 0xFFFF;
                
                if (Vendor == 0xFFFF) {
                    continue;
                }

                if (Vendor == TargetVendor && Device == TargetDevice) {
                    DebugStr("Found controller: Bus=");
                    DebugU8((u8)Bus);
                    DebugStr(" Dev=");
                    DebugU8((u8)Dev);
                    DebugStr(" Func=");
                    DebugU8((u8)Func);
                    DebugStr(" Vendor=0x");
                    DebugU16(Vendor);
                    DebugStr(" Device=0x");
                    DebugU16(Device);
                    DebugChar('\n');
                    
                    PCIEnableDevice((u8)Bus, (u8)Dev, (u8)Func);
                    *OutBus = (u8)Bus;
                    *OutDev = (u8)Dev;
                    *OutFunc = (u8)Func;
                    return;
                }
            }
        }
    }
}

void PCIFindDeviceByIDAndClass(u16 TargetVendor, u16 TargetDevice, u8 TargetClass, u8 TargetSubClass, u8 TargetProgIF, u8 *OutBus, u8 *OutDev, u8 *OutFunc)
{
    *OutBus = 0xFF;
    *OutDev = 0xFF;
    *OutFunc = 0xFF;
    
    for (u16 Bus = 0; Bus < 256; Bus++)
    {
        for (u16 Dev = 0; Dev < 32; Dev++)
        {
            u32 HeaderTypeReg = PCIReadDWORD((u8)Bus, (u8)Dev, 0, 0x0C);
            u8 HeaderType = (HeaderTypeReg >> 16) & 0xFF;
            u16 MaxFunc = (HeaderType & 0x80) ? 8 : 1;

            for (u16 Func = 0; Func < MaxFunc; Func++)
            {
                u32 VendorDevice = PCIReadDWORD((u8)Bus, (u8)Dev, (u8)Func, 0x00);
                u16 Vendor = VendorDevice & 0xFFFF;
                u16 Device = (VendorDevice >> 16) & 0xFFFF;
                
                if (Vendor == 0xFFFF) {
                    continue;
                }

                u32 ClassRev = PCIReadDWORD((u8)Bus, (u8)Dev, (u8)Func, 0x08);
                u8 ClassCode = (ClassRev >> 24) & 0xFF;
                u8 SubClassCode = (ClassRev >> 16) & 0xFF;
                u8 ProgIF = (ClassRev >> 8) & 0xFF;

                if (Vendor == TargetVendor && Device == TargetDevice && ClassCode == TargetClass && ClassCode == TargetClass && SubClassCode == TargetSubClass && ProgIF == TargetProgIF) {
                    DebugStr("Found controller: Bus=");
                    DebugU8((u8)Bus);
                    DebugStr(" Dev=");
                    DebugU8((u8)Dev);
                    DebugStr(" Func=");
                    DebugU8((u8)Func);
                    DebugStr(" Vendor=0x");
                    DebugU16(Vendor);
                    DebugStr(" Device=0x");
                    DebugU16(Device);
                    DebugChar('\n');
                    
                    PCIEnableDevice((u8)Bus, (u8)Dev, (u8)Func);
                    *OutBus = (u8)Bus;
                    *OutDev = (u8)Dev;
                    *OutFunc = (u8)Func;
                    return;
                }
            }
        }
    }
}

void PCIScanBusRecursive(u8 Bus)
{
    for (u8 Dev = 0; Dev < 32; Dev++)
    {
        for (u8 Func = 0; Func < 8; Func++)
        {
            u32 VendorDevice = PCIReadDWORD((u8)Bus, (u8)Dev, Func, 0x00);
            if ((VendorDevice & 0xFFFF) == 0xFFFF)
            {
                if (Func == 0) break;
                continue;
            }
            
            u32 ClassRev = PCIReadDWORD((u8)Bus, (u8)Dev, Func, 0x08);
            u8 ClassCode = (ClassRev >> 24) & 0xFF;
            u8 SubClass = (ClassRev >> 16) & 0xFF;
            
            if (ClassCode == 0x06 && SubClass == 0x04)
            {
                u32 SecondaryBus = PCIReadDWORD((u8)Bus, (u8)Dev, Func, 0x18);
                u8 SecondaryBusNum = (SecondaryBus >> 8) & 0xFF;
                PCIScanBusRecursive(SecondaryBusNum);
            }
        }
    }
}

u8 PCIReadBusNumber(u8 Bus, u8 Slot, u8 Func) {
    u32 busReg = PCIReadDWORD((u8)Bus, Slot, Func, 0x18);
    return (busReg >> 8) & 0xFF;
}

Bool PCIIsBridge(u8 Bus, u8 Slot, u8 Func) {
    u32 ClassRev = PCIReadDWORD((u8)Bus, (u8)Slot, (u8)Func, 0x08);
    u8 ClassCode = (ClassRev >> 24) & 0xFF;
    u8 SubClass = (ClassRev >> 16) & 0xFF;
    return (ClassCode == 0x06 && SubClass == 0x04);
}

u8 PCIGetPrimaryBus(u8 Bus, u8 Slot, u8 Func) {
    return PCIReadBYTE((u8)Bus, Slot, Func, PCI_BRIDGE_PRIMARY_BUS);
}

u8 PCIGetSecondaryBus(u8 Bus, u8 Slot, u8 Func) {
    return PCIReadBYTE((u8)Bus, Slot, Func, PCI_BRIDGE_SECONDARY_BUS);
}

u8 PCIGetSubordinateBus(u8 Bus, u8 Slot, u8 Func) {
    return PCIReadBYTE((u8)Bus, Slot, Func, PCI_BRIDGE_SUBORDINATE_BUS);
}