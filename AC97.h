#pragma once

#include "int.h"

#define AC97_PO_BDBAR    0x10
#define AC97_PO_CIV      0x14
#define AC97_PO_LVI      0x15
#define AC97_PO_SR       0x16
#define AC97_PO_PICB     0x18
#define AC97_PO_CR_RUN   0x01
#define AC97_PO_CR_IOCE  0x04
#define AC97_PO_CR_LVBIE 0x10

#define AC97_PO_CR_RUN   0x01
#define AC97_PO_CR_NAD   0x02
#define AC97_PO_CR_IOCE  0x04
#define AC97_PO_CR_FEIE  0x08
#define AC97_PO_CR_LVBIE 0x10

#define AC97_PO_SR_DCH   0x01
#define AC97_PO_SR_BCIS  0x02
#define AC97_PO_SR_LVBIS 0x04
#define AC97_PO_SR_FIFOE 0x08
#define AC97_PO_SR_DESCE 0x10

#define AC97_BDL_IOC  0x01  

#define AC97_PO_BDBAR   0x10
#define AC97_PO_CIV     0x14
#define AC97_PO_LVI     0x15
#define AC97_PO_SR      0x16
#define AC97_PO_PICB    0x18
#define AC97_PO_CR      0x1B

#define AC97_PO_CR_RUN  0x01
#define AC97_PO_CR_RR   0x02

#define AC97_REG_MASTER_VOL     0x02
#define AC97_REG_HEADPHONE_VOL  0x04
#define AC97_REG_PCM_OUT_VOL    0x18

#define AC97_MUTE_BIT           (1 << 15)

typedef struct {
    union{
        volatile u16 U;
        struct{
            volatile u16 rwID    : 10;
            volatile u16 rwSE    : 5;
            volatile u16 rwRsvd0 : 1;
        };
    };
} __attribute__((packed)) AC97RegReset;

typedef struct {
    union{
        volatile u16 U;
        struct{
            volatile u16 rwMR    : 6;
            volatile u16 rwRsvd0 : 2;
            volatile u16 rwML    : 6;
            volatile u16 rwRsvd1 : 1;
            volatile u16 rwMute  : 1;
        };
    };
} __attribute__((packed)) AC97RegMasterVolume;

typedef struct {
    union{
        volatile u16 U;
        struct{
            volatile u16 rwMR    : 6;
            volatile u16 rwRsvd0 : 2;
            volatile u16 rwML    : 6;
            volatile u16 rwRsvd1 : 1;
            volatile u16 rwMute  : 1;
        };
    };
} __attribute__((packed)) AC97RegHeadphoneVolume;

typedef struct {
    union{
        volatile u16 U;
        struct{
            volatile u16 rwMM    : 6;
            volatile u16 rwRsvd0 : 9;
            volatile u16 rwMute  : 1;
        };
    };
} __attribute__((packed)) AC97RegMasterVolumeMono;

typedef struct {
    union{
        volatile u16 U;
        struct{
            volatile u16 rwTR    : 4;
            volatile u16 rwRsvd0 : 4;
            volatile u16 rwBA    : 4;
            volatile u16 rwRsvd1 : 4;
        };
    };
} __attribute__((packed)) AC97RegMasterToneRAndL;

typedef struct {
    union{
        volatile u16 U;
        struct{
            volatile u16 rwRsvd0 : 1;
            volatile u16 rwPV    : 4;
            volatile u16 rwRsvd1 : 10;
            volatile u16 rwMute  : 1;
        };
    };
} __attribute__((packed)) AC97RegPCBeepVolume;

typedef struct {
    union{
        volatile u16 U;
        struct{
            volatile u16 rwGN    : 5;
            volatile u16 rwRsvd0 : 10;
            volatile u16 rwMute  : 1;
        };
    };
} __attribute__((packed)) AC97RegPhoneVolume;

typedef struct {
    union{
        volatile u16 U;
        struct{
            volatile u16 rwGN       : 5;
            volatile u16 rwRsvd0    : 1;
            volatile u16 rw20dB    : 1;
            volatile u16 rwRsvd1    : 8;
            volatile u16 rwMute     : 1;
        };
    };
} __attribute__((packed)) AC97RegMicVolume;

