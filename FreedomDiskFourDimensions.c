#include "FreedomDiskFourDimensions.h"
#include "int.h"
#include "DiskIO.h"
#include "ahci.h"
#include "kmalloc.h"
#include "str.h"

extern AllocPool KernelPool;

char* GetFDFDFolderTableName(FDFDFolderTable* Data, u8 Idx){
    if (!Data) return NULL;
    FDFDHeader* Header=(FDFDHeader*)ReadDisk(GlobalAHCICtrl, Idx, 1, 0);
    if (Header->Sign0xFDFD!=0xFDFD) {
        Free(&KernelPool, Header);
        return NULL;
    }
    Free(&KernelPool, Header);
    return Data->Sign0xAA01==0xAA01 && Data->Sign0x5501==0x5501 ? (char*)ReadDisk(GlobalAHCICtrl, Idx, Data->SizeOfName, Data->OffSetOfSector) : NULL;
}

char* GetFDFDFileTableName(FDFDFileTable* Data, u8 Idx){
    if (!Data) return NULL;
    FDFDHeader* Header=(FDFDHeader*)ReadDisk(GlobalAHCICtrl, Idx, 1, 0);
    if (Header->Sign0xFDFD!=0xFDFD) {
        Free(&KernelPool, Header);
        return NULL;
    }
    Free(&KernelPool, Header);
    return Data->Sign0xAA00==0xAA00 && Data->Sign0x5500==0x5500 ? (char*)ReadDisk(GlobalAHCICtrl, Idx, Data->SizeOfName, Data->OffSetOfSector) : NULL;
}

u8* GetFDFDFolderTableBuffer(FDFDFolderTable* Data, u8 Idx){
    if (!Data) return NULL;
    FDFDHeader* Header=(FDFDHeader*)ReadDisk(GlobalAHCICtrl, Idx, 1, 0);
    if (Header->Sign0xFDFD!=0xFDFD) {
        Free(&KernelPool, Header);
        return NULL;
    }
    Free(&KernelPool, Header);
    return Data->Sign0xAA01==0xAA01 && Data->Sign0x5501==0x5501 ? ReadDisk(GlobalAHCICtrl, Idx, Data->SizeOfData, Data->OffSetOfSector) : NULL;
}

u8* GetFDFDFileTableBuffer(FDFDFileTable* Data, u8 Idx){
    if (!Data) return NULL;
    FDFDHeader* Header=(FDFDHeader*)ReadDisk(GlobalAHCICtrl, Idx, 1, 0);
    if (Header->Sign0xFDFD!=0xFDFD) {
        Free(&KernelPool, Header);
        return NULL;
    }
    return Data->Sign0xAA00==0xAA00 && Data->Sign0x5500==0x5500 ? ReadDisk(GlobalAHCICtrl, Idx, Data->SizeOfData, Data->OffSetOfSector) : NULL;
}

void NewFile(FDFDFileTable Table, u8 Idx, u8* Data) {
    if (!Data) return;
    
    FDFDHeader* Header = (FDFDHeader*)ReadDisk(GlobalAHCICtrl, Idx, 1, 0);
    if (!Header || Header->Sign0xFDFD != 0xFDFD) {
        if (Header) Free(&KernelPool, Header);
        return;
    }
    
    u64 TotalBytes = Header->CountOfSector * 512;
    u64 UsedBytes = Header->SizeOfData;
    u64 NeedBytes = sizeof(FDFDFileTable);
    
    if (UsedBytes + NeedBytes <= TotalBytes) {
        MemCopy(&Header->Data[UsedBytes], &Table, NeedBytes);
        Header->SizeOfData = UsedBytes + NeedBytes;
        WriteDisk(GlobalAHCICtrl, Idx, Header, Header->CountOfSector, 0);
        Free(&KernelPool, Header);
        return;
    }
    
    u64 NewSectorCount = Header->CountOfSector + 1;
    u8* NewBuffer = ReadDisk(GlobalAHCICtrl, Idx, Header->CountOfSector, 0);
    if (!NewBuffer) {
        Free(&KernelPool, Header);
        return;
    }
    
    FDFDHeader* NewHeader = (FDFDHeader*)NewBuffer;
    NewHeader->CountOfSector = NewSectorCount;
    NewHeader->SizeOfData = UsedBytes + NeedBytes;

    if (NewHeader->Data[(NewHeader->CountOfSector+NewHeader->SizeOfData%512)-sizeof(FDFDFileTable)]==0x01){
        FDFDFolderTable* PreFolder=(FDFDFolderTable*)&NewHeader->Data[(NewHeader->CountOfSector+NewHeader->SizeOfData%512)-sizeof(FDFDFolderTable)-sizeof(FDFDFileTable)];
        u64 DataOffSet=(PreFolder->OffSetOfSector+PreFolder->SectorCount);
        WriteDisk(GlobalAHCICtrl, Idx, Data, Table.SectorCount, DataOffSet);
        Table.OffSetOfSector=DataOffSet;
        MemCopy(&NewBuffer[UsedBytes], &Table, NeedBytes);
        WriteDisk(GlobalAHCICtrl, Idx, NewBuffer, NewSectorCount, 0);
    }else if (NewHeader->Data[(NewHeader->CountOfSector+NewHeader->SizeOfData%512)-sizeof(FDFDFileTable)]==0x00){
        FDFDFileTable* PreFile=(FDFDFileTable*)&NewHeader->Data[(NewHeader->CountOfSector+NewHeader->SizeOfData%512)-sizeof(FDFDFileTable)-sizeof(FDFDFileTable)];
        u64 DataOffSet=(PreFile->OffSetOfSector+PreFile->SectorCount);
        WriteDisk(GlobalAHCICtrl, Idx, Data, Table.SectorCount, DataOffSet);
        Table.OffSetOfSector=DataOffSet;
        MemCopy(&NewBuffer[UsedBytes], &Table, NeedBytes);
        WriteDisk(GlobalAHCICtrl, Idx, NewBuffer, NewSectorCount, 0);
    }
    
    Free(&KernelPool, Header);
    Free(&KernelPool, NewBuffer);
}

