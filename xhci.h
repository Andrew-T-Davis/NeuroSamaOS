#pragma once

#include "int.h"

#define TRB_TYPE_NORMAL              1
#define TRB_TYPE_SETUP_STAGE         2
#define TRB_TYPE_DATA_STAGE          3
#define TRB_TYPE_STATUS_STAGE        4
#define TRB_TYPE_ISOCH               5
#define TRB_TYPE_LINK                6
#define TRB_TYPE_EVENT_DATA          7
#define TRB_TYPE_NO_OP               8

#define CMD_ENABLE_SLOT              1
#define CMD_DISABLE_SLOT             2
#define CMD_ADDRESS_DEVICE           11
#define CMD_CONFIGURE_ENDPOINT       12
#define CMD_EVALUATE_CONTEXT         13
#define CMD_RESET_ENDPOINT           14
#define CMD_STOP_ENDPOINT            15
#define CMD_SET_TR_DEQUEUE           16
#define CMD_RESET_DEVICE             17
#define CMD_FORCE_EVENT              18
#define CMD_NEGOTIATE_BANDWIDTH      19
#define CMD_SET_LATENCY_TOLERANCE    20
#define CMD_GET_PORT_BANDWIDTH       21
#define CMD_FORCE_HEADER             22

#define CC_SUCCESS                   1
#define CC_DATA_BUFFER_ERROR         2
#define CC_BABBLE_DETECTED           3
#define CC_TRB_ERROR                 4
#define CC_STALL                     5
#define CC_RESOURCE_ERROR            6
#define CC_BANDWIDTH_ERROR           7
#define CC_NO_SLOTS_AVAILABLE        8
#define CC_INVALID_STREAM_TYPE       9
#define CC_SLOT_NOT_ENABLED          10
#define CC_EP_NOT_ENABLED            11
#define CC_SHORT_PACKET              12
#define CC_RING_UNDERRUN             13
#define CC_RING_OVERRUN              14
#define CC_VF_EVENT_RING_FULL        15
#define CC_PARAMETER_ERROR           16
#define CC_BANDWIDTH_OVERRUN         17
#define CC_CONTEXT_STATE_ERROR       18
#define CC_NO_PING_RESPONSE          19
#define CC_EVENT_RING_FULL           20
#define CC_INCOMPATIBLE_DEVICE       21
#define CC_MISSED_SERVICE            22
#define CC_COMMAND_RING_STOPPED      23
#define CC_COMMAND_ABORTED           24
#define CC_STOPPED                  25
#define CC_STOPPED_LENGTH_INVALID   26
#define CC_MAX_EXIT_LATENCY_TOO_LARGE 27
#define CC_ISOCH_BUFFER_OVERRUN      28
#define CC_EVENT_LOST                29
#define CC_UNDEFINED_ERROR           30
#define CC_INVALID_STREAM_ID         31
#define CC_SECONDARY_BANDWIDTH_ERROR 32
#define CC_SPLIT_TRANSACTION_ERROR   33

#define USB_REQ_GET_STATUS           0x00
#define USB_REQ_CLEAR_FEATURE        0x01
#define USB_REQ_SET_FEATURE          0x03
#define USB_REQ_SET_ADDRESS          0x05
#define USB_REQ_GET_DESCRIPTOR       0x06
#define USB_REQ_SET_DESCRIPTOR       0x07
#define USB_REQ_GET_CONFIGURATION    0x08
#define USB_REQ_SET_CONFIGURATION    0x09
#define USB_REQ_GET_INTERFACE        0x0A
#define USB_REQ_SET_INTERFACE        0x0B
#define USB_REQ_SYNCH_FRAME          0x0C

#define DESC_TYPE_DEVICE             1
#define DESC_TYPE_CONFIGURATION      2
#define DESC_TYPE_STRING             3
#define DESC_TYPE_INTERFACE          4
#define DESC_TYPE_ENDPOINT           5
#define DESC_TYPE_DEVICE_QUALIFIER   6
#define DESC_TYPE_OTHER_SPEED        7
#define DESC_TYPE_INTERFACE_POWER    8