typedef struct {
    union{
        volatile u16 U;
        struct{
            volatile u16 rwGR    : 5;
            volatile u16 rwRsvd0 : 3;
            volatile u16 rwGL    : 5;
            volatile u16 rwRsvd1 : 2;
            volatile u16 rwMute  : 1;
        };
    };
} __attribute__((packed)) AC97RegLineInVolume;

typedef struct {
    union{
        volatile u16 U;
        struct{
            volatile u16 rwGR    : 5;
            volatile u16 rwRsvd0 : 3;
            volatile u16 rwGL    : 5;
            volatile u16 rwRsvd1 : 2;
            volatile u16 rwMute  : 1;
        };
    };
} __attribute__((packed)) AC97RegCDVolume;

typedef struct {
    union{
        volatile u16 U;
        struct{
            volatile u16 rwGR    : 5;
            volatile u16 rwRsvd0 : 3;
            volatile u16 rwGL    : 5;
            volatile u16 rwRsvd1 : 2;
            volatile u16 rwMute  : 1;
        };
    };
} __attribute__((packed)) AC97RegVideoVolume;

typedef struct {
    union{
        volatile u16 U;
        struct{
            volatile u16 rwGR    : 5;
            volatile u16 rwRsvd0 : 3;
            volatile u16 rwGL    : 5;
            volatile u16 rwRsvd1 : 2;
            volatile u16 rwMute  : 1;
        };
    };
} __attribute__((packed)) AC97RegAuxVolume;

typedef struct {
    union{
        volatile u16 U;
        struct{
            volatile u16 rwGR    : 5;
            volatile u16 rwRsvd0 : 3;
            volatile u16 rwGL    : 5;
            volatile u16 rwRsvd1 : 2;
            volatile u16 rwMute  : 1;
        };
    };
} __attribute__((packed)) AC97RegPCMOutVol;

typedef struct {
    union{
        volatile u16 U;
        struct{
            volatile u16 rwGR    : 3;
            volatile u16 rwRsvd0 : 5;
            volatile u16 rwGL    : 3;
            volatile u16 rwRsvd1 : 5;
        };
    };
} __attribute__((packed)) AC97RegRecordSelect;

typedef struct {
    union{
        volatile u16 U;
        struct{
            volatile u16 rwGR    : 4;
            volatile u16 rwRsvd0 : 4;
            volatile u16 rwGL    : 4;
            volatile u16 rwRsvd1 : 3;
            volatile u16 rwMute  : 1;
        };
    };
} __attribute__((packed)) AC97RegRecordGain;

typedef struct {
    union{
        volatile u16 U;
        struct{
            volatile u16 rwGM    : 4;
            volatile u16 rwRsvd0 : 11;
            volatile u16 rwMute  : 1;
        };
    };
} __attribute__((packed)) AC97RegRecordGainMic;

typedef struct {
    union{
        volatile u16 U;
        struct{
            volatile u16 rwRsvd0 : 7;
            volatile u16 rwLPBK  : 1;
            volatile u16 rwMS    : 1;
            volatile u16 rwMIX   : 1;
            volatile u16 rwRsvd1 : 2;
            volatile u16 rwLD    : 1;
            volatile u16 rw3D    : 1;
            volatile u16 rwST    : 1;
            volatile u16 rwPOP   : 1;
        };
    };
} __attribute__((packed)) AC97RegGeneralPurpose;

typedef struct {
    union{
        volatile u16 U;
        struct{
            volatile u16 rwDP    : 4;
            volatile u16 rwRsvd0 : 4;
            volatile u16 rwCR    : 4;
            volatile u16 rwRsvd1 : 4;
        };
    };
} __attribute__((packed)) AC97Reg3DControl;

typedef struct {
    union{
        volatile u16 U;
        struct{
            volatile u16 rwRsvd0 : 16;
        };
    };
} __attribute__((packed)) AC97RegReserved;

