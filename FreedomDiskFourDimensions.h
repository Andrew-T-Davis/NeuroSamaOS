#pragma once
#include "int.h"

#define FDFDHeaderStartSign 0xFDFD
#define FDFDHeaderEndSign 0xDFDF

#define FDFDFileStartSign 0x5500
#define FDFDFileEndSign 0xAA00

#define FDFDFolderStartSign 0x5501
#define FDFDFolderEndSign 0xAA01

typedef struct {
    u16 Sign0xFDFD;
    Size64 CountOfSector;
    Size64 SizeOfData;
    union {
        u8 Data[];
        u8 DataSector[][512];
    };
} __attribute__((packed)) FDFDHeader;

typedef struct {
    u16 Sign0x5500;
    Size64 SizeOfName;
    Size64 SizeOfData;
    u64 OffSetOfSector;
    u64 SectorCount;
    u64 Idx;
    u16 Sign0xAA00;
} __attribute__((packed)) FDFDFileTable;

typedef struct {
    u16 Sign0x5501;
    Size64 SizeOfName;
    Size64 SizeOfData;
    u64 OffSetOfSector;
    u64 SectorCount;
    u64 Idx;
    u16 Sign0xAA01;
} __attribute__((packed)) FDFDFolderTable;

char* GetFDFDFolderTableName(FDFDFolderTable* Data, u8 Idx);
char* GetFDFDFileTableName(FDFDFileTable* Data, u8 Idx);
u8* GetFDFDFolderTableBuffer(FDFDFolderTable* Data, u8 Idx);
u8* GetFDFDFileTableBuffer(FDFDFileTable* Data, u8 Idx);
void NewFile(FDFDFileTable Table, u8 Idx, u8* Data);
void NewFolder(FDFDFolderTable Table, u8 Idx, u8* Data);
void InitDiskToFDFD(u8 Idx);
