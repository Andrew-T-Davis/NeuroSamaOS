#include "FreedomDiskFourDimensions.h"
#include "debug.h"
#include "flash.h"
#include "int.h"
#include "DiskIO.h"
#include "ahci.h"
#include "kmalloc.h"
#include "str.h"

extern AllocPool KernelPool;

static FDFDHeader* ReadAndCheckHeader(u8 Idx) {
    FDFDHeader* Header = (FDFDHeader*)ReadDisk(GlobalAHCICtrl, Idx, 1, 0);
    if (!Header) return NULL;
    if (Header->Sign0xFDFD != 0xFDFD) {
        Free(&KernelPool, Header);
        return NULL;
    }
    return Header;
}

static FDFDHeader* ReadWholeHeader(u8 Idx, u64* OutSectorCount) {
    FDFDHeader* First = (FDFDHeader*)ReadDisk(GlobalAHCICtrl, Idx, 1, 0);
    if (!First) return NULL;
    if (First->Sign0xFDFD != 0xFDFD) {
        Free(&KernelPool, First);
        return NULL;
    }
    u64 Count = First->CountOfSector;
    Free(&KernelPool, First);
    if (Count == 0) Count = 1;

    FDFDHeader* Whole = (FDFDHeader*)ReadDisk(GlobalAHCICtrl, Idx, Count, 0);
    if (OutSectorCount) *OutSectorCount = Count;
    return Whole;
}

static char* GetTableName(u8 Idx, u64 SizeOfName, u64 OffSetOfSector) {
    FDFDHeader* Header = ReadAndCheckHeader(Idx);
    if (!Header) return NULL;
    Free(&KernelPool, Header);

    if (SizeOfName == 0) return NULL;
    return (char*)ReadDisk(GlobalAHCICtrl, Idx, SizeOfName, OffSetOfSector);
}

static u8* GetTableBuffer(u8 Idx, u64 SizeOfData, u64 OffSetOfSector) {
    FDFDHeader* Header = ReadAndCheckHeader(Idx);
    if (!Header) return NULL;
    Free(&KernelPool, Header);

    if (SizeOfData == 0) return NULL;
    return ReadDisk(GlobalAHCICtrl, Idx, SizeOfData, OffSetOfSector);
}

static int IsValidTable(FDFDAnyTable* Any) {
    return Any->SignStart.SignHigh == 0x55 && Any->SignEnd.SignHigh == 0xAA;
}

static int IsEmptySlot(FDFDAnyTable* Any) {
    return Any->SignStart.Sign0x55XX == 0 && Any->SignEnd.Sign0xAAXX == 0;
}

static u64 FindEmptySlot(FDFDHeader* Whole) {
    u64 UsedBytes = Whole->SizeOfData;
    u64 TableNum  = UsedBytes / FDFDTableSize;
    for (u64 i = 0; i < TableNum; i++) {
        if (IsEmptySlot(&Whole->Any[i]))
            return i;
    }
    return (u64)-1;
}

static FDFDAnyTable* FindLastValidTable(FDFDHeader* Whole) {
    u64 UsedBytes = Whole->SizeOfData;
    u64 TableNum  = UsedBytes / FDFDTableSize;
    FDFDAnyTable* Last = NULL;
    for (u64 i = 0; i < TableNum; i++) {
        if (IsValidTable(&Whole->Any[i]))
            Last = &Whole->Any[i];
    }
    return Last;
}

static u64 FindHoleSlot(FDFDHeader* Whole, u64 NeedSectors) {
    u64 UsedBytes = Whole->SizeOfData;
    u64 TableNum  = UsedBytes / FDFDTableSize;
    u64 BestSlot  = (u64)-1;
    u64 BestSize  = 0;

    for (u64 i = 0; i < TableNum; i++) {
        FDFDAnyTable* Any = &Whole->Any[i];
        if (!IsEmptySlot(Any)) continue;
        if (Any->SectorCount < NeedSectors) continue;
        if (BestSlot == (u64)-1 || Any->SectorCount < BestSize) {
            BestSlot = i;
            BestSize = Any->SectorCount;
        }
    }
    return BestSlot;
}

static int TableNameEquals(u8 Idx, FDFDAnyTable* Any, const char* Name, u64 NameLen) {
    if (Any->SizeOfName == 0) return 0;
    if (Any->SizeOfName != NameLen) return 0;

    char* TableName = (char*)ReadDisk(GlobalAHCICtrl, Idx,
                                      Any->SizeOfName, Any->OffSetOfSector);
    if (!TableName) return 0;

    int Eq = (MemCmp(TableName, Name, NameLen) == 0);
    Free(&KernelPool, TableName);
    return Eq;
}

static u64 FindTableSlot(FDFDHeader* Whole, u8 Idx, const char* Name, u8 TypeLow) {
    u64 UsedBytes = Whole->SizeOfData;
    u64 TableNum  = UsedBytes / FDFDTableSize;
    u64 NameLen   = StrLen(Name);

    for (u64 i = 0; i < TableNum; i++) {
        FDFDAnyTable* Any = &Whole->Any[i];
        if (!IsValidTable(Any)) continue;
        if (Any->SignStart.SignLow != TypeLow || Any->SignEnd.SignLow != TypeLow) continue;
        if (TableNameEquals(Idx, Any, Name, NameLen)) return i;
    }
    return (u64)-1;
}

