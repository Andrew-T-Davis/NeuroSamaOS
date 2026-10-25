#pragma once

#include "int.h"

typedef struct {
    u8 JumpBoot[3];
    u8 FileSystemName[8];
    u8 MustBeZero[53];
    u64 PartitionOffset;
    u64 VolumeLength;
    u32 FatOffset;
    u32 FatLength;
    u32 ClusterHeapOffset;
    u32 ClusterCount;
    u32 FirstClusterOfRootDirectory;
    u32 VolumeSerialNumber;
    u16 FileSystemRevision;
    u16 VolumeFlags;
    u8 BytesPerSectorShift;
    u8 SectorsPerClusterShift;
    u8 NumberOfFats;
    u8 DriveSelect;
    u8 PercentInUse;
    u8 Reserved[7];
    u8 BootCode[390];
    u16 BootSignature;
    u8 ExcessSpace[];
} __attribute__((packed)) ExFATBootSector;

typedef struct {
    u8 ExtendedBootCode[1];
} __attribute__((packed)) ExFATExtendedBootSector;

typedef struct {
    u8 ParametersGuid[16];
    u8 CustomDefined[32];
} __attribute__((packed)) ExFATGenericParameters;

typedef struct {
    u8 ParametersGuid[16];
    u8 Reserved[32];
} __attribute__((packed)) ExFATNullParameters;

typedef struct {
    u8 ParametersGuid[16];
    u32 EraseBlockSize;
    u32 PageSize;
    u32 SpareSectors;
    u32 RandomAccessTime;
    u32 ProgrammingTime;
    u32 ReadCycle;
    u32 WriteCycle;
    u8 Reserved[4];
} __attribute__((packed)) ExFATFlashParameters;

typedef struct {
    ExFATGenericParameters Parameters[10];
    u8 Reserved[1];
} __attribute__((packed)) ExFATOEMParameters;

typedef struct {
    u32 FatEntry[1];
} __attribute__((packed)) ExFATFileAllocationTable;

typedef struct {
    u8 Cluster[1];
} __attribute__((packed)) ExFATClusterHeap;

typedef struct {
    u8 EntryType;
    u8 CustomDefined[19];
    u32 FirstCluster;
    u64 DataLength;
} __attribute__((packed)) ExFATGenericDirectoryEntry;

typedef struct {
    u8 EntryType;
    u8 SecondaryCount;
    u16 SetChecksum;
    u16 GeneralPrimaryFlags;
    u8 CustomDefined[14];
    u32 FirstCluster;
    u64 DataLength;
} __attribute__((packed)) ExFATGenericPrimaryDirectoryEntry;

typedef struct {
    u8 EntryType;
    u8 GeneralSecondaryFlags;
    u8 CustomDefined[18];
    u32 FirstCluster;
    u64 DataLength;
} __attribute__((packed)) ExFATGenericSecondaryDirectoryEntry;

typedef struct {
    u8 EntryType;
    u8 BitmapFlags;
    u8 Reserved[18];
    u32 FirstCluster;
    u64 DataLength;
} __attribute__((packed)) ExFATAllocationBitmapDirectoryEntry;

typedef struct {
    u8 EntryType;
    u8 Reserved1[3];
    u32 TableChecksum;
    u8 Reserved2[12];
    u32 FirstCluster;
    u64 DataLength;
} __attribute__((packed)) ExFATUpcaseTableDirectoryEntry;

typedef struct {
    u8 EntryType;
    u8 CharacterCount;
    u16 VolumeLabel[11];
    u8 Reserved[8];
} __attribute__((packed)) ExFATVolumeLabelDirectoryEntry;

