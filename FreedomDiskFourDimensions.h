#pragma once
#include "int.h"

#define FDFDHeaderStartSign 0xFDFD
#define FDFDHeaderEndSign 0xDFDF

#define FDFDFileStartSign 0x5500
#define FDFDFileEndSign 0xAA00

#define FDFDFolderStartSign 0x5501
#define FDFDFolderEndSign 0xAA01

#define FDFDTableSize 36

typedef struct {
    union {
        u16 Sign0x5500;
        struct {
            u8 SignLow;
            u8 SignHigh;
        };
    } SignStart;
    Size64 SizeOfName;
    Size64 SizeOfData;
    u64 OffSetOfSector;
    u64 SectorCount;
    union {
        u16 Sign0xAA00;
        struct {
            u8 SignLow;
            u8 SignHigh;
        };
    } SignEnd;
} __attribute__((packed)) FDFDFileTable;

typedef struct {
    union {
        u16 Sign0x5501;
        struct {
            u8 SignLow;
            u8 SignHigh;
        };
    } SignStart;
    Size64 SizeOfName;
    Size64 SizeOfData;
    u64 OffSetOfSector;
    u64 SectorCount;
    union {
        u16 Sign0xAA01;
        struct {
            u8 SignLow;
            u8 SignHigh;
        };
    } SignEnd;
} __attribute__((packed)) FDFDFolderTable;

typedef struct {
    union {
        u16 Sign0x55XX;
        struct {
            u8 SignLow;
            u8 SignHigh;
        };
    } SignStart;
    Size64 SizeOfName;
    Size64 SizeOfData;
    u64 OffSetOfSector;
    u64 SectorCount;
    union {
        u16 Sign0xAAXX;
        struct {
            u8 SignLow;
            u8 SignHigh;
        };
    } SignEnd;
} __attribute__((packed)) FDFDAnyTable;

typedef struct {
    u16 Sign0xFDFD;
    Size64 CountOfSector;
    Size64 SizeOfData;
    union {
        u8 Data[];
        FDFDFileTable Files[];
        FDFDFolderTable Folders[];
        FDFDAnyTable Any[];
        u8 DataSector[][512];
    };
} __attribute__((packed)) FDFDHeader;

char* GetFDFDFolderTableName(FDFDFolderTable* Data, u8 Idx);
char* GetFDFDFileTableName(FDFDFileTable* Data, u8 Idx);
u8* GetFDFDFolderTableBuffer(FDFDFolderTable* Data, u8 Idx);
u8* GetFDFDFileTableBuffer(FDFDFileTable* Data, u8 Idx);
void NewFile(FDFDFileTable Table, u8 Idx, u8* Data);
void NewFolder(FDFDFolderTable Table, u8 Idx, u8* Data);
u8* ReadFile(u8 Idx, char* Name);
void InitDiskToFDFD(u8 Idx);

int DelFile(u8 Idx, const char* Name);
int DelFolder(u8 Idx, const char* Name);