#define USB_SPEED_FULL               0
#define USB_SPEED_LOW                1
#define USB_SPEED_HIGH               2
#define USB_SPEED_SUPER              3
#define USB_SPEED_SUPER_PLUS         4

typedef volatile enum {
    PORT_STATE_DISABLED,
    PORT_STATE_CONNECTED,
    PORT_STATE_RESETTING,
    PORT_STATE_ENABLED,
    PORT_STATE_ERROR
} PortState;

typedef struct {
    union {
        volatile u32 U;
        struct {
            volatile u32 RouteString    : 20;
            volatile u32 Speed          : 4;
            volatile u32 RsvdZ0         : 1;
            volatile u32 MultiTT        : 1;
            volatile u32 Hub            : 1;
            volatile u32 ContextEntries : 5;
        };
    };
} __attribute__((packed)) xHCIRegSlotContextField0;

typedef struct {
    union {
        volatile u32 U;
        struct {
            volatile u32 MaxExitLatency    : 16;
            volatile u32 RootHubPortNumber : 8;
            volatile u32 NumberOfPorts     : 8;
        };
    };
} __attribute__((packed)) xHCIRegSlotContextField1;

typedef struct {
    union {
        volatile u32 U;
        struct {
            volatile u32 ParentHubSlotID   : 8;
            volatile u32 ParentPortNumber  : 8;
            volatile u32 TThinkTime        : 2;
            volatile u32 RsvdZ0            : 4;
            volatile u32 InterrupterTarget : 10;
        };
    };
} __attribute__((packed)) xHCIRegSlotContextField2;

typedef struct {
    union {
        volatile u32 U;
        struct {
            volatile u32 USBDeviceAddress : 8;
            volatile u32 RsvdZ0           : 22;
            volatile u32 SlotState        : 5;
        };
    };
} __attribute__((packed)) xHCIRegSlotContextField3;

typedef struct {
    union {
        volatile u32 U;
        struct {
            volatile u32 EndPointState                             : 3;
            volatile u32 RsvdZ0                                    : 5;
            volatile u32 Mult                                      : 2;
            volatile u32 MaxPrimaryStreams                         : 5;
            volatile u32 LinearStreamArray                         : 1;
            volatile u32 Interval                                  : 8;
            volatile u32 MaxEndPointServiceTimeIntervalPayloadHigh : 8;
        };
    };
} __attribute__((packed)) xHCIRegEndpointContextField0;

typedef struct {
    union {
        volatile u32 U;
        struct {
            volatile u32 RsvdZ0             : 1;
            volatile u32 ErrorCount         : 2;
            volatile u32 EndPointType       : 3;
            volatile u32 RsvdZ1             : 1;
            volatile u32 HostInititeDisable : 1;
            volatile u32 MaxBurstSize       : 8;
            volatile u32 MaxPacketSize      : 16;
        };
    };
} __attribute__((packed)) xHCIRegEndpointContextField1;

typedef struct {
    union {
        volatile u64 U;
        struct {
            volatile u64 DequeueCycleState : 1;
            volatile u64 RsvdZ0            : 3;
            volatile u64 EndPointType      : 60;
        };
    };
} __attribute__((packed)) xHCIRegEndpointContextField2;

typedef struct {
    union {
        volatile u32 U;
        struct {
            volatile u32 AverageTRBLength                         : 16;
            volatile u32 MaxEndpointServiceTimeIntervalPayloadLow : 16;
        };
    };
} __attribute__((packed)) xHCIRegEndpointContextField3;

typedef struct {
    union {
        volatile u64 U;
        struct {
            volatile u64 DequeueCycleState : 1;
            volatile u64 StreamContextType : 4;
            volatile u64 TRDequeuePointer  : 59;
        };
    };
} __attribute__((packed)) xHCIRegStreamContextField0;

typedef volatile struct {
    union {
        volatile u32 U;
        struct {
            volatile u32 StoppedEDTLA : 24;
            volatile u32 RsvdO0       : 8;
        };
    };
} __attribute__((packed)) xHCIRegStreamContextField1;