typedef struct {
    union{
        volatile u16 U;
        struct{
            union {
                volatile u16 roLow : 8;
                struct{
                    volatile u16 roADCSectionReady      : 1;
                    volatile u16 roDACSectionReady      : 1;
                    volatile u16 roAnalogMixersEtcReady : 1;
                    volatile u16 roVrefsUpNominalLevel  : 1;
                    volatile u16 roReserved0 : 4;
                };
                struct{
                    volatile u16 roADC    : 1;
                    volatile u16 roDAC    : 1;
                    volatile u16 roANL    : 1;
                    volatile u16 roREF    : 1;
                    volatile u16 roRsvd0  : 4;
                };
            };
            union {
                volatile u16 rwHigh : 8;
                struct {
                    union {
                        volatile u16 rwPR : 7;
                        struct {
                            volatile u16 rwPCMInADCAndInputMuxPowerDown : 1;
                            volatile u16 rwPCMOutDACsPowerDown          : 1;
                            volatile u16 rwAnalogMixerpowerDown         : 1;
                            volatile u16 rwAnalogMixerPowerDown         : 1;
                            volatile u16 rwDigitalInterfacePowerDown    : 1;
                            volatile u16 rwInternalClkDisable           : 1;
                            volatile u16 rwHPAmpPowerDown               : 1;
                        };
                        struct {
                            volatile u16 rwPR0 : 1;
                            volatile u16 rwPR1 : 1;
                            volatile u16 rwPR2 : 1;
                            volatile u16 rwPR3 : 1;
                            volatile u16 rwPR4 : 1;
                            volatile u16 rwPR5 : 1;
                            volatile u16 rwPR6 : 1;
                        };
                    };
                    union {
                        volatile u16 rwEAPD   : 1;
                        volatile u16 rwExternalAmplifierPowerDown : 1;
                    };
                };
            };
        };
    };
} __attribute__((packed)) AC97RegPowerdownCtrlOrStat;

typedef struct {
    union {
        volatile u16 U;
        struct {
            volatile u16 rwVRA   : 1;
            volatile u16 rwDRA   : 1;
            volatile u16 rwRsvd0 : 1;
            volatile u16 rwVRM   : 1;
            volatile u16 rwRsvd1 : 2;
            volatile u16 rwCDAC  : 1;
            volatile u16 rwSDAC  : 1;
            volatile u16 rwLDAC  : 1;
            volatile u16 rwAMAP  : 1;
            volatile u16 rwRsvd2 : 4;
            volatile u16 rwID    : 2;
        };
        struct {
            volatile u16 rwVariableRatePCMAudioSupported     : 1;
            volatile u16 rwDoubleRatePCMAudioOutputSupported : 1;
            volatile u16 rwReserved0                         : 1;
            volatile u16 rwVariableRateMICInputSupported     : 1;
            volatile u16 rwReserved1                         : 2;
            volatile u16 rwPCMCenterDACSupported             : 1;
            volatile u16 rwPCMSurroundDACSupported           : 1;
            volatile u16 rwPCMLfeDACSupported                : 1;
            volatile u16 rwSlotOrDACMappingsBasedOnCodecID   : 1;
            volatile u16 rwReserved2                         : 4;
            volatile u16 rwCodecConfiguration                : 2;
        };
    };
} __attribute__((packed)) AC97RegExtendedAudioID;

typedef struct {
    union {
        volatile u16 U;
        struct {
            volatile u16 rwVRA   : 1;
            volatile u16 rwDRA   : 1;
            volatile u16 rwRsvd0 : 1;
            volatile u16 rwVRM   : 1;
            volatile u16 rwRsvd1 : 2;
            volatile u16 rwCDAC  : 1;
            volatile u16 rwSDAC  : 1;
            volatile u16 rwLDAC  : 1;
            volatile u16 rwMADC  : 1;
            volatile u16 rwPRI   : 1;
            volatile u16 rwPRJ   : 1;
            volatile u16 rwPRK   : 1;
            volatile u16 rwPRL   : 1;
            volatile u16 rwRsvd2 : 1;
        };
        struct {
            volatile u16 rwVariableRateAudioMode             : 1;
            volatile u16 rwDoubleRateAudioMode               : 1;
            volatile u16 rwReserved0                         : 1;
            volatile u16 rwVariableRateAudioModeForMICAndADC : 1;
            volatile u16 rwReserved1                         : 2;
            volatile u16 rwPCMCenterDACReady                 : 1;
            volatile u16 rwPCMSurroundDACsReady              : 1;
            volatile u16 rwPCMLfeDACReady                    : 1;
            volatile u16 rwMICAndADCReady                    : 1;
            volatile u16 rwPCMCenterDACOff                   : 1;
            volatile u16 rwPCMSurroundDACsOff                : 1;
            volatile u16 rwPCMLfeDACsOff                     : 1;
            volatile u16 rwMICAndADCOff                      : 1;
            volatile u16 rwReserved2                         : 1;
        };
    };
} __attribute__((packed)) AC97RegExtendedAudioStatOrCtrl;