void NewFolder(FDFDFolderTable Table, u8 Idx, u8* Data){
    if (!Data) return;
    
    FDFDHeader* Header = (FDFDHeader*)ReadDisk(GlobalAHCICtrl, Idx, 1, 0);
    if (!Header || Header->Sign0xFDFD != 0xFDFD) {
        if (Header) Free(&KernelPool, Header);
        return;
    }
    
    u64 TotalBytes = Header->CountOfSector * 512;
    u64 UsedBytes = Header->SizeOfData;
    u64 NeedBytes = sizeof(FDFDFolderTable);
    
    if (UsedBytes + NeedBytes <= TotalBytes) {
        MemCopy(&Header->Data[UsedBytes], &Table, NeedBytes);
        Header->SizeOfData = UsedBytes + NeedBytes;
        WriteDisk(GlobalAHCICtrl, Idx, Header, Header->CountOfSector, 0);
        Free(&KernelPool, Header);
        return;
    }
    
    u64 NewSectorCount = Header->CountOfSector + 1;
    u8* NewBuffer = ReadDisk(GlobalAHCICtrl, Idx, Header->CountOfSector, 0);
    if (!NewBuffer) {
        Free(&KernelPool, Header);
        return;
    }
    
    FDFDHeader* NewHeader = (FDFDHeader*)NewBuffer;
    NewHeader->CountOfSector = NewSectorCount;
    NewHeader->SizeOfData = UsedBytes + NeedBytes;

    if (NewHeader->Data[(NewHeader->CountOfSector+NewHeader->SizeOfData%512)-sizeof(FDFDFolderTable)]==0x01){
        FDFDFolderTable* PreFolder=(FDFDFolderTable*)&NewHeader->Data[(NewHeader->CountOfSector+NewHeader->SizeOfData%512)-sizeof(FDFDFolderTable)-sizeof(FDFDFolderTable)];
        u64 DataOffSet=(PreFolder->OffSetOfSector+PreFolder->SectorCount);
        WriteDisk(GlobalAHCICtrl, Idx, Data, Table.SectorCount, DataOffSet);
        Table.OffSetOfSector=DataOffSet;
        MemCopy(&NewBuffer[UsedBytes], &Table, NeedBytes);
        WriteDisk(GlobalAHCICtrl, Idx, NewBuffer, NewSectorCount, 0);
    }else if (NewHeader->Data[(NewHeader->CountOfSector+NewHeader->SizeOfData%512)-sizeof(FDFDFolderTable)]==0x00){
        FDFDFileTable* PreFile=(FDFDFileTable*)&NewHeader->Data[(NewHeader->CountOfSector+NewHeader->SizeOfData%512)-sizeof(FDFDFileTable)-sizeof(FDFDFolderTable)];
        u64 DataOffSet=(PreFile->OffSetOfSector+PreFile->SectorCount);
        WriteDisk(GlobalAHCICtrl, Idx, Data, Table.SectorCount, DataOffSet);
        Table.OffSetOfSector=DataOffSet;
        MemCopy(&NewBuffer[UsedBytes], &Table, NeedBytes);
        WriteDisk(GlobalAHCICtrl, Idx, NewBuffer, NewSectorCount, 0);
    }
    
    Free(&KernelPool, Header);
    Free(&KernelPool, NewBuffer);
}