typedef volatile struct {
    union {
        volatile u64 U;
        struct {
            volatile u64 RsvdZ0 : 2;
            volatile u64 D      : 62;
        };
    };
} __attribute__((packed)) xHCIRegInputControlContextField0;

typedef volatile struct {
    union {
        volatile u64 A;
        volatile u64 U;
    };
} __attribute__((packed)) xHCIRegInputControlContextField1;

typedef volatile struct {
    union {
        volatile u32 U;
        struct {
            volatile u32 ConfigurationValue : 8;
            volatile u32 InterfaceNumber    : 8;
            volatile u32 AlternateSetting   : 8;
            volatile u32 RsvdZ0             : 8;
        };
    };
} __attribute__((packed)) xHCIRegInputControlContextField2;

typedef volatile struct {
    volatile xHCIRegSlotContextField0    SlotField0;
    volatile xHCIRegSlotContextField1    SlotField1;
    volatile xHCIRegSlotContextField2    SlotField2;
    volatile xHCIRegSlotContextField3    SlotField3;
    volatile u32                         RsvdO0[4];
} __attribute__((packed)) xHCISlotContext;

typedef volatile struct {
    xHCISlotContext SlotContext;
    struct {
        volatile xHCIRegEndpointContextField0 Field0;
        volatile xHCIRegEndpointContextField1 Field1;
        volatile xHCIRegEndpointContextField2 Field2;
        volatile xHCIRegEndpointContextField3 Field3;
        volatile u32                          RsvdO1[3];
    } EndpointContext[31];
} __attribute__((packed)) xHCIDeviceContext;

typedef volatile struct {
    volatile xHCIRegStreamContextField0 StreamContextField0;
    volatile xHCIRegStreamContextField1 StreamContextField1;
    volatile u32 RsvdO2[1];
} __attribute__((packed)) xHCIStreamContext;

typedef volatile struct {
    volatile xHCIRegInputControlContextField0 Field0;
    volatile xHCIRegInputControlContextField1 Field1;
    volatile u32                              RsvdO3[5];
    volatile xHCIRegInputControlContextField2 Field2;
} __attribute__((packed)) xHCIInputControlContext;

typedef volatile struct {
    volatile u8 RsvdZ0[1];
    volatile u8 Ports[255];
} __attribute__((packed)) xHCIPortBandwidthContext;

typedef struct {
    union {
        volatile u64 U;
        volatile u64 Base;
        struct {
            u64 Low  : 32;
            u64 High : 32;
        };
    };
} __attribute__((packed)) xHCIRegNormalTRBField0;

typedef struct {
    union {
        volatile u32 U;
        struct {
            volatile u32 TRBTransferLength : 17;
            volatile u32 TDSize            : 5;
            volatile u32 InterrupterTarget : 10;
        };
    };
} __attribute__((packed)) xHCIRegNormalTRBField1;

typedef struct {
    union {
        volatile u32 U;
        struct {
            volatile u32 CycleBit               : 1;
            volatile u32 EvaluateNextTRB        : 1;
            volatile u32 InterruptOnShortPacket : 1;
            volatile u32 NoSnoop                : 1;
            volatile u32 ChainBit               : 1;
            volatile u32 InterruptOnCompletion  : 1;
            volatile u32 ImmediateData          : 1;
            volatile u32 RsvdZ0                 : 2;
            volatile u32 BlockEventInterrupt    : 1;
            volatile u32 TRBType                : 6;
            volatile u32 RsvdZ1                 : 15;
        };
    };
} __attribute__((packed)) xHCIRegNormalTRBField2;

typedef volatile struct {
    volatile xHCIRegNormalTRBField0 NormalTRBField0;
    volatile xHCIRegNormalTRBField1 NormalTRBField1;
    volatile xHCIRegNormalTRBField2 NormalTRBField2;
} __attribute__((packed)) xHCITransferRequestBlock;

typedef struct {
    union {
        volatile u32 U;
        struct {
            volatile u32 RequestType : 8;
            volatile u32 Request     : 8;
            volatile u32 Value       : 16;
        };
    };
} __attribute__((packed)) xHCIRegSetupStageTRBField0;

