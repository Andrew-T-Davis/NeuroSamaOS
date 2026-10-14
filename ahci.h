#pragma once

#include "int.h"

#define DefineAHCINULL 0
#define DefineAHCIMSIX 1
#define DefineAHCIMSI  2
#define DefineAHCIINTx 3

typedef struct {
    union {
        u32 U;
        struct {
            u32 roNumberOfPorts                          : 5;
            u32 roSupportsExternalSATA                   : 1;
            u32 roEnclosureManagementSupported           : 1;
            u32 roCommandCompletionCoalescingSupported   : 1;
            u32 roNumberOfCommandSlots                   : 5;
            u32 roPartialStateCapable                    : 1;
            u32 roSlumberStateCapable                    : 1;
            u32 roPIOMultipleDRQBlock                    : 1;
            u32 roFISBasedSwitchingSupported             : 1;
            u32 roSupportsAHCIModeOnly                   : 1;
            u32 roReserved0                              : 1;
            u32 roInterfaceSpeedSupport                  : 4;
            u32 roSupportsCommandListOverride            : 1;
            u32 roSupportsActivityLED                    : 1;
            u32 roSupportsAggressiveLinkPowerManagement  : 1;
            u32 roSupportsStaggeredSpinUp                : 1;
            u32 roSupportsMechanicalPresenceSwitch       : 1;
            u32 roSupportsSNotificationRegister          : 1;
            u32 roSupportsNativeCommandQueuing           : 1;
            u32 roSupports64BitAddressin                 : 1;
        };
    };
} __attribute__((packed)) AHCIRegHostCapabilities;

typedef struct {
    union {
        u32 U;
        struct {
            u32 rwHBAReset                 : 1;
            u32 rwInterruptEnable          : 1;
            u32 roMSIRevertToSingleMessage : 1;
            u32 roReserved0                : 28;
            u32 rwAHCIEnable               : 1;
        };
    };
} __attribute__((packed)) AHCIRegGlobalHBAControl;

typedef struct {
    union {
        u32 U;
        u32 rwcInterruptPendingStatus;
    };
} __attribute__((packed)) AHCIRegInterruptStatus;

typedef struct {
    union {
        u32 U;
        u32 roPortImplemented;
    };
} __attribute__((packed)) AHCIRegPortsImplemented;

typedef struct {
    union {
        u32 U;
        struct {
            u32 roMinorVersionNumber : 16;
            u32 roMajorVersionNumber : 16;
        };
    };
} __attribute__((packed)) AHCIRegAHCIVersion;

typedef struct {
    union {
        u32 U;
        struct {
            u32 rwEnable             : 1;
            u32 roReserved0          : 2;
            u32 roInterrupt          : 5;
            u32 rwCommandCompletions : 8;
            u32 rwTimeoutValue       : 16;
        };
    };
} __attribute__((packed)) AHCIRegCommandCompletionCoalescingControl;

typedef struct {
    union {
        u32 U;
        u32 rwPorts;
    };
} __attribute__((packed)) AHCIRegCommandCompletionCoalescingPorts;

typedef struct {
    union {
        u32 U;
        struct {
            u32 roBufferSize : 16;
            u32 roOffset     : 16;
        };
    };
} __attribute__((packed)) AHCIRegEnclosureManagementLocation;

typedef struct {
    union {
        u32 U;
        struct {
            u32 rwcMessageReceived                 : 1;
            u32 roReserved0                        : 7;
            u32 rw1TransmitMessage                 : 1;
            u32 rw1Reset                           : 1;
            u32 roReserved1                        : 6;
            u32 roLEDMessageTypes                  : 1;
            u32 roSAFTEEnclosureManagementMessages : 1;
            u32 roSES2EnclosureManagementMessages  : 1;
            u32 roSGPIOEnclosureManagementMessages : 1;
            u32 roReserved2                        : 4;
            u32 roSingleMessageBuffer              : 1;
            u32 roTransmitOnly                     : 1;
            u32 roActivityLEDHardwareDriven        : 1;
            u32 roPortMultiplierSupport            : 1;
            u32 roReserved3                        : 4;
        };
    };
} __attribute__((packed)) AHCIRegEnclosureManagementControl;

