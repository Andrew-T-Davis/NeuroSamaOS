#include "exFAT.h"
#include "int.h"

static ExFATBootSector* Boot(u8* Volume){
    return (ExFATBootSector*)Volume;
}

static u16 BytesPerSector(u8* Volume){
    return (u16)(1u << Boot(Volume)->BytesPerSectorShift);
}

u64 GetExFATClusterSize(u8* Volume){
    return ((u64)1 << Boot(Volume)->BytesPerSectorShift)
         * ((u64)1 << Boot(Volume)->SectorsPerClusterShift);
}

u32 GetExFATClusterCount(u8* Volume){
    return Boot(Volume)->ClusterCount;
}

u32 GetExFATFatCount(u8* Volume){
    return Boot(Volume)->NumberOfFats;
}

u32 ExFATGetRootDirectoryCluster(u8* Volume){
    return Boot(Volume)->FirstClusterOfRootDirectory;
}

static u64 SectorToByte(u8* Volume, u64 Sector){
    return Sector * ((u64)1 << Boot(Volume)->BytesPerSectorShift);
}

u64 GetExFATClusterOffset(u8* Volume, u32 Cluster){
    ExFATBootSector* B = Boot(Volume);
    if (Cluster < 2 || Cluster > B->ClusterCount + 1)
        return 0;
    return SectorToByte(Volume, B->ClusterHeapOffset)
         + (u64)(Cluster - 2) * GetExFATClusterSize(Volume);
}

u64 GetExFATRootDirectoryOffset(u8* Volume){
    return GetExFATClusterOffset(Volume, Boot(Volume)->FirstClusterOfRootDirectory);
}

static u32* FatBase(u8* Volume){
    return (u32*)(Volume + SectorToByte(Volume, Boot(Volume)->FatOffset));
}

static int DirCapacity(u8* Volume){
    return (int)(GetExFATClusterSize(Volume) / 32);
}

static ExFATGenericDirectoryEntry* DirEntryAt(u8* Volume, u32 Cluster, int Index){
    return (ExFATGenericDirectoryEntry*)(Volume + GetExFATClusterOffset(Volume, Cluster) + (u64)Index * 32);
}

u32 ExFATGetBitmapCluster(u8* Volume){
    ExFATBootSector* B = Boot(Volume);
    int Capacity = DirCapacity(Volume);
    u32 Cluster = B->FirstClusterOfRootDirectory;
    u32 Guard = 0;
    while (Cluster >= 2 && Cluster <= B->ClusterCount + 1 && Guard++ < B->ClusterCount)
    {
        for (int i = 0; i < Capacity; i++)
        {
            ExFATGenericDirectoryEntry* E = DirEntryAt(Volume, Cluster, i);
            if (E->EntryType == 0x00)
                return 0;
            if (E->EntryType == 0x81)
                return E->FirstCluster;
        }
        Cluster = ExFATGetNextCluster(Volume, Cluster, 0);
    }
    return 0;
}

u32 GetExFATAllocationBitmapCluster(u8* Volume){
    return ExFATGetBitmapCluster(Volume);
}

u32 ExFATGetUpcaseCluster(u8* Volume){
    ExFATBootSector* B = Boot(Volume);
    int Capacity = DirCapacity(Volume);
    u32 Cluster = B->FirstClusterOfRootDirectory;
    u32 Guard = 0;
    while (Cluster >= 2 && Cluster <= B->ClusterCount + 1 && Guard++ < B->ClusterCount)
    {
        for (int i = 0; i < Capacity; i++)
        {
            ExFATGenericDirectoryEntry* E = DirEntryAt(Volume, Cluster, i);
            if (E->EntryType == 0x00)
                return 0;
            if (E->EntryType == 0x82)
                return E->FirstCluster;
        }
        Cluster = ExFATGetNextCluster(Volume, Cluster, 0);
    }
    return 0;
}

static u8* Bitmap(u8* Volume){
    u32 Cluster = ExFATGetBitmapCluster(Volume);
    if (Cluster < 2)
        return 0;
    return Volume + GetExFATClusterOffset(Volume, Cluster);
}

int ExFATIsClusterFree(u8* Volume, u32 Cluster){
    ExFATBootSector* B = Boot(Volume);
    if (Cluster < 2 || Cluster > B->ClusterCount + 1)
        return 0;
    u8* Map = Bitmap(Volume);
    if (!Map)
        return 0;
    u32 Index = Cluster - 2;
    return ((Map[Index >> 3] >> (Index & 7)) & 1) ? 0 : 1;
}

