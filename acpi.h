#pragma once

#include "int.h"
#include "efi.h"

#define DefineHPETBase         0xFED00000
#define DefineHPETIRQ          20
#define DefineHPETVector       0xEF

typedef struct {
    struct {
        u8 AddressSpaceID;
        u8 RegisterBitWidth;
        u8 RegisterBitOffset;
        u8 AccessSize;
    } Info;
    u64 Addr;
    u8 ResetValue;
} __attribute__((packed)) AcpiReset;

typedef struct {
    struct {
        u8 AddressSpaceID;
        u8 RegisterBitWidth;
        u8 RegisterBitOffset;
        u8 AccessSize;
    } Info;
    u64 Addr;
} __attribute__((packed)) AcpiShutDown;

typedef struct {
    u64 roCapabilitiesAndIDRegister;
    u64 Rsvd0;
    u64 rwConfigurationRegister;
    u64 Rsvd1;
    u64 rw1cInterruptStatusRegister;
    u64 Rsvd2[0x19];
    u64 rwCounterValueRegister;
    u64 Rsvd3;
    u64 rwTimer0ConfigurationAndCapabilityRegister;
    u64 rwTimer0ComparatorValueRegister;
    u64 rwTimer0FSBInterruptRouteRegister;
    u64 Rsvd4;
    u64 rwTimer1ConfigurationAndCapabilityRegister;
    u64 rwTimer1ComparatorValueRegister;
    u64 rwTimer1FSBInterruptRouteRegister;
    u64 Rsvd5;
    u64 rwTimer2ConfigurationAndCapabilityRegister;
    u64 rwTimer2ComparatorValueRegister;
    u64 rwTimer2FSBInterruptRouteRegister;
    u64 Rsvd6;
    u64 Rsvd7[0x54];
} __attribute__((packed)) AcpiHPETRegs;

typedef struct {
    struct {
        u8 AddressSpaceID;
        u8 RegisterBitWidth;
        u8 RegisterBitOffset;
        u8 AccessSize;
    } Info;
    u64 Address;
} __attribute__((packed)) HPETBaseAddress;

typedef struct {
    char     Signature[4];
    u32      Length;
    u8       Revision;
    u8       Checksum;
    char     OemId[6];
    char     OemTableId[8];
    u32      OemRevision;
    u32      CreatorId;
    u32      CreatorRevision;
    u32      LocalApicAddr;
    u32      Flags;
} __attribute__((packed)) MADTHeader;

typedef struct {
    u8 Type;
    u8 Length;
} __attribute__((packed)) MADTEntryHeader;

typedef struct {
    MADTEntryHeader Header;
    u8  AcpiProcessorId;
    u8  ApicId;
    u32 Flags;
} __attribute__((packed)) MADTLocalApic;

typedef struct {
    MADTEntryHeader Header;
    u16 Reserved;
    u32 X2ApicId;
    u32 Flags;
    u32 AcpiProcessorId;
} __attribute__((packed)) MADTLocalX2Apic;

extern u64 HPETFreqHz;
extern u64 HPET10MsTicks;

u64 GetAcpiAddrUEFI(EFI_BOOT_SERVICES* bs, u64 *Size, u64 *FADTAddr, u64 *DSDTAddr, u64* HPETAddr, u64* MADTAddr);
void ReBoot(u64 FADTAddr);
void ShutDown(u64 FADTAddr, u64 DSDTAddr);
u64 GetHPETMMIO(u64 HPETAddr);
void SetHPETMMIO(u64 HPETAddr);
void HPETISR(void);
void CalibrateHPET(void);
void StopOtherCores(void);