typedef struct {
    union {
        volatile u16 U;
        volatile u16 rwSR;
        volatile u16 rwSampleRate;
    };
} __attribute__((packed)) AC97RegRate;

typedef struct {
    union {
        volatile u16 U;
        struct{
            union {
                volatile u16 rwCNT   : 6;
                volatile u16 rwCount : 6;
            };
            union {
                volatile u16 rwRsvd0     : 1;
                volatile u16 rwReserved0 : 1;
            };
            volatile u16 rwMute0 : 1;
            union {
                volatile u16 rwLFE               : 6;
                volatile u16 LowFrequencyEffects : 6;
            };
            union {
                volatile u16 rwRsvd1     : 1;
                volatile u16 rwReserved1 : 1;
            };
            volatile u16 rwMute1 : 1;
        };
    };
} __attribute__((packed)) AC97RegCAndLFEVol;

typedef struct {
    union {
        volatile u16 U;
        struct{
            union {
                volatile u16 rwRSR                        : 6;
                volatile u16 rwRightSurroundVolumeControl : 6;
            };
            union {
                volatile u16 rwRsvd0     : 1;
                volatile u16 rwReserved0 : 1;
            };
            volatile u16 rwMute0 : 1;
            union {
                volatile u16 rwLSR                       : 6;
                volatile u16 rwLeftSurroundVolumeControl : 6;
            };
            union {
                volatile u16 rwRsvd1     : 1;
                volatile u16 rwReserved1 : 1;
            };
            volatile u16 rwMute1 : 1;
        };
    };
} __attribute__((packed)) AC97RegLAndRAndSurrVol;

typedef struct {
    union {
        volatile u16 U;
        struct{
            union {
                volatile u16 rwLIN : 2;
                struct {
                    union {
                        volatile u16 rwLIN1           : 1;
                        volatile u16 rwLine1Supported : 1;
                    };
                    union {
                        volatile u16 rwLIN2           : 1;
                        volatile u16 rwLine2Supported : 1;
                    };
                };
            };
            union {
                volatile u16 rwHSET                     : 1;
                volatile u16 rwHandsetDACOrADCSupported : 1;
            };
            union {
                volatile u16 rwCID   : 2;
                struct {
                    union {
                        volatile u16 rwCID1                         : 1;
                        volatile u16 rwCallerIDDecodeLine1Supported : 1;
                    };
                    union {
                        volatile u16 rwCID2                         : 1;
                        volatile u16 rwCallerIDDecodeLine2Supported : 1;
                    };
                };
            };
            union {
                volatile u16 rwRsvd0     : 9;
                volatile u16 rwReserved0 : 9;
            };
            volatile u16 rwID    : 2;
        };
    };
} __attribute__((packed)) AC97RegExtendedModemID;