u32 ExFATAllocCluster(u8* Volume){
    ExFATBootSector* B = Boot(Volume);
    u8* Map = Bitmap(Volume);
    if (!Map)
        return 0;
    for (u32 Index = 0; Index < B->ClusterCount; Index++)
    {
        if (!((Map[Index >> 3] >> (Index & 7)) & 1))
        {
            Map[Index >> 3] |= (u8)(1u << (Index & 7));
            return Index + 2;
        }
    }
    return 0;
}

int ExFATFreeCluster(u8* Volume, u32 Cluster){
    ExFATBootSector* B = Boot(Volume);
    if (Cluster < 2 || Cluster > B->ClusterCount + 1)
        return 0;
    u8* Map = Bitmap(Volume);
    if (!Map)
        return 0;
    u32 Index = Cluster - 2;
    Map[Index >> 3] &= (u8)~(1u << (Index & 7));
    return 1;
}

u32 ExFATGetNextCluster(u8* Volume, u32 Cluster, int NoFatChain){
    ExFATBootSector* B = Boot(Volume);
    if (Cluster < 2 || Cluster > B->ClusterCount + 1)
        return 0;
    if (NoFatChain)
        return Cluster + 1;
    u32 Next = FatBase(Volume)[Cluster];
    if (Next == 0xFFFFFFFFu || Next == 0xFFFFFFF7u)
        return 0;
    if (Next < 2 || Next > B->ClusterCount + 1)
        return 0;
    return Next;
}

int ExFATSetNextCluster(u8* Volume, u32 Cluster, u32 Next){
    ExFATBootSector* B = Boot(Volume);
    if (Cluster < 2 || Cluster > B->ClusterCount + 1)
        return 0;
    FatBase(Volume)[Cluster] = Next;
    return 1;
}

static int FindEntrySet(u8* Volume, u32 DirCluster, const u16* Name, u8 NameLength,
                        ExFATFileDirectoryEntry** OutFile,
                        ExFATStreamExtensionDirectoryEntry** OutStream){
    ExFATBootSector* B = Boot(Volume);
    int Capacity = DirCapacity(Volume);
    u32 Cluster = DirCluster;
    u32 Guard = 0;
    while (Cluster >= 2 && Cluster <= B->ClusterCount + 1 && Guard++ < B->ClusterCount)
    {
        for (int i = 0; i < Capacity; i++)
        {
            ExFATGenericDirectoryEntry* E = DirEntryAt(Volume, Cluster, i);
            if (E->EntryType == 0x00)
                return 0;
            if (E->EntryType != 0x85)
                continue;
            ExFATFileDirectoryEntry* File = (ExFATFileDirectoryEntry*)E;
            if (i + File->SecondaryCount >= Capacity)
                continue;
            ExFATStreamExtensionDirectoryEntry* Stream =
                (ExFATStreamExtensionDirectoryEntry*)DirEntryAt(Volume, Cluster, i + 1);
            if (Stream->EntryType != 0xC0 || Stream->NameLength != NameLength)
                continue;
            u16* Stored = (u16*)DirEntryAt(Volume, Cluster, i + 2);
            int Match = 1;
            for (u8 n = 0; n < NameLength; n++)
            {
                u16 a = Stored[n];
                u16 b = Name[n];
                if (a >= 'a' && a <= 'z') a -= 0x20;
                if (b >= 'a' && b <= 'z') b -= 0x20;
                if (a != b) { Match = 0; break; }
            }
            if (!Match)
                continue;
            if (ExFATNameHash(Name, NameLength) != Stream->NameHash)
                continue;
            if (OutFile) *OutFile = File;
            if (OutStream) *OutStream = Stream;
            return 1;
        }
        Cluster = ExFATGetNextCluster(Volume, Cluster, 0);
    }
    return 0;
}

ExFATFileDirectoryEntry* FindExFATFile(u8* Volume, u32 DirCluster, const u16* Name, u8 NameLength){
    ExFATFileDirectoryEntry* File = 0;
    if (FindEntrySet(Volume, DirCluster, Name, NameLength, &File, 0))
        return File;
    return 0;
}