typedef struct {
    union {
        u32 U;
        struct {
            u32 roBIOSOrOSHandoff                         : 1;
            u32 roNVMHCIPresent                           : 1;
            u32 roAutomaticPartialToSlumberTransitions    : 1;
            u32 roSupportsDeviceSleep                     : 1;
            u32 roSupportsAggressiveDeviceSleepManagement : 1;
            u32 roDevSleepEntrancefromSlumberOnly         : 1;
            u32 roReserved0                               : 26;
        };
    };
} __attribute__((packed)) AHCIRegHBACapabilitiesExtended;

typedef struct {
    union {
        u32 U;
        struct {
            u32 rwBIOSOwnedSemaphore : 1;
            u32 rwOSOwnedSemaphore   : 1;
            u32 rwcOSOwnerShipChange : 1;
            u32 rwBIOSBusy           : 1;
            u32 roReserved0          : 27;
        };
    };
} __attribute__((packed)) AHCIRegBIOSOrOSHandoffControlAndStatus;

typedef struct {
    union {
        u32 U;
        struct {
            u32 roReserved0              : 10;
            u32 rwCommandListBaseAddress : 22;
        };
    };
} __attribute__((packed)) AHCIRegPortCommandListBaseAddress;

typedef struct {
    union {
        u32 U;
        u32 roCommandListBaseAddressUpper : 32;
    };
} __attribute__((packed)) AHCIRegPortCommandListBaseAddressUpper32Bits;

typedef struct {
    union {
        u32 U;
        struct {
            u32 roReserved       : 8;
            u32 rwFISBaseAddress : 24;
        };
    };
} __attribute__((packed)) AHCIRegPortFISBaseAddress;

typedef struct {
    union {
        u32 U;
        u32 roFISBaseAddressUpper32Bits;
    };
} __attribute__((packed)) AHCIRegPortFISBaseAddressUpper32Bits;

typedef struct {
    union {
        u32 U;
        struct {
            u32 rwcDeviceToHostRegisterFISInterrupt : 1;
            u32 rwcPIOSetupFISInterrupt             : 1;
            u32 rwcDMASetupFISInterrupt             : 1;
            u32 rwcSetDeviceBitsInterrupt           : 1;
            u32 roUnknownFISInterrupt               : 1;
            u32 rwcDescriptorProcessed              : 1;
            u32 roPortConnectChangeStatus           : 1;
            u32 rwcDeviceMechanicalPresenceStatus   : 1;
            u32 roReserved0                         : 14;
            u32 roPhyRdyChangeStatus                : 1;
            u32 rwcIncorrectPortMultiplierStatus    : 1;
            u32 rwcOverflowStatus                   : 1;
            u32 roReserved1                         : 1;
            u32 rwcInterfaceNonFatalErrorStatus     : 1;
            u32 rwcInterfaceFatalErrorStatus        : 1;
            u32 rwcHostBusDataErrorStatus           : 1;
            u32 rwcHostBusFatalErrorStatus          : 1;
            u32 rwcTaskFileErrorStatus              : 1;
            u32 rwcColdPortDetectStatus             : 1;
        };
    };
} __attribute__((packed)) AHCIRegPortInterruptStatus;