typedef struct {
    u8 EntryType;
    u8 SecondaryCount;
    u16 SetChecksum;
    u16 FileAttributes;
    u8 Reserved1[2];
    u32 CreateTimestamp;
    u32 LastModifiedTimestamp;
    u32 LastAccessedTimestamp;
    u8 Create10msIncrement;
    u8 LastModified10msIncrement;
    u8 CreateUtcOffset;
    u8 LastModifiedUtcOffset;
    u8 LastAccessedUtcOffset;
    u8 Reserved2[7];
} __attribute__((packed)) ExFATFileDirectoryEntry;

typedef struct {
    u8 EntryType;
    u8 GeneralSecondaryFlags;
    u8 Reserved1;
    u8 NameLength;
    u16 NameHash;
    u16 Reserved2;
    u64 ValidDataLength;
    u32 Reserved3;
    u32 FirstCluster;
    u64 DataLength;
} __attribute__((packed)) ExFATStreamExtensionDirectoryEntry;

typedef struct {
    u8 EntryType;
    u8 GeneralSecondaryFlags;
    u16 FileName[15];
} __attribute__((packed)) ExFATFileNameDirectoryEntry;

u64 GetExFATRootDirectoryOffset(u8* Volume);
u64 GetExFATClusterOffset(u8* Volume, u32 Cluster);
u64 GetExFATClusterSize(u8* Volume);
u32 GetExFATClusterCount(u8* Volume);
u32 GetExFATFatCount(u8* Volume);
u32 GetExFATAllocationBitmapCluster(u8* Volume);

char* GetExFATFileName(ExFATFileDirectoryEntry* File);
u8* GetExFATFileBuffer(ExFATFileDirectoryEntry* File);
u64 GetExFATFileSize(ExFATFileDirectoryEntry* File);
u32 GetExFATFileFirstCluster(ExFATFileDirectoryEntry* File);
u16 GetExFATFileAttributes(ExFATFileDirectoryEntry* File);

ExFATFileDirectoryEntry* FindExFATFile(u8* Volume, u32 DirCluster, const u16* Name, u8 NameLength);
ExFATFileDirectoryEntry* FindExFATFolder(u8* Volume, u32 DirCluster, const u16* Name, u8 NameLength);

int NewExFATFile(u8* Volume, u32 DirCluster, const u16* Name, u8 NameLength, u16 Attributes);
int NewExFATFolder(u8* Volume, u32 DirCluster, const u16* Name, u8 NameLength);

u8* ReadExFATFile(u8* Volume, const u16* Name, u8 NameLength);
u64 WriteExFATFile(u8* Volume, const u16* Name, u8 NameLength, const u8* Buffer, u64 Offset, u64 Length);

int DelExFATFile(u8* Volume, u32 DirCluster, const u16* Name, u8 NameLength);
int DelExFATFolder(u8* Volume, u32 DirCluster, const u16* Name, u8 NameLength);

void InitDiskToExFAT(u8* Volume);
int FormatExFAT(u8* Volume, u64 VolumeLength, u8 BytesPerSectorShift, u8 SectorsPerClusterShift);
int MountExFAT(u8* Volume);
int UnmountExFAT(u8* Volume);

u32 ExFATBootChecksum(u8* Sectors, u16 BytesPerSector);
u16 ExFATEntrySetChecksum(u8* Entries, u8 SecondaryCount);
u32 ExFATNameHash(const u16* FileName, u8 NameLength);
u16 ExFATUpcaseTableChecksum(const u8* UpcaseTable, u32 DataLength);

u32 ExFATAllocCluster(u8* Volume);
int ExFATFreeCluster(u8* Volume, u32 Cluster);
int ExFATIsClusterFree(u8* Volume, u32 Cluster);
u32 ExFATGetNextCluster(u8* Volume, u32 Cluster, int NoFatChain);
int ExFATSetNextCluster(u8* Volume, u32 Cluster, u32 Next);

u32 ExFATGetRootDirectoryCluster(u8* Volume);
u32 ExFATGetBitmapCluster(u8* Volume);
u32 ExFATGetUpcaseCluster(u8* Volume);