ExFATFileDirectoryEntry* FindExFATFolder(u8* Volume, u32 DirCluster, const u16* Name, u8 NameLength){
    ExFATFileDirectoryEntry* File = FindExFATFile(Volume, DirCluster, Name, NameLength);
    if (File && (File->FileAttributes & 0x10))
        return File;
    return 0;
}

u16 GetExFATFileAttributes(ExFATFileDirectoryEntry* File){
    return File->FileAttributes;
}

u32 GetExFATFileFirstCluster(ExFATFileDirectoryEntry* File){
    return ((ExFATStreamExtensionDirectoryEntry*)((u8*)File + 32))->FirstCluster;
}

u64 GetExFATFileSize(ExFATFileDirectoryEntry* File){
    return ((ExFATStreamExtensionDirectoryEntry*)((u8*)File + 32))->DataLength;
}

char* GetExFATFileName(ExFATFileDirectoryEntry* File){
    return (char*)((u8*)File + 64);
}

u8* GetExFATFileBuffer(ExFATFileDirectoryEntry* File){
    return (u8*)File;
}

static int FreeChain(u8* Volume, u32 FirstCluster, int NoFatChain){
    ExFATBootSector* B = Boot(Volume);
    u32 Cluster = FirstCluster;
    u32 Guard = 0;
    while (Cluster >= 2 && Cluster <= B->ClusterCount + 1 && Guard++ < B->ClusterCount)
    {
        u32 Next = ExFATGetNextCluster(Volume, Cluster, NoFatChain);
        ExFATFreeCluster(Volume, Cluster);
        if (Next == 0)
            break;
        Cluster = Next;
    }
    return 1;
}

int NewExFATFile(u8* Volume, u32 DirCluster, const u16* Name, u8 NameLength, u16 Attributes){
    ExFATBootSector* B = Boot(Volume);
    if (FindExFATFile(Volume, DirCluster, Name, NameLength))
        return 0;
    int Capacity = DirCapacity(Volume);
    int Need = 2 + (NameLength + 14) / 15;
    u32 Cluster = DirCluster;
    u32 Guard = 0;
    while (Cluster >= 2 && Cluster <= B->ClusterCount + 1 && Guard++ < B->ClusterCount)
    {
        for (int i = 0; i < Capacity; i++)
        {
            if (i + Need >= Capacity)
                break;
            int Space = 1;
            for (int s = 0; s < Need; s++)
            {
                ExFATGenericDirectoryEntry* E = DirEntryAt(Volume, Cluster, i + s);
                if (E->EntryType != 0x00 && E->EntryType != 0x01)
                {
                    Space = 0;
                    break;
                }
            }
            if (!Space)
                continue;

            u8 SecondaryCount = (u8)(1 + (NameLength + 14) / 15);

            ExFATFileDirectoryEntry* File = (ExFATFileDirectoryEntry*)DirEntryAt(Volume, Cluster, i);
            u8* Raw = (u8*)File;
            for (int b = 0; b < 32; b++) Raw[b] = 0;
            File->EntryType = 0x85;
            File->SecondaryCount = SecondaryCount;
            File->FileAttributes = Attributes;

            ExFATStreamExtensionDirectoryEntry* Stream =
                (ExFATStreamExtensionDirectoryEntry*)DirEntryAt(Volume, Cluster, i + 1);
            Raw = (u8*)Stream;
            for (int b = 0; b < 32; b++) Raw[b] = 0;
            Stream->EntryType = 0xC0;
            Stream->GeneralSecondaryFlags = 0x03;
            Stream->NameLength = NameLength;
            Stream->NameHash = (u16)ExFATNameHash(Name, NameLength);
            Stream->FirstCluster = 0;
            Stream->DataLength = 0;
            Stream->ValidDataLength = 0;

            u16* Stored = (u16*)DirEntryAt(Volume, Cluster, i + 2);
            for (u8 n = 0; n < NameLength; n++)
                Stored[n] = Name[n];

            File->SetChecksum = ExFATEntrySetChecksum((u8*)File, SecondaryCount);
            return 1;
        }
        Cluster = ExFATGetNextCluster(Volume, Cluster, 0);
    }
    return 0;
}