static void AppendTableEntry(const void* Table, u64 TableSize, u8 Idx,
                             const u8* Data, u64 DataSectors) {
    if (!Table || TableSize == 0) return;

    u64 SectorCount = 0;
    FDFDHeader* Whole = ReadWholeHeader(Idx, &SectorCount);
    if (!Whole) return;

    u64 TotalBytes = SectorCount * 512;
    u64 UsedBytes  = Whole->SizeOfData;

    if (!Data || DataSectors == 0) {
        u64 EmptySlot = FindEmptySlot(Whole);
        if (EmptySlot != (u64)-1) {
            FDFDAnyTable* Slot = &Whole->Any[EmptySlot];
            MemSet((u8*)Slot, 0, FDFDTableSize);
            MemCopy((u8*)Slot, Table, TableSize);
            Slot->OffSetOfSector = 0;
            Slot->SectorCount    = 0;
        } else {
            if (UsedBytes + TableSize > TotalBytes) {
                Free(&KernelPool, Whole);
                return;
            }
            MemCopy((u8*)Whole->Data + UsedBytes, Table, TableSize);
            Whole->SizeOfData = UsedBytes + TableSize;
        }
        WriteDisk(GlobalAHCICtrl, Idx, Whole, SectorCount, 0);
        Free(&KernelPool, Whole);
        return;
    }

    u64 DataOffSet;

    u64 HoleSlot = FindHoleSlot(Whole, DataSectors);
    if (HoleSlot != (u64)-1) {
        DataOffSet = Whole->Any[HoleSlot].OffSetOfSector;
        WriteDisk(GlobalAHCICtrl, Idx, (u8*)Data, DataSectors, DataOffSet);

        u8 TableCopy[FDFDTableSize];
        MemCopy(TableCopy, Table, TableSize);
        FDFDAnyTable* T = (FDFDAnyTable*)TableCopy;
        T->OffSetOfSector = DataOffSet;
        T->SectorCount    = DataSectors;

        MemCopy((u8*)&Whole->Any[HoleSlot], TableCopy, TableSize);
        WriteDisk(GlobalAHCICtrl, Idx, Whole, SectorCount, 0);
        Free(&KernelPool, Whole);
        return;
    }

    FDFDAnyTable* LastValid = FindLastValidTable(Whole);
    if (!LastValid) {
        DataOffSet = SectorCount;
    } else {
        DataOffSet = LastValid->OffSetOfSector + LastValid->SectorCount;
    }

    WriteDisk(GlobalAHCICtrl, Idx, (u8*)Data, DataSectors, DataOffSet);

    u8 TableCopy[FDFDTableSize];
    MemCopy(TableCopy, Table, TableSize);
    FDFDAnyTable* T = (FDFDAnyTable*)TableCopy;
    T->OffSetOfSector = DataOffSet;

    u64 EmptySlot = FindEmptySlot(Whole);
    if (EmptySlot != (u64)-1) {
        MemCopy((u8*)&Whole->Any[EmptySlot], TableCopy, TableSize);
        WriteDisk(GlobalAHCICtrl, Idx, Whole, SectorCount, 0);
        Free(&KernelPool, Whole);
        return;
    }

    if (UsedBytes + TableSize <= TotalBytes) {
        MemCopy((u8*)Whole->Data + UsedBytes, TableCopy, TableSize);
        Whole->SizeOfData = UsedBytes + TableSize;
        WriteDisk(GlobalAHCICtrl, Idx, Whole, SectorCount, 0);
        Free(&KernelPool, Whole);
        return;
    }

    u64 NewSectorCount = SectorCount + 1;
    u8* NewBuffer = (u8*)ReadDisk(GlobalAHCICtrl, Idx, NewSectorCount, 0);
    if (!NewBuffer) {
        Free(&KernelPool, Whole);
        return;
    }

    FDFDHeader* NewHeader = (FDFDHeader*)NewBuffer;
    NewHeader->CountOfSector = NewSectorCount;
    NewHeader->SizeOfData    = UsedBytes + TableSize;

    MemCopy((u8*)NewHeader->Data + UsedBytes, TableCopy, TableSize);
    WriteDisk(GlobalAHCICtrl, Idx, NewBuffer, NewSectorCount, 0);

    Free(&KernelPool, Whole);
    Free(&KernelPool, NewBuffer);
}

static int DeleteTableEntry(u8 Idx, const char* Name, u8 TypeLow) {
    if (!Name) return -1;

    u64 SectorCount = 0;
    FDFDHeader* Whole = ReadWholeHeader(Idx, &SectorCount);
    if (!Whole) return -1;

    u64 Slot = FindTableSlot(Whole, Idx, Name, TypeLow);
    if (Slot == (u64)-1) {
        Free(&KernelPool, Whole);
        return -1;
    }

    FDFDAnyTable* Any = &Whole->Any[Slot];
    Any->SignStart.Sign0x55XX = 0;
    Any->SignEnd.Sign0xAAXX   = 0;
    Any->SizeOfName = 0;
    Any->SizeOfData = 0;

    WriteDisk(GlobalAHCICtrl, Idx, Whole, SectorCount, 0);
    Free(&KernelPool, Whole);
    return 0;
}