typedef struct {
    union {
        volatile u32 U;
        struct {
            volatile u32 Index  : 16;
            volatile u32 Length : 16;
        };
    };
} __attribute__((packed)) xHCIRegSetupStageTRBField1;

typedef struct {
    union {
        volatile u32 U;
        struct {
            volatile u32 TRBTransferLength : 17;
            volatile u32 RsvdZ0            : 5;
            volatile u32 InterrupterTarget : 10;
        };
    };
} __attribute__((packed)) xHCIRegSetupStageTRBField2;

typedef struct {
    union {
        volatile u32 U;
        struct {
            volatile u32 CycleBit              : 1;
            volatile u32 RsvdZ0                : 5;
            volatile u32 InterruptOnCompletion : 1;
            volatile u32 ImmediateData         : 1;
            volatile u32 RsvdZ1                : 3;
            volatile u32 TRBType               : 6;
            volatile u32 TransferType          : 2;
            volatile u32 RsvdZ2                : 14;
        };
    };
} __attribute__((packed)) xHCIRegSetupStageTRBField3;

typedef volatile struct {
    volatile xHCIRegSetupStageTRBField0 SetupStageTRBField0;
    volatile xHCIRegSetupStageTRBField1 SetupStageTRBField1;
    volatile xHCIRegSetupStageTRBField2 SetupStageTRBField2;
    volatile xHCIRegSetupStageTRBField3 SetupStageTRBField3;
} __attribute__((packed)) xHCISetupStageTRB;

typedef struct {
    union {
        volatile u64 U;
        volatile u64 Base;
        struct {
            u64 Low  : 32;
            u64 High : 32;
        };
    };
} __attribute__((packed)) xHCIRegDataStageTRBField0;

typedef struct {
    union {
        volatile u32 U;
        struct {
            volatile u32 TRBTransferLength : 17;
            volatile u32 TDSize            : 5;
            volatile u32 InterrupterTarget : 10;
        };
    };
} __attribute__((packed)) xHCIRegDataStageTRBField1;

typedef struct {
    union {
        volatile u32 U;
        struct {
            volatile u32 CycleBit               : 1;
            volatile u32 EvaluateNextTRB        : 1;
            volatile u32 InterruptOnShortPacket : 1;
            volatile u32 NoSnoop                : 1;
            volatile u32 ChainBit               : 1;
            volatile u32 InterruptOnCompletion  : 1;
            volatile u32 ImmediateData          : 1;
            volatile u32 RsvdZ0                 : 2;
            volatile u32 BlockEventInterrupt    : 1;
            volatile u32 TRBType                : 6;
            volatile u32 RsvdZ1                 : 15;
        };
    };
} __attribute__((packed)) xHCIRegDataStageTRBField2;

typedef volatile struct {
    volatile xHCIRegDataStageTRBField0 xHCIRegDataStageTRBField0;
    volatile xHCIRegDataStageTRBField1 xHCIRegDataStageTRBField1;
    volatile xHCIRegDataStageTRBField2 xHCIRegDataStageTRBField2;
} __attribute__((packed)) xHCIDataStageTRB;

typedef volatile struct {
    union {
        volatile u64 U;
        volatile u64 RsvdZ0;
    };
} __attribute__((packed)) xHCIRegStatusStageTRBField0;

typedef struct {
    union {
        volatile u64 U;
        struct {
            volatile u64 RsvdZ0            : 22;
            volatile u64 InterrupterTarget : 10;
        };
    };
} __attribute__((packed)) xHCIRegStatusStageTRBField1;

typedef struct {
    union {
        volatile u32 U;
        struct {
            volatile u32 CycleBit               : 1;
            volatile u32 EvaluateNextTRB        : 1;
            volatile u32 RsvdZ0                 : 2;
            volatile u32 ChainBit               : 1;
            volatile u32 InterruptOnCompletion  : 1;
            volatile u32 RsvdZ1                 : 4;
            volatile u32 TRBType                : 6;
            volatile u32 Direction              : 1;
            volatile u32 RsvdZ2                 : 15;
        };
    };
} __attribute__((packed)) xHCIRegStatusStageTRBField2;