int NewExFATFolder(u8* Volume, u32 DirCluster, const u16* Name, u8 NameLength){
    u32 NewCluster = ExFATAllocCluster(Volume);
    if (NewCluster == 0)
        return 0;
    ExFATSetNextCluster(Volume, NewCluster, 0xFFFFFFFFu);

    u8* Raw = Volume + GetExFATClusterOffset(Volume, NewCluster);
    u64 Bytes = GetExFATClusterSize(Volume);
    for (u64 i = 0; i < Bytes; i++)
        Raw[i] = 0;

    if (!NewExFATFile(Volume, DirCluster, Name, NameLength, 0x10))
    {
        ExFATFreeCluster(Volume, NewCluster);
        return 0;
    }

    ExFATFileDirectoryEntry* File = FindExFATFile(Volume, DirCluster, Name, NameLength);
    if (!File)
    {
        ExFATFreeCluster(Volume, NewCluster);
        return 0;
    }

    ExFATStreamExtensionDirectoryEntry* Stream =
        (ExFATStreamExtensionDirectoryEntry*)((u8*)File + 32);
    Stream->GeneralSecondaryFlags = 0x01;
    Stream->FirstCluster = NewCluster;
    Stream->DataLength = GetExFATClusterSize(Volume);
    Stream->ValidDataLength = GetExFATClusterSize(Volume);
    File->SetChecksum = ExFATEntrySetChecksum((u8*)File, File->SecondaryCount);
    return 1;
}

static u64 FileWrite(u8* Volume, ExFATStreamExtensionDirectoryEntry* Stream,
                     const u8* Buffer, u64 Offset, u64 Length){
    u64 DataLength = Stream->DataLength;
    if (Offset >= DataLength)
        return 0;
    if (Length > DataLength - Offset)
        Length = DataLength - Offset;
    if (Length == 0)
        return 0;

    u64 ClusterBytes = GetExFATClusterSize(Volume);
    int NoFatChain = (Stream->GeneralSecondaryFlags & 0x02) ? 1 : 0;
    u32 Cluster = Stream->FirstCluster;

    if (Cluster == 0)
    {
        Cluster = ExFATAllocCluster(Volume);
        if (Cluster == 0)
            return 0;
        ExFATSetNextCluster(Volume, Cluster, 0xFFFFFFFFu);
        Stream->FirstCluster = Cluster;
    }

    while (Offset >= ClusterBytes)
    {
        u32 Next = ExFATGetNextCluster(Volume, Cluster, NoFatChain);
        if (Next == 0)
        {
            u32 New = ExFATAllocCluster(Volume);
            if (New == 0)
                return 0;
            ExFATSetNextCluster(Volume, Cluster, New);
            Next = New;
        }
        Cluster = Next;
        Offset -= ClusterBytes;
    }

    u64 Written = 0;
    while (Written < Length)
    {
        u8* ClusterData = Volume + GetExFATClusterOffset(Volume, Cluster);
        u64 Chunk = ClusterBytes - Offset;
        if (Chunk > Length - Written)
            Chunk = Length - Written;
        for (u64 i = 0; i < Chunk; i++)
            ClusterData[Offset + i] = Buffer[Written + i];
        Written += Chunk;
        Offset = 0;
        if (Written < Length)
        {
            u32 Next = ExFATGetNextCluster(Volume, Cluster, NoFatChain);
            if (Next == 0)
            {
                u32 New = ExFATAllocCluster(Volume);
                if (New == 0)
                    break;
                ExFATSetNextCluster(Volume, Cluster, New);
                Next = New;
            }
            Cluster = Next;
        }
    }
    return Written;
}

u64 WriteExFATFile(u8* Volume, const u16* Name, u8 NameLength,
                   const u8* Buffer, u64 Offset, u64 Length){
    ExFATFileDirectoryEntry* File = FindExFATFile(Volume,
        Boot(Volume)->FirstClusterOfRootDirectory, Name, NameLength);
    if (!File)
        return 0;
    ExFATStreamExtensionDirectoryEntry* Stream =
        (ExFATStreamExtensionDirectoryEntry*)((u8*)File + 32);
    u64 Written = FileWrite(Volume, Stream, Buffer, Offset, Length);
    if (Written && Offset + Written > Stream->ValidDataLength)
        Stream->ValidDataLength = Offset + Written;
    File->SetChecksum = ExFATEntrySetChecksum((u8*)File, File->SecondaryCount);
    return Written;
}