char* GetFDFDFolderTableName(FDFDFolderTable* Data, u8 Idx) {
    if (!Data) return NULL;
    if (Data->SignStart.Sign0x5501 != 0x5501 || Data->SignEnd.Sign0xAA01 != 0xAA01) return NULL;
    return GetTableName(Idx, Data->SizeOfName, Data->OffSetOfSector);
}

char* GetFDFDFileTableName(FDFDFileTable* Data, u8 Idx) {
    if (!Data) return NULL;
    if (Data->SignStart.Sign0x5500 != 0x5500 || Data->SignEnd.Sign0xAA00 != 0xAA00) return NULL;
    return GetTableName(Idx, Data->SizeOfName, Data->OffSetOfSector);
}

u8* GetFDFDFolderTableBuffer(FDFDFolderTable* Data, u8 Idx) {
    if (!Data) return NULL;
    if (Data->SignStart.Sign0x5501 != 0x5501 || Data->SignEnd.Sign0xAA01 != 0xAA01) return NULL;
    return GetTableBuffer(Idx, Data->SizeOfData, Data->OffSetOfSector);
}

u8* GetFDFDFileTableBuffer(FDFDFileTable* Data, u8 Idx) {
    if (!Data) return NULL;
    if (Data->SignStart.Sign0x5500 != 0x5500 || Data->SignEnd.Sign0xAA00 != 0xAA00) return NULL;
    return GetTableBuffer(Idx, Data->SizeOfData, Data->OffSetOfSector);
}

void NewFile(FDFDFileTable Table, u8 Idx, u8* Data) {
    AppendTableEntry(&Table, sizeof(FDFDFileTable), Idx, Data, Table.SectorCount);
}

void NewFolder(FDFDFolderTable Table, u8 Idx, u8* Data) {
    AppendTableEntry(&Table, sizeof(FDFDFolderTable), Idx, Data, Table.SectorCount);
}

int DelFile(u8 Idx, const char* Name) {
    return DeleteTableEntry(Idx, Name, 0x00);
}

int DelFolder(u8 Idx, const char* Name) {
    return DeleteTableEntry(Idx, Name, 0x01);
}

u8* ReadFile(u8 Idx, char* Name) {
    if (!Name) return NULL;

    u64 SectorCount = 0;
    FDFDHeader* Whole = ReadWholeHeader(Idx, &SectorCount);
    if (!Whole) return NULL;

    u64 Slot = FindTableSlot(Whole, Idx, Name, 0x00);
    if (Slot == (u64)-1) {
        Free(&KernelPool, Whole);
        return NULL;
    }

    FDFDAnyTable* Any = &Whole->Any[Slot];
    u8* Result = NULL;
    if (Any->SizeOfData > 0) {
        Result = ReadDisk(GlobalAHCICtrl, Idx,
                          Any->SizeOfData, Any->OffSetOfSector);
    }

    Free(&KernelPool, Whole);
    return Result;
}

void InitDiskToFDFD(u8 Idx) {
    u8 Buffer[512];
    MemSet(Buffer, 0, 512);
    ReadDiskToBuffer(GlobalAHCICtrl, Idx, 1, 0, Buffer);

    FDFDHeader* Header = (FDFDHeader*)Buffer;
    if (Header->Sign0xFDFD==0xFDFD && Header->Any[0].SignStart.SignHigh==0x55 && Header->Any[0].SignEnd.SignHigh==0xAA){
        u64 Times=Header->CountOfSector;
        if (Times==0) goto NotFDFD;
        if (Times > 0x10000000ULL) Times = 0x10000000ULL;
        MemSet(Buffer, 0, 512);
        for (u64 SecIdx=0;SecIdx<Times;SecIdx++){
            WriteDisk(GlobalAHCICtrl, Idx, Buffer, 1, SecIdx);
        }
        Header->Sign0xFDFD    = 0xFDFD;
        Header->CountOfSector = 1;
        Header->SizeOfData    = 0;
    }else{
        NotFDFD:
        MemSet(Buffer, 0, 512);
        Header->Sign0xFDFD    = 0xFDFD;
        Header->CountOfSector = 1;
        Header->SizeOfData    = 0;
    }

    MemFullFlash();

    WriteDisk(GlobalAHCICtrl, Idx, Buffer, 1, 0);

    u8 Verify[512];
    MemSet(Verify, 0, 512);
    ReadDiskToBuffer(GlobalAHCICtrl, Idx, 1, 0, Verify);

    if (MemCmp(Verify, Buffer, sizeof(FDFDHeader)) == 0) {
        DebugStr("DMA WRITE/READ OK\n");
    } else {
        DebugStr("DMA MISMATCH!\n");
    }
}