typedef struct {
    union {
        volatile u16 U;
        struct{
            volatile u16 roGPIO : 1;
            volatile u16 roMREF : 1;
            volatile u16 roADC1 : 1;
            volatile u16 roDAC1 : 1;
            volatile u16 roADC2 : 1;
            volatile u16 roDAC2 : 1;
            volatile u16 roHADC : 1;
            volatile u16 roHDAC : 1;
            union {
                volatile u16 rwPR : 8;
                struct {
                    volatile u16 rwPRA : 1;
                    volatile u16 rwPRB : 1;
                    volatile u16 rwPRC : 1;
                    volatile u16 rwPRD : 1;
                    volatile u16 rwPRE : 1;
                    volatile u16 rwPRF : 1;
                    volatile u16 rwPRG : 1;
                    volatile u16 rwPRH : 1;
                };
            };
        };
        struct{
            volatile u16 roGPIOReady                : 1;
            volatile u16 roModemVrefsUpNominalLevel : 1;
            volatile u16 roModemLine1ADCReady       : 1;
            volatile u16 roModemLine1DACReady       : 1;
            volatile u16 roModemLine2ADCReady       : 1;
            volatile u16 roModemLine2DACReady       : 1;
            volatile u16 roHandsetADCReady          : 1;
            volatile u16 roHandsetDACReady          : 1;
            volatile u16 rwGPIOPowerDown            : 1;
            volatile u16 rwModemVrefOff             : 1;
            volatile u16 rwModemLine1ADCOff         : 1;
            volatile u16 rwModemLine1DACOff         : 1;
            volatile u16 rwModemLine2ADCOff         : 1;
            volatile u16 rwModemLine2DACOff         : 1;
            volatile u16 rwHandsetADCOff            : 1;
            volatile u16 rwHandsetDACOff            : 1;
        };
    };
} __attribute__((packed)) AC97RegExtendedModemStatOrCtrl;

typedef struct {
    union {
        volatile u16 U;
        struct{
            volatile u16 rwADC   : 4;
            union {
                volatile u16 rwRsvd0     : 3;
                volatile u16 rwReserved0 : 3;
            };
            volatile u16 rwMute0 : 1;
            volatile u16 rwDAC   : 4;
            union {
                volatile u16 rwRsvd1     : 3;
                volatile u16 rwReserved1 : 3;
            };
            volatile u16 rwMute1 : 1;
        };
    };
} __attribute__((packed)) AC97RegDACOrADCLevel;

typedef struct {
    union {
        volatile u16 U;
        volatile u16 rwGC;
        volatile u16 rwGPIOConfig;
    };
} __attribute__((packed)) AC97RegGPIOPinConfig;

typedef struct {
    union {
        volatile u16 U;
        volatile u16 rwGP;
        volatile u16 rwGPIOPinPolarity;
    };
} __attribute__((packed)) AC97RegGPIOPinPolarityOrType;

typedef struct {
    union {
        volatile u16 U;
        volatile u16 rwGS;
        volatile u16 rwGPIOPinSticky;
    };
} __attribute__((packed)) AC97RegGPIOPinSticky;

typedef struct {
    union {
        volatile u16 U;
        volatile u16 rwGW;
        volatile u16 rwGPIOPinWakeUp;
    };
} __attribute__((packed)) AC97RegGPIOPinWakeUp;

typedef struct {
    union {
        volatile u16 U;
        volatile u16 rwGI;
        volatile u16 rwGPIOPinStatus;
    };
} __attribute__((packed)) AC97RegGPIOPinStatus; 

typedef struct {
    union {
        volatile u16 U;
        struct {
            volatile u16 rwL1B   : 3;
            volatile u16 rwRsvd0 : 1;
            volatile u16 rwL2B   : 3;
            volatile u16 rwRsvd1 : 1;
            volatile u16 rwHSB   : 3;
            volatile u16 rwRsvd2 : 1;
            volatile u16 rwMLNK  : 1;
            volatile u16 rwCIDR  : 1;
            volatile u16 rwCID   : 2;
        };
    };
} __attribute__((packed)) AC97RegMiscMdmAFEStatOrCtrl; 

typedef struct {
    union{
        volatile u16 U;
        struct{
            volatile u16 roS : 8;
            volatile u16 roF : 8;
        };
    };
} __attribute__((packed)) AC97RegVendorID1;

typedef struct {
    union{
        volatile u16 U;
        struct{
            volatile u16 roREV : 8;
            volatile u16 roT   : 8;
        };
    };
} __attribute__((packed)) AC97RegVendorID2;