typedef volatile struct {
    volatile xHCIRegStatusStageTRBField0 xHCIRegStatusStageTRBField0;
    volatile xHCIRegStatusStageTRBField1 xHCIRegStatusStageTRBField1;
    volatile xHCIRegStatusStageTRBField2 xHCIRegStatusStageTRBField2;
} __attribute__((packed)) xHCIStatusStageTRB;

typedef volatile struct {
    volatile xHCISetupStageTRB  SetupStageTRB;
    volatile xHCIDataStageTRB   DataStageTRB;
    volatile xHCIStatusStageTRB StatusStageTRB;
} __attribute__((packed)) xHCIControlTRBs;

typedef struct {
    union {
        volatile u64 U;
        struct {
            u64 rwRsvdP0                        : 6;
            u64 rwEventRingSegmentBaseAddress   : 58;
        };
    };
    union {
        volatile u32 U;
        struct {
            u32 rwEventRingSegmentSize          : 16;
            u32 rwRsvdP1                        : 16;
        };
    } EventRingSegmentSize;
    u32 rwRsvdZ0;
} __attribute__((packed)) xHCIEventRingSegmentTableEntry;

typedef struct {
    volatile u8  CAPLENGTH;
    volatile u8  roRsvd;
    volatile u16 HCIVERSION;
    union {
        volatile u32 U;
        struct {
            u32 roNumberOfDeviceSlots  : 8;
            u32 roNumberOfInterrupters : 11;
            u32 roRsvd0                : 5;
            u32 roNumberOfPorts        : 8;
        };
    } HCSPARAMS1;
    union {
        volatile u32 U;
        struct {
            u32 roIsochronousSchedulingThreshold : 4;
            u32 roEventRingSegmentTableMax       : 4;
            u32 roRsvd0                          : 13;
            u32 roMaxScratchpadBuffers           : 5;
            u32 roScratchpadRestore              : 1;
            u32 roNumberOfPorts                  : 5;
        };
    } HCSPARAMS2;
    union {
        volatile u32 U;
        struct {
            u32 roU1DeviceExitLatency : 8;
            u32 roU2DeviceExitLatency : 8;
            u32 roRsvd0               : 16;
        };
    } HCSPARAMS3;
    union {
        volatile u32 U;
        struct {
            u32 roAddressingCapability64Bit           : 1;
            u32 roBWNegotiationCapability             : 1;
            u32 roContextSize                         : 1;
            u32 roPortPowerControl                    : 1;
            u32 roPortIndicators                      : 1;
            u32 roLightHCResetCapability              : 1;
            u32 roLatencyToleranceMessagingCapability : 1;
            u32 roNoSecondarySIDSupport               : 1;
            u32 roParseAllEventData                   : 1;
            u32 roStoppedShortPacketCapability        : 1;
            u32 roStoppedEDTLACapability              : 1;
            u32 roContiguousFrameIDCapability         : 1;
            u32 roMaximumPrimaryStreamArraySize       : 4;
            u32 roxHCIExtendedCapabilitiesPointer     : 16;
        };
    } HCCPARAMS1;
    union {
        volatile u32 U;
        struct {
            u32 roRsvd0               : 2;
            u32 roDoorbellArrayOffset : 30;
        };
    } DBOFF;
    union {
        volatile u32 U;
        struct {
            u32 roRsvd0                      : 5;
            u32 roRuntimeRegisterSpaceOffset : 27;
        };
    } RTSOFF;
    union {
        volatile u32 U;
        struct {
            u32 roU3EntryCapability                                        : 1;
            u32 roConfigureEndpointCommandMaxExitLatencyTooLargeCapability : 1;
            u32 roForceSaveContextCapability                               : 1;
            u32 roComplianceTransitionCapability                           : 1;
            u32 roLargeESITPayloadCapability                               : 1;
            u32 roConfigurationInformationCapability                       : 1;
            u32 roExtendedTBCCapability                                    : 1;
            u32 roExtendedTBCTRBStatusCapability                           : 1;
            u32 roGetSetExtendedPropertyCapability                         : 1;
            u32 roVirtualizationBasedTrustedIOCapability                   : 1;
            u32 roRsvd0                                                    : 22;
        };
    } HCCPARAMS2;
} __attribute__((packed)) xHCIRegsCapability;