static u64 FileRead(u8* Volume, ExFATStreamExtensionDirectoryEntry* Stream,
                    u8* Buffer, u64 Offset, u64 Length){
    u64 DataLength = Stream->DataLength;
    if (Offset >= DataLength)
        return 0;
    if (Length > DataLength - Offset)
        Length = DataLength - Offset;
    if (Length == 0)
        return 0;

    u64 ClusterBytes = GetExFATClusterSize(Volume);
    int NoFatChain = (Stream->GeneralSecondaryFlags & 0x02) ? 1 : 0;
    u32 Cluster = Stream->FirstCluster;
    if (Cluster < 2)
        return 0;

    while (Offset >= ClusterBytes)
    {
        u32 Next = ExFATGetNextCluster(Volume, Cluster, NoFatChain);
        if (Next == 0)
            return 0;
        Cluster = Next;
        Offset -= ClusterBytes;
    }

    u64 Done = 0;
    while (Done < Length)
    {
        if (Cluster < 2 || Cluster > Boot(Volume)->ClusterCount + 1)
            break;
        u8* ClusterData = Volume + GetExFATClusterOffset(Volume, Cluster);
        u64 Chunk = ClusterBytes - Offset;
        if (Chunk > Length - Done)
            Chunk = Length - Done;
        for (u64 i = 0; i < Chunk; i++)
            Buffer[Done + i] = ClusterData[Offset + i];
        Done += Chunk;
        Offset = 0;
        if (Done < Length)
        {
            u32 Next = ExFATGetNextCluster(Volume, Cluster, NoFatChain);
            if (Next == 0)
                break;
            Cluster = Next;
        }
    }
    return Done;
}

u8* ReadExFATFile(u8* Volume, const u16* Name, u8 NameLength){
    ExFATFileDirectoryEntry* File = FindExFATFile(Volume,
        Boot(Volume)->FirstClusterOfRootDirectory, Name, NameLength);
    if (!File)
        return 0;
    ExFATStreamExtensionDirectoryEntry* Stream =
        (ExFATStreamExtensionDirectoryEntry*)((u8*)File + 32);
    if (Stream->DataLength == 0 || Stream->FirstCluster == 0)
        return 0;

    u8* Buffer = (u8*)Stream;
    FileRead(Volume, Stream, Buffer, 0, Stream->DataLength);
    return Buffer;
}

int DelExFATFile(u8* Volume, u32 DirCluster, const u16* Name, u8 NameLength){
    ExFATFileDirectoryEntry* File = 0;
    ExFATStreamExtensionDirectoryEntry* Stream = 0;
    if (!FindEntrySet(Volume, DirCluster, Name, NameLength, &File, &Stream))
        return 0;
    if (File->FileAttributes & 0x10)
        return 0;
    int NoFatChain = (Stream->GeneralSecondaryFlags & 0x02) ? 1 : 0;
    if (Stream->FirstCluster >= 2)
        FreeChain(Volume, Stream->FirstCluster, NoFatChain);
    u8* Raw = (u8*)File;
    for (u8 i = 0; i <= File->SecondaryCount; i++)
        Raw[i * 32] = 0x01;
    return 1;
}

int DelExFATFolder(u8* Volume, u32 DirCluster, const u16* Name, u8 NameLength){
    ExFATFileDirectoryEntry* File = 0;
    ExFATStreamExtensionDirectoryEntry* Stream = 0;
    if (!FindEntrySet(Volume, DirCluster, Name, NameLength, &File, &Stream))
        return 0;
    if (!(File->FileAttributes & 0x10))
        return 0;

    int Capacity = DirCapacity(Volume);
    u32 Cluster = Stream->FirstCluster;
    u32 Guard = 0;
    while (Cluster >= 2 && Cluster <= GetExFATClusterCount(Volume) + 1 && Guard++ < GetExFATClusterCount(Volume))
    {
        for (int i = 0; i < Capacity; i++)
        {
            ExFATGenericDirectoryEntry* E = DirEntryAt(Volume, Cluster, i);
            if (E->EntryType == 0x00)
                break;
            if (E->EntryType == 0x85)
                return 0;
        }
        Cluster = ExFATGetNextCluster(Volume, Cluster, 0);
    }

    if (Stream->FirstCluster >= 2)
        FreeChain(Volume, Stream->FirstCluster, 0);

    u8* Raw = (u8*)File;
    for (u8 i = 0; i <= File->SecondaryCount; i++)
        Raw[i * 32] = 0x01;
    return 1;
}