typedef volatile struct {
    volatile AC97RegReset Reset;

    volatile AC97RegMasterVolume MasterVolume;
    volatile AC97RegHeadphoneVolume  HeadphoneVolume;
    volatile AC97RegMasterVolumeMono MasterVolumeMono;
    volatile AC97RegMasterToneRAndL MasterToneRAndL;

    volatile AC97RegPCBeepVolume PCBeepVolume;
    volatile AC97RegPhoneVolume PhoneVolume;
    volatile AC97RegMicVolume MicVolume;
    volatile AC97RegLineInVolume LineInVolume;
    volatile AC97RegCDVolume CDVolume;
    volatile AC97RegVideoVolume VideoVolume;
    volatile AC97RegAuxVolume AuxVolume;
    volatile AC97RegPCMOutVol PCMOutVol;

    volatile AC97RegRecordSelect RecordSelect;
    volatile AC97RegRecordGain RecordGain;
    volatile AC97RegRecordGainMic RecordGainMic;

    volatile AC97RegGeneralPurpose GeneralPurpose;
    volatile AC97Reg3DControl Control3D;

    volatile AC97RegReserved Reserved0;

    volatile AC97RegPowerdownCtrlOrStat PowerdownCtrlOrStat;
    volatile AC97RegExtendedAudioID ExtendedAudioID;
    volatile AC97RegExtendedAudioStatOrCtrl ExtendedAudioStatOrCtrl;
    volatile AC97RegRate PCMFrontDACRate;
    volatile AC97RegRate PCMSurrDACRate;
    volatile AC97RegRate PCMLfeDACRate;
    volatile AC97RegRate PCMLrADCRate;
    volatile AC97RegRate MICAndADCRate;
    volatile AC97RegCAndLFEVol CAndLFEVol;
    volatile AC97RegLAndRAndSurrVol LAndRAndSurrVol;

    volatile AC97RegExtendedModemID ExtendedModemID;
    volatile AC97RegExtendedModemStatOrCtrl ExtendedModemStatOrCtrl;

    volatile AC97RegRate Line1DACOrADCRate;
    volatile AC97RegRate Line2DACOrADCRate;
    volatile AC97RegRate HandsetDACOrADCRate;

    volatile AC97RegDACOrADCLevel DACOrADCLevel;

    volatile AC97RegGPIOPinConfig GPIOPinConfig;
    volatile AC97RegGPIOPinPolarityOrType GPIOPinPolarityOrType;
    volatile AC97RegGPIOPinSticky GPIOPinSticky;

    volatile AC97RegMiscMdmAFEStatOrCtrl MiscMdmAFEStatOrCtrl;

    volatile AC97RegVendorID1 VendorID1;
    volatile AC97RegVendorID2 VendorID2;
} __attribute__((packed)) AC97Regs;

typedef struct {
    volatile AC97Regs* Regs;
    u16 CodecPort;
    u16 DmaPort;
    u8 CodecIndex;
} AC97Controller;

typedef struct {
    volatile u32 BufferPointer;
    volatile u16 BufferLength;
    volatile u16 Flags;
} __attribute__((packed)) AC97BDLEntry;

extern s16* NowPCM;
extern Size64 NowPCMSize;

u64 AC97MMIOBase(u32* Bar1, u16* DmaPort);
AC97Controller* InitAC97(void);
u16 AC97ReadIO(volatile void* Ptr, u16 Offset);
void AC97WriteIO(volatile void* Ptr, u16 Val, u16 Offset);
void AC97StartPlayback(AC97Controller* AC97, u16 Rate);
void AC97StopPlayback(AC97Controller* AC97);
void AC97ResumePlayback(AC97Controller* AC97);
void AC97PausePlayback(AC97Controller* AC97);
Bool AC97PollStream(AC97Controller* AC97);
void AC97SetMasterVolume(AC97Controller* AC97, u8 volume);
void AC97SetPCMVolume(AC97Controller* AC97, u8 volume);
void AC97SetMute(AC97Controller* AC97, Bool enable_mute);
Bool AC97DMAFinish(AC97Controller* AC97);
void AC97TriggerNext(AC97Controller* AC97);