typedef struct {
    union {
        volatile u32 U;
        struct {
            u32 rwRunStop                    : 1;
            u32 rwHostControllerReset        : 1;
            u32 rwInterrupterEnable          : 1;
            u32 rwHostSystemErrorEnable      : 1;
            u32 rwRsvdP                      : 3;
            u32 rwLightHostControllerReset   : 1;
            u32 rwControllerSaveState        : 1;
            u32 rwControllerRestoreState     : 1;
            u32 rwEnableWrapEvent            : 1;
            u32 rwEnableU3MFINDEXStop        : 1;
            u32 rwRsvdP0                     : 1;
            u32 rwCEMEnable                  : 1;
            u32 rwExtendedTBCEnable          : 1;
            u32 rwExtendedTBCTRBStatusEnable : 1;
            u32 rwVTIOEnable                 : 1;
            u32 rwRsvdP1                     : 15;
        };
    };
} __attribute__((packed)) xHCIRegsUSBCMD;

typedef struct {
    union {
        volatile u32 U;
        struct {
            u32 roHCHalted            : 1;
            u32 rwRsvdZ               : 1;
            u32 rw1cHostSystemError   : 1;
            u32 rw1cEventInterrupt    : 1;
            u32 rw1cPortChangeDetect  : 1;
            u32 rwRsvdZ0              : 3;
            u32 roSaveStateStatus     : 1;
            u32 roRestoreStateStatus  : 1;
            u32 rw1cSaveRestoreError  : 1;
            u32 roControllerNotReady  : 1;
            u32 roHostControllerError : 1;
            u32 rwRsvdZ1              : 19;
        };
    };
} __attribute__((packed)) xHCIRegsUSBSTS;

typedef struct {
    union {
        volatile u32 U;
        struct {
            u32 roPageSize : 16;
            u32 rwRsvdZ0   : 16;
        };
    };
} __attribute__((packed)) xHCIRegsPAGESIZE;

typedef struct {
    union {
        volatile u32 U;
        struct {
            u32 rwNotificationEnable : 16;
            u32 rwRsvdP0             : 16;
        };
    };
} __attribute__((packed)) xHCIRegsDNCTRL;

typedef struct {
    union {
        volatile u64 U;
        struct {
            u64 rwRingCycleState     : 1;
            u64 rw1cCommandStop      : 1;
            u64 rw1cCommandAbort     : 1;
            u64 roCommandRingRunning : 1;
            u64 rwRsvdP0             : 2;
            union {
                u64 rwCommandRingPointerBase : 58;
                struct {
                    u64 rwCommandRingPointerLow  : 26;
                    u64 rwCommandRingPointerHigh : 32;
                };
            };
        };
    };
} __attribute__((packed)) xHCIRegsCRCR;

typedef struct {
    union {
        volatile u64 U;
        struct {
            u64 rwRsvdP0 : 6;
            u64 rwDeviceContextBaseAddressArrayPointerBase : 58;
        };
    };
} __attribute__((packed)) xHCIRegsDCBAAP;

typedef struct {
    union {
        volatile u32 U;
        struct {
            u32 rwMaxDeviceSlotsEnabled          : 8;
            u32 rwU3EntryEnable                  : 1;
            u32 rwConfigurationInformationEnable : 1;
            u32 rwRsvdP                          : 22;
        };
    };
} __attribute__((packed)) xHCIRegsCONFIG;

typedef struct {
    union {
        volatile u32 U;
        struct {
            u32 rosCurrentConnectStatus        : 1;
            u32 rw1csPortEnabledDisabled       : 1;
            u32 rwRsvdZ0                       : 1;
            u32 roOvercurrentActive            : 1;
            u32 rw1sPortReset                  : 1;
            u32 rwsPortLinkState               : 4;
            u32 rwsPortPower                   : 1;
            u32 rosPortSpeed                   : 4;
            u32 rwsPortIndicatorControl        : 2;
            u32 rwPortLinkStateWriteStrobe     : 1;
            u32 rw1csConnectStatusChange       : 1;
            u32 rw1csPortEnabledDisabledChange : 1;
            u32 rw1csWarmPortResetChange       : 1;
            u32 rw1csOvercurrentChange         : 1;
            u32 rw1csPortResetChange           : 1;
            u32 rw1csPortLinkStateChange       : 1;
            u32 rw1csPortConfigErrorChange     : 1;
            u32 roColdAttachStatus             : 1;
            u32 rwsWakeOnConnectEnable         : 1;
            u32 rwsWakeOnDisconnectEnable      : 1;
            u32 rwsWakeOnOverCurrentEnable     : 1;
            u32 rwRsvdZ1                       : 2;
            u32 roDeviceRemovable              : 1;
            u32 rw1sWarmPortReset              : 1;
        };
    };
} __attribute__((packed)) xHCIRegsPORTSC;

typedef struct {
    union {
        volatile u32 U;
        struct {
            u32 rwsU1TimeOut        : 8;
            u32 rwsU2TimeOut        : 8;
            u32 rwForceLinePMAccept : 1;
            u32 rwRsvdP0            : 15;
        } USB3;
        struct {
            u32 roL1Status                 : 3;
            u32 rwRemoteWakeEnable         : 1;
            u32 rwBestEffortServiceLatency : 4;
            u32 rwL1DeviceSlot             : 8;
            u32 rwHardWareLPMEnable        : 1;
            u32 rwRsvdP0                   : 11;
            u32 rwPortTestControl          : 4;
        } USB2;
    };
} __attribute__((packed)) xHCIRegsPORTPMSC;

typedef struct {
    union {
        volatile u32 U;
        struct {
            u32 rwLinkErrorCount    : 16;
            u32 roRxLaneCount       : 4;
            u32 roTxLaneCount       : 4;
            u32 rwRsvdP0            : 8;
        } USB3;
        struct {
            u32 rwRsvdP0            : 32;
        } USB2;
    };
} __attribute__((packed)) xHCIRegsPORTLI;

typedef struct {
    union {
        volatile u32 U;
        struct {
            u32 rwLinkSoftErrorCount : 16;
            u32 rwRsvdP0             : 16;
        } USB3;
        struct {
            u32 rwsHostInitiatedResumeDurationMode : 2;
            u32 rwsL1TimeOut                       : 8;
            u32 rwsBestEffortServiceLatencyDeep    : 4;
            u32 rwsRsvdP0                          : 18;
        } USB2;
    };
} __attribute__((packed)) xHCIRegsPORTEXSC;

typedef struct {
    union {
        volatile u32 U;
        struct {
            u32 rwRsvdP0             : 32;
        } USB3;
        struct {
            u32 rwsHostInitiatedResumeDurationMode : 2;
            u32 rwsL1TimeOut                       : 8;
            u32 rwsBestEffortServiceLatencyDeep    : 4;
            u32 rwsRsvdP0                          : 18;
        } USB2;
    };
} __attribute__((packed)) xHCIRegsPORTHLPMC;

typedef volatile struct {
    volatile xHCIRegsPORTSC    PORTSC;
    volatile xHCIRegsPORTPMSC  PORTPMSC;
    volatile xHCIRegsPORTLI    PORTLI;
    volatile xHCIRegsPORTHLPMC PORTHLPMC;
} __attribute__((packed)) xHCIRegsPORTS;

typedef volatile struct {
    volatile xHCIRegsUSBCMD   USBCMD;
    volatile xHCIRegsUSBSTS   USBSTS;
    volatile xHCIRegsPAGESIZE PAGESIZE;
    volatile u32              rwRsvdZ0[2];
    volatile xHCIRegsDNCTRL   DNCTRL;
    volatile xHCIRegsCRCR     CRCR;
    volatile u32              rwRsvdZ1[2];
    volatile xHCIRegsDCBAAP   DCBAAP;
    volatile xHCIRegsCONFIG   CONFIG;
    volatile u8               rwRsvdZ2[0x3C4];
    volatile xHCIRegsPORTS    PORTS[];
} __attribute__((packed)) xHCIRegsOperational;

typedef struct {
    union {
        volatile u32 U;
        struct {
            u32 roMicroframeIndex : 14;
            u32 rwRsvdP0          : 18;
        };
    };
} __attribute__((packed)) xHCIRegsMFINDEX;

typedef struct {
    union {
        volatile u32 U;
        struct {
            u32 rw1cInterruptPending : 1;
            u32 rwInterruptEnable    : 1;
            u32 rwRsvdP0             : 30;
        };
    };
} __attribute__((packed)) xHCIRegsIMAN;

typedef struct {
    union {
        volatile u32 U;
        struct {
            u32 rwInterruptModerationInterval : 16;
            u32 rwInterruptModerationCounter  : 16;
        };
    };
} __attribute__((packed)) xHCIRegsIMOD;


typedef struct {
    union {
        volatile u32 U;
        struct {
            u32 rwEventRingSegmentTableSize : 16;
            u32 rwRsvdP0                    : 16;
        };
    };
} __attribute__((packed)) xHCIRegsERSTSZ;

typedef volatile struct {
    volatile union {
        volatile u64 U;
        volatile u64 rwEventRingSegmentTableBaseAddressRegisteBase;
    };
} __attribute__((packed)) xHCIRegsERSTBA;

typedef struct {
    union {
        volatile u64 U;
        struct {
            u64 rwDequeueERSTSegmentIndex : 3;
            u64 rw1cEventHandlerBusy      : 1;
            u64 rwEventRingDequeuePointer : 60;
        };
    };
} __attribute__((packed)) xHCIRegsERDP;

typedef volatile struct {
    volatile xHCIRegsIMAN   IMAN;
    volatile xHCIRegsIMOD   IMOD;
    volatile xHCIRegsERSTSZ ERSTSZ;
    volatile u32            rwRsvdP;
    volatile xHCIRegsERSTBA ERSTBA;
    volatile xHCIRegsERDP   ERDP;
} __attribute__((packed)) xHCIInterrupterRegs;

typedef volatile struct {
    volatile xHCIRegsMFINDEX     MFINDEX;
    volatile u8                  RsvdZ[28];
    volatile xHCIInterrupterRegs Interrupter[];
} __attribute__((packed)) xHCIRegsRuntime;

typedef volatile struct {
    union {
        volatile u32 U;
        struct {
            u32 rwDBTarget   : 8;
            u32 rwRsvdZ      : 8;
            u32 rwDBStreamID : 16;
        };
    };
} __attribute__((packed)) xHCIRegsDoorbell;

typedef struct {
    u64 MMIOBase;
    volatile xHCIRegsCapability*  CapabilityRegs;
    volatile xHCIRegsOperational* OperationalRegs;
    volatile xHCIRegsRuntime*     RuntimeRegs;
    volatile xHCIRegsDoorbell*    DoorbellRegs;

    u32 MaxSlots;
    u32 MaxPorts;
    u32 MaxInterrupters;
    
    __attribute__((aligned(64))) xHCITransferRequestBlock       EventRingTRBs[256];
    __attribute__((aligned(64))) xHCIEventRingSegmentTableEntry ERST[1];
    __attribute__((aligned(64))) xHCITransferRequestBlock       CommandRingTRBs[64];
    
    __attribute__((aligned(64))) xHCIDeviceContext DeviceContexts[256]; 
    __attribute__((aligned(64))) u64               xHCIDCBAAList[256];

    u32 EventRingCycleState;
    u32 CommandRingCycleState;
    
    u8 IsRunning;
    u8 IsMSIEnabled;
    
    u32 UsedSlots;
    u32 LastSlotID;

    u32 PendingCommandType;
    u8  CommandPending;
} xHCIController;

typedef struct {
    volatile u64 MessageAddress;
    volatile u32 MessageData;
    volatile u32 VectorControl;
} __attribute__((packed)) xHCIMSIXTableEntry;

typedef struct {
    volatile u64 PendingBits;
} __attribute__((packed)) xHCIMSIXPBA;

u64 GetXHCIMMIO();
xHCIController* InitXHCI();
__attribute__((naked)) void XHCIInt();