typedef struct {
    union {
        u32 U;
        struct {
            u32 rwDeviceToHostRegisterFISInterruptEnable : 1;
            u32 rwPIOSetupFISInterruptEnable             : 1;
            u32 rwDMASetupFISInterruptEnable             : 1;
            u32 rwSetDeviceBitsInterruptEnable           : 1;
            u32 rwUnknownFISInterruptEnable              : 1;
            u32 rwDescriptorProcessedInterruptEnable     : 1;
            u32 rwPortChangeInterruptEnable              : 1;
            u32 rwDeviceMechanicalPresenceEnable         : 1;
            u32 roReserved0                              : 14;
            u32 rwPhyRdyChangeInterruptEnable            : 1;
            u32 rwIncorrectPortMultiplierEnable          : 1;
            u32 rwOverflowEnable                         : 1;
            u32 roReserved1                              : 1;
            u32 rwInterfaceNonFatalErrorEnable           : 1;
            u32 rwInterfaceFatalErrorEnable              : 1;
            u32 rwHostBusDataErrorEnable                 : 1;
            u32 rwHostBusFatalErrorEnable                : 1;
            u32 rwTaskFileErrorEnable                    : 1;
            u32 rwColdPresenceDetectEnable               : 1;
        };
    };
} __attribute__((packed)) AHCIRegPortInterruptEnable;

typedef struct {
    union {
        u32 U;
        struct {
            u32 rwStart                                       : 1;
            u32 roSpinUpDevice                                : 1;
            u32 roPowerOnDevice                               : 1;
            u32 rw1CommandListOverride                        : 1;
            u32 rwFISReceiveEnable                            : 1;
            u32 roReserved0                                   : 3;
            u32 roCurrentCommandSlot                          : 5;
            u32 roMechanicalPresenceSwitchState               : 1;
            u32 roFISReceiveRunning                           : 1;
            u32 roCommandListRunning                          : 1;
            u32 roColdPresenceState                           : 1;
            u32 roPortMultiplierAttached                      : 1;
            u32 roHotPlugCapablePort                          : 1;
            u32 roMechanicalPresenceSwitchAttachedToPort      : 1;
            u32 roColdPresenceDetection                       : 1;
            u32 roExternalSATAPort                            : 1;
            u32 roFISBasedSwitchingCapablePort                : 1;
            u32 rwAutomaticPartialToSlumberTransitionsEnabled : 1;
            u32 rwDeviceIsATAPI                               : 1;
            u32 rwDriveLEDOnATAPIEnable                       : 1;
            u32 roAggressiveLinkPowerManagementEnable         : 1;
            u32 roAggressiveSlumberPartial                    : 1;
            u32 rwInterfaceCommunicationControl               : 4;
        };
    };
} __attribute__((packed)) AHCIRegPortCommandAndStatus;

typedef struct {
    union {
        u32 U;
        struct {
            u32 ERR : 1;
            u32 CS0 : 2;
            u32 DRQ : 1;
            u32 CS1 : 3;
            u32 BSY : 1;
            u32 Error : 8;
            u32 Reserved : 16;
        };
    };
} __attribute__((packed)) AHCIRegPortTaskFileData;

typedef struct {
    union {
        u32 U;
        union {
            u32 roSignature;
            struct {
                u32 SectorCountRegister : 8;
                u32 LBALowRegister      : 8;
                u32 LBAMidRegister      : 8;
                u32 LBAHighRegister     : 8;
            };
        };
    };
} __attribute__((packed)) AHCIRegPortSignature;

typedef struct {
    union {
        u32 U;
        struct {
            u32 roDeviceDetection          : 4;
            u32 roCurrentInterfaceSpeed    : 4;
            u32 roInterfacePowerManagement : 4;
            u32 roReserved0                : 20;
        };
    };
} __attribute__((packed)) AHCIRegPortSerialATAStatus;

typedef struct {
    union {
        u32 U;
        struct {
            u32 rwDeviceDetectionInitialization              : 4;
            u32 rwSpeedAllowed                               : 4;
            u32 rwInterfacePowerManagementTransitionsAllowed : 4;
            u32 roSelectPowerManagement                      : 4;
            u32 roPortMultiplierPort                         : 4;
            u32 roReserved0                                  : 11;
        };
    };
} __attribute__((packed)) AHCIRegPortSerialATAControl;

typedef struct {
    union {
        u32 U;
        struct {
            union {
                u16 rwcError;
                struct {
                    u16 RecoveredDataIntegrityError                 : 1;  
                    u16 RecoveredCommunicationsError                : 1;
                    u16 Reserved0                                   : 6;
                    u16 TransientDataIntegrityError                 : 1;
                    u16 PersistentCommunicationOrDataIntegrityError : 1;
                    u16 ProtocolError                               : 1;
                    u16 InternalError                               : 1;
                    u16 Reserved1                                   : 4;
                } Error;
            } __attribute__((packed));
            union {
                u16 rwcDiagnostics;
                struct {
                    u16 PhyRdyChange                                : 1;  
                    u16 PhyInternalError                            : 1;  
                    u16 CommWake                                    : 1;
                    u16 n10BTo8BDecodeError                         : 1;  
                    u16 CRCError                                    : 1;
                    u16 HandshakeError                              : 1;  
                    u16 RecoveredCommunicationsError                : 1;
                    u16 LinkSequenceError                           : 1;
                    u16 TransportStateTransitionError               : 1;
                    u16 UnknownFISType                              : 1;
                    u16 Exchanged                                   : 1;
                    u16 Reserved0                                   : 5;
                } Diagnostics;
            } __attribute__((packed));
        } __attribute__((packed));
    };
} __attribute__((packed)) AHCIRegPortSerialATAError;

typedef struct {
    union{
        u32 U;
        u32 rw1DeviceStatus;
    };
} __attribute__((packed)) AHCIRegPortSerialATAActive;

typedef struct {
    union{
        u32 U;
        u32 rw1CommandsIssued;
    };
} __attribute__((packed)) AHCIRegPortCommandIssue;

typedef struct {
    union {
        u32 U;
        struct {
            u32 rwcPMNotify : 16;
            u32 roReserved0 : 16;
        };
    };
} __attribute__((packed)) AHCIRegPortSerialATANotification;

typedef struct {
    union {
        u32 U;
        struct {
            u32 rwEnable                   : 1;
            u32 rw1DeviceErrorClear        : 1;
            u32 roSingleDeviceError        : 1;
            u32 roReserved0                : 4;
            u32 rwDeviceToIssue            : 4;
            u32 roActiveDeviceOptimization : 4;
            u32 roDeviceWithError          : 4;
            u32 roReserved1                : 12;
        };
    };
} __attribute__((packed)) AHCIRegPortFISBasedSwitchingControl;

typedef struct {
    union {
        u32 U;
        struct {
            u32 roAggressiveDeviceSleepEnable     : 1;
            u32 roDeviceSleepPresent              : 1;
            u32 roDeviceSleepExitTimeout          : 8;
            u32 roMinimumDeviceSleepAssertionTime : 5;
            u32 roDeviceSleepIdleTimeout          : 10;
            u32 roDITOMultiplier                  : 4;
            u32 roReserved0                       : 3;
        };
    };
} __attribute__((packed)) AHCIRegPortDeviceSleep;

typedef volatile struct {
    volatile AHCIRegPortCommandListBaseAddress            PortCommandListBaseAddress;
    volatile AHCIRegPortCommandListBaseAddressUpper32Bits PortCommandListBaseAddressUpper32Bits;
    volatile AHCIRegPortFISBaseAddress                    PortFISBaseAddress;
    volatile AHCIRegPortFISBaseAddressUpper32Bits         PortFISBaseAddressUpper32Bits;
    volatile AHCIRegPortInterruptStatus                   PortInterruptStatus;
    volatile AHCIRegPortInterruptEnable                   PortInterruptEnable;
    volatile AHCIRegPortCommandAndStatus                  PortCommandAndStatus;
    volatile u32                                          Reserved0;
    volatile AHCIRegPortTaskFileData                      PortTaskFileData;
    volatile AHCIRegPortSignature                         PortSignature;
    volatile AHCIRegPortSerialATAStatus                   PortSerialATAStatus;
    volatile AHCIRegPortSerialATAControl                  PortSerialATAControl;
    volatile AHCIRegPortSerialATAError                    PortSerialATAError;
    volatile AHCIRegPortSerialATAActive                   PortSerialATAActive;
    volatile AHCIRegPortCommandIssue                      PortCommandIssue;
    volatile AHCIRegPortSerialATANotification             PortSerialATANotification;
    volatile AHCIRegPortFISBasedSwitchingControl          PortFISBasedSwitchingControl;
    volatile AHCIRegPortDeviceSleep                       PortDeviceSleep;
    volatile u32                                          Reserved1[10];
    volatile u32                                          VendorSpecific[4];        
} __attribute__((packed)) AHCIPortRegister;

typedef volatile struct {
    volatile struct {
        volatile AHCIRegHostCapabilities                   HostCapabilities;
        volatile AHCIRegGlobalHBAControl                   GlobalHBAControl;
        volatile AHCIRegInterruptStatus                    InterruptStatus;
        volatile AHCIRegPortsImplemented                   PortsImplemented;
        volatile AHCIRegAHCIVersion                        AHCIVersion;
        volatile AHCIRegCommandCompletionCoalescingControl CommandCompletionCoalescingControl;
        volatile AHCIRegCommandCompletionCoalescingPorts   CommandCompletionCoalescingPorts;
        volatile AHCIRegEnclosureManagementLocation        EnclosureManagementLocation;
        volatile AHCIRegEnclosureManagementControl         EnclosureManagementControl;
        volatile AHCIRegHBACapabilitiesExtended            HBACapabilitiesExtended;
        volatile AHCIRegBIOSOrOSHandoffControlAndStatus    BIOSOrOSHandoffControlAndStatus;
        volatile u8 roReserved0[112];
        volatile char Vendor[100];
    } __attribute__((packed)) GenericHostControl;
    AHCIPortRegister PortRegisters[32];
} __attribute__((packed)) AHCIRegs;

typedef struct {
    u8  FISType;
    u8  PMPort : 4;
    u8  Reserved0 : 3; 
    u8  C : 1; 
    u8  Command;
    u8  FeaturesLow;
    u8  LBALow;
    u8  LBAMid;
    u8  LBAHigh;
    u8  Device;
    u8  LBALowExp;
    u8  LBAMidExp;
    u8  LBAHighExp;
    u8  FeaturesHigh;
    u8  SectorCount;
    u8  SectorCountExp;
    u8  Reserved1;
    u8  Control;
    u8  Reserved2[4];
} __attribute__((packed)) AHCIFISEntry;

typedef volatile struct {
    u32 DescriptionInformation;
    u32 PRDByteCount;
    u32 CommandTableBaseAddress;
    u32 CommandTableBaseAddressUpper32Bits;
    u32 Reserved[4];
} __attribute__((packed)) AHCICommandSlotEntry;

typedef volatile struct {
    u32 DataBaseAddress;
    u32 DataBaseAddressUpper32Bits;
    u32 Reserved0;
    u32 DBC : 22;
    u32 Reserved1 : 9;
    u32 I : 1;
} __attribute__((packed)) AHCIPRDTEntry;

typedef struct {
    u64 Base;
    u64 SoftPortImplemented;
    Bool PortSlotWork[32];
    u8 PortFISSlot[32];
    volatile AHCIRegs* Regs;
} __attribute__((packed)) AHCIController;

extern AHCIController* GlobalAHCICtrl;

u64 GetAHCIMMIO();
AHCIController* InitAHCI();
__attribute__((naked)) void AHCIInt();
int AHCISendMessage(AHCIController* controller, u8 port_idx, u8 slot, u64 lba, u16 count, void* buffer, u8 is_write);
void AHCIProcessReceivedFIS(AHCIController* controller, u8 port_idx);