u32 ExFATBootChecksum(u8* Sectors, u16 BytesPerSector){
    u32 NumberOfBytes = (u32)BytesPerSector * 11;
    u32 Checksum = 0;
    for (u32 Index = 0; Index < NumberOfBytes; Index++)
    {
        if ((Index == 106) || (Index == 107) || (Index == 112))
            continue;
        Checksum = ((Checksum & 1) ? 0x80000000 : 0)
                 + (Checksum >> 1)
                 + (u32)Sectors[Index];
    }
    return Checksum;
}

u16 ExFATEntrySetChecksum(u8* Entries, u8 SecondaryCount){
    u16 NumberOfBytes = ((u16)SecondaryCount + 1) * 32;
    u16 Checksum = 0;
    for (u16 Index = 0; Index < NumberOfBytes; Index++)
    {
        if ((Index == 2) || (Index == 3))
            continue;
        Checksum = ((Checksum & 1) ? 0x8000 : 0)
                 + (Checksum >> 1)
                 + (u16)Entries[Index];
    }
    return Checksum;
}

u32 ExFATNameHash(const u16* FileName, u8 NameLength){
    u32 Hash = 0;
    for (u8 Index = 0; Index < NameLength; Index++)
    {
        u16 Character = FileName[Index];
        Hash = ((Hash & 1) ? 0x80000000 : 0)
             + (Hash >> 1)
             + (u32)(Character & 0x00FF);
        Hash = ((Hash & 1) ? 0x80000000 : 0)
             + (Hash >> 1)
             + (u32)((Character >> 8) & 0x00FF);
    }
    return Hash;
}

u16 ExFATUpcaseTableChecksum(const u8* UpcaseTable, u32 DataLength){
    u16 Checksum = 0;
    for (u32 Index = 0; Index < DataLength; Index++)
    {
        Checksum = ((Checksum & 1) ? 0x8000 : 0)
                 + (Checksum >> 1)
                 + (u16)UpcaseTable[Index];
    }
    return Checksum;
}

void InitDiskToExFAT(u8* Volume){
    u64 Bytes = SectorToByte(Volume, 24);
    for (u64 i = 0; i < Bytes; i++)
        Volume[i] = 0;
}

int MountExFAT(u8* Volume){
    ExFATBootSector* B = Boot(Volume);
    if (B->JumpBoot[0] != 0xEB || B->JumpBoot[1] != 0x76 || B->JumpBoot[2] != 0x90)
        return 0;
    u8 Name[8] = {'E','X','F','A','T',' ',' ',' '};
    for (int i = 0; i < 8; i++)
        if (B->FileSystemName[i] != Name[i])
            return 0;
    for (int i = 0; i < 53; i++)
        if (B->MustBeZero[i] != 0)
            return 0;
    if (B->BootSignature != 0xAA55)
        return 0;
    if (B->BytesPerSectorShift < 9 || B->BytesPerSectorShift > 12)
        return 0;
    if (B->SectorsPerClusterShift > 25 - B->BytesPerSectorShift)
        return 0;
    if (B->NumberOfFats != 1 && B->NumberOfFats != 2)
        return 0;
    if (B->FileSystemRevision >> 8 != 1)
        return 0;
    if (B->FirstClusterOfRootDirectory < 2)
        return 0;
    if (B->FirstClusterOfRootDirectory > B->ClusterCount + 1)
        return 0;
    u16 Bps = BytesPerSector(Volume);
    u32 Expected = ExFATBootChecksum(Volume, Bps);
    u8* Region = Volume + (u64)11 * Bps;
    u32* Stored = (u32*)Region;
    for (u32 i = 0; i < Bps / 4; i++)
        if (Stored[i] != Expected)
            return 0;
    return 1;
}

int UnmountExFAT(u8* Volume){
    (void)Volume;
    return 1;
}

