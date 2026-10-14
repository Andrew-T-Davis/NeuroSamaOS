#pragma once

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC
#define PCI_BRIDGE_PRIMARY_BUS    0x18
#define PCI_BRIDGE_SECONDARY_BUS  0x19
#define PCI_BRIDGE_SUBORDINATE_BUS 0x1A

#include "int.h"

u32 PCIAddr(u8 Bus, u8 Slot, u8 Func, u8 Offset);
u32 PCIReadDWORD(u8 Bus, u8 Slot, u8 Func, u8 Offset);
void PCIWriteDWORD(u8 Bus, u8 Slot, u8 Func, u8 Offset, u32 Value);
void PCIWriteWORD(u8 Bus, u8 Slot, u8 Func, u8 Offset, u16 Value);
u16 PCIReadWORD(u8 Bus, u8 Slot, u8 Func, u8 Offset);
u8 PCIReadBYTE(u8 Bus, u8 Slot, u8 Func, u8 Offset);
void PCIWriteBYTE(u8 Bus, u8 Slot, u8 Func, u8 Offset, u8 Value);
void PCIEnableDevice(u8 Bus, u8 Slot, u8 Func);
u64 PCIGetBARAddress(u8 Bus, u8 Slot, u8 Func, u8 BarIndex);
u64 PCIGetBARSize64(u8 Bus, u8 Slot, u8 Func, u8 BARIndex);
void PCIScanBus(void);
void PCIFindDeviceByClass(u8 TargetClass, u8 TargetSubClass, u8 TargetProgIF, u8 *OutBus, u8 *OutDev, u8 *OutFunc);
void PCIFindDeviceByClassList(u8 TargetClass[], u8 TargetSubClass[], u8 TargetProgIF[], u64 ListSize, u8 *OutBus, u8 *OutDev, u8 *OutFunc);
void PCIFindDeviceByID(u16 TargetVendor, u16 TargetDevice, u8 *OutBus, u8 *OutDev, u8 *OutFunc);
void PCIFindDeviceByIDAndClass(u16 TargetVendor, u16 TargetDevice, u8 TargetClass, u8 TargetSubClass, u8 TargetProgIF, u8 *OutBus, u8 *OutDev, u8 *OutFunc);
void PCIScanBusRecursive(u8 Bus);
u8 PCIReadBusNumber(u8 Bus, u8 Slot, u8 Func);
Bool PCIIsBridge(u8 Bus, u8 Slot, u8 Func);
u8 PCIGetPrimaryBus(u8 Bus, u8 Slot, u8 Func);
u8 PCIGetSecondaryBus(u8 Bus, u8 Slot, u8 Func);
u8 PCIGetSubordinateBus(u8 Bus, u8 Slot, u8 Func);