int FormatExFAT(u8* Volume, u64 VolumeLength, u8 BytesPerSectorShift, u8 SectorsPerClusterShift){
    u64 Bytes = VolumeLength * ((u64)1 << BytesPerSectorShift);
    for (u64 i = 0; i < Bytes; i++)
        Volume[i] = 0;

    ExFATBootSector* B = Boot(Volume);
    B->JumpBoot[0] = 0xEB;
    B->JumpBoot[1] = 0x76;
    B->JumpBoot[2] = 0x90;
    u8 Name[8] = {'E','X','F','A','T',' ',' ',' '};
    for (int i = 0; i < 8; i++)
        B->FileSystemName[i] = Name[i];
    B->PartitionOffset = 0;
    B->VolumeLength = VolumeLength;
    B->FatOffset = 24;
    B->FatLength = (u32)(((VolumeLength / 2) >> BytesPerSectorShift) + 1);
    if (B->FatLength < 4)
        B->FatLength = 4;
    B->ClusterHeapOffset = B->FatOffset + B->FatLength;
    B->ClusterCount = (u32)((VolumeLength - B->ClusterHeapOffset) >> SectorsPerClusterShift);
    B->FirstClusterOfRootDirectory = 2;
    B->VolumeSerialNumber = 0x12345678;
    B->FileSystemRevision = 0x0100;
    B->VolumeFlags = 0;
    B->BytesPerSectorShift = BytesPerSectorShift;
    B->SectorsPerClusterShift = SectorsPerClusterShift;
    B->NumberOfFats = 1;
    B->DriveSelect = 0x80;
    B->PercentInUse = 0xFF;
    B->BootSignature = 0xAA55;

    u16 Bps = BytesPerSector(Volume);
    u32 Checksum = ExFATBootChecksum(Volume, Bps);
    u8* Region = Volume + (u64)11 * Bps;
    u32* Stored = (u32*)Region;
    for (u32 i = 0; i < Bps / 4; i++)
        Stored[i] = Checksum;

    u32* Fat = FatBase(Volume);
    Fat[0] = 0xFFFFFFF8u;
    Fat[1] = 0xFFFFFFFFu;
    for (u32 i = 2; i <= B->ClusterCount + 1; i++)
        Fat[i] = 0;

    u8* Root = Volume + GetExFATClusterOffset(Volume, 2);
    u64 ClusterBytes = GetExFATClusterSize(Volume);
    for (u64 i = 0; i < ClusterBytes; i++)
        Root[i] = 0;

    u32 BitmapCluster = ExFATAllocCluster(Volume);
    u32 UpcaseCluster = ExFATAllocCluster(Volume);
    if (BitmapCluster == 0 || UpcaseCluster == 0)
        return 0;

    Fat[BitmapCluster] = 0xFFFFFFFFu;
    Fat[UpcaseCluster] = 0xFFFFFFFFu;

    u8* BitmapData = Volume + GetExFATClusterOffset(Volume, BitmapCluster);
    for (u64 i = 0; i < ClusterBytes; i++)
        BitmapData[i] = 0;
    BitmapData[0] |= (u8)(1u << 0);
    BitmapData[0] |= (u8)(1u << 1);
    BitmapData[0] |= (u8)(1u << 2);

    u8* Upcase = Volume + GetExFATClusterOffset(Volume, UpcaseCluster);
    for (u64 i = 0; i < ClusterBytes; i++)
        Upcase[i] = 0;
    for (u32 i = 0; i < 0x10000 && (u64)i * 2 + 1 < ClusterBytes; i++)
    {
        u16 C = (u16)i;
        if (C >= 'a' && C <= 'z')
            C -= 0x20;
        Upcase[i * 2] = (u8)(C & 0xFF);
        Upcase[i * 2 + 1] = (u8)(C >> 8);
    }

    ExFATGenericDirectoryEntry* Entry = (ExFATGenericDirectoryEntry*)Root;

    ExFATAllocationBitmapDirectoryEntry* BitmapEntry = (ExFATAllocationBitmapDirectoryEntry*)&Entry[0];
    for (int i = 0; i < 32; i++)
        ((u8*)BitmapEntry)[i] = 0;
    BitmapEntry->EntryType = 0x81;
    BitmapEntry->FirstCluster = BitmapCluster;
    BitmapEntry->DataLength = ClusterBytes;

    ExFATUpcaseTableDirectoryEntry* UpcaseEntry = (ExFATUpcaseTableDirectoryEntry*)&Entry[1];
    for (int i = 0; i < 32; i++)
        ((u8*)UpcaseEntry)[i] = 0;
    UpcaseEntry->EntryType = 0x82;
    UpcaseEntry->TableChecksum = ExFATUpcaseTableChecksum(Upcase, (u32)ClusterBytes);
    UpcaseEntry->FirstCluster = UpcaseCluster;
    UpcaseEntry->DataLength = ClusterBytes;
    
    return 1;
}