#include "CpuInfo.h"
#include "RandomSomething.h"
#include "ahci.h"
#include "efi.h"
#include "int.h"
#include "ioapic.h"
#include "pci.h"
#include "flash.h"
#include "gop.h"
#include "kmalloc.h"
#include "str.h"
#include "debug.h"
#include "gdt.h"
#include "idt.h"
#include "Context.h"
#include "task.h"
#include "mouse.h"
#include "keyboard.h"
#include "clock.h"
#include "Audio.h"
#include "AC97.h"
#include "port.h"
#include "ps2Mouse.h"
#include "ps2Keyboard.h"
#include "wm.h"
#include "shell.h"
#include "acpi.h"
#include "event.h"
#include "Notepad.h"
#include "ap.h"
#include "xhci.h"

#define KERNEL_STACK_SIZE ((1<<20)*288) 

extern const unsigned char _binary_unifont_bin_start[];
__attribute__((section(".bss"))) __attribute__((aligned(64))) u16 KernelFonts[65536][19];
u8 *KernelPoolHeadList=NULL;
u8* KernelStack=NULL;

#define PAGE_PRESENT  (1ULL << 0)
#define PAGE_WRITABLE (1ULL << 1)
#define PAGE_LARGE    (1ULL << 7)
#define PAGE_HUGE     (1ULL << 7)

u32* GopOut=NULL;
u32* GopBack=NULL;
u8 SystemMode=0;

EFI_BOOT_SERVICES *bs=NULL;
EFI_GOP_MODE GopMode={0};
EFI_GOP_MODE_INFO GopInfo={0};
EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop=NULL;
EFI_LOADED_IMAGE_PROTOCOL LoadedImage={0};

EFI_GUID GopGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
RegContext MainGlobalContext={0};

#define PAGE_SIZE_4KB  0x1000ULL
#define PAGE_SIZE_2MB  0x200000ULL
#define PAGE_SIZE_1GB  0x40000000ULL
#define PAGE_SIZE_512GB 0x8000000000ULL
#define PAGE_SIZE_256TB 0xE80000000000ULL

EFI_HANDLE ImageBase;
u64 FADTAddr=0;
u64 DSDTAddr=0;
u64 HPETAddr=0;
u64 MADTAddr=0;
u64 DMARAddr=0;
u64 AcpiSize=0;
u64 APCount=0;
u32 TmpCoreWakeID=0;
u64 TmpGlobalRSI=0;
u64 APBootloaderAddr=0x100000;

AllocPool KernelPool = { .Head=NULL};
WM KernelGlobalWindowManager={0};

#pragma clang optimize off
__attribute__((target("no-avx,no-avx2")))
void SIMDInit(void) {
    unsigned long long cr4;
    __asm__ volatile ("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= (1 << 9) | (1 << 10) | (1 << 18);
    __asm__ volatile ("mov %0, %%cr4" : : "r"(cr4));
    MemFullFlash();
    unsigned long long xcr0 = 0;
    __asm__ volatile ("xgetbv" : "=A"(xcr0) : "c"(0));
    MemFullFlash();
    xcr0 |= (1 << 1) | (1 << 2);
    __asm__ volatile ("xsetbv" : : "c"(0), "A"(xcr0));
    MemFullFlash();
}
void FPUInit(void){
    u64 cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 &= ~((u64)0xE);
    cr0 |= 0x22;
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0));
}
#pragma clang optimize on

Bool IsLa57Supported() {
    unsigned int eax, ebx, ecx, edx;
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
    return (ecx & (1 << 16)) != 0;
}

u64 Cr3=0;

u64* BuildDynamicPageTable(
    u64 MaxPhysMem, u64 PhysMem, 
    u64 CodeStart, u64 CodeEnd, 
    u64 *RsvdStartAddrList, u64 *RsvdEndAddrList, u64 RsvdAddrListCount
) {
    DebugStr("PhysMem:");DebugU64(PhysMem);DebugChar('\n');
    DebugStr("MaxPhysMem");DebugU64(MaxPhysMem);DebugChar('\n');
    EFI_PHYSICAL_ADDRESS alloc_addr;
    int Use_l5 = IsLa57Supported();

    wrmsr(0x277, 0x0000000000010006ULL);

    alloc_addr = 0xFFFFFFFFFFFFFFFF;
    if (bs->AllocatePages(AllocateMaxAddress, EfiRuntimeServicesData, 1, &alloc_addr) != EFI_SUCCESS) return 0;
    u64* root_table = (u64*)alloc_addr;
    MemSet(root_table, 0, 4096);

    u64* pml4 = NULL;
    if (Use_l5) {
        alloc_addr = 0xFFFFFFFFFFFFFFFF;
        if (bs->AllocatePages(AllocateMaxAddress, EfiRuntimeServicesData, 1, &alloc_addr) != EFI_SUCCESS) return 0;
        pml4 = (u64*)alloc_addr;
        MemSet(pml4, 0, 4096);
        root_table[0] = ((u64)pml4 & ~0xFFFULL) | 0x003;
    } else {
        pml4 = root_table;
    }

    if (pml4[0] == 0) {
        alloc_addr = 0xFFFFFFFFFFFFFFFF;
        if (bs->AllocatePages(AllocateMaxAddress, EfiRuntimeServicesData, 1, &alloc_addr) != EFI_SUCCESS) return 0;
        u64* pdpt = (u64*)alloc_addr;
        MemSet(pdpt, 0, 4096);
        pml4[0] = ((u64)pdpt & ~0xFFFULL) | 0x003;
    }
    u64* pdpt_low = (u64*)(pml4[0] & ~0xFFFULL);

    alloc_addr = 0xFFFFFFFFFFFFFFFF;
    if (bs->AllocatePages(AllocateMaxAddress, EfiRuntimeServicesData, 1, &alloc_addr) != EFI_SUCCESS) return 0;
    u64* pd_low = (u64*)alloc_addr;
    MemSet(pd_low, 0, 4096);
    pdpt_low[0] = ((u64)pd_low & ~0xFFFULL) | 0x003;

    alloc_addr = 0xFFFFFFFFFFFFFFFF;
    if (bs->AllocatePages(AllocateMaxAddress, EfiRuntimeServicesData, 1, &alloc_addr) != EFI_SUCCESS) return 0;
    u64* pt_4k = (u64*)alloc_addr;
    MemSet(pt_4k, 0, 4096);
    for (u64 i = 0; i < 512; i++) {
        pt_4k[i] = (i * 0x1000) | 0x003;
    }
    pt_4k[0]=0x000;
    pd_low[0] = ((u64)pt_4k & ~0xFFFULL) | 0x003;

    for (u64 i = 1; i < 512; i++) {
        u64 phys = i * 0x200000;
        pd_low[i] = phys | 0x183;
    }

    u64 max_1gb_pages = Use_l5 ? 262144 : 512;

    u64 code_start_1gb = (CodeStart / 0x40000000) * 0x40000000;
    u64 code_end_1gb = ((CodeEnd + 0x3FFFFFFF) / 0x40000000) * 0x40000000;

    for (u64 i = 1; i < max_1gb_pages; i++) {
        u64 phys = i * 0x40000000ULL;
        u64 pml5_idx = phys / (1ULL << 48);
        u64 pml4_idx = (phys % (1ULL << 48)) / (1ULL << 39);
        u64 pdpt_idx = (phys % (1ULL << 39)) / 0x40000000ULL;

        if (!Use_l5 && phys >= (1ULL << 48)) break;
        if (pml5_idx >= 512) break;

        u64* active_pml4 = pml4;
        if (Use_l5) {
            if (root_table[pml5_idx] == 0) {
                alloc_addr = 0xFFFFFFFFFFFFFFFF;
                if (bs->AllocatePages(AllocateMaxAddress, EfiRuntimeServicesData, 1, &alloc_addr) != EFI_SUCCESS) return 0;
                u64* new_pml4 = (u64*)alloc_addr;
                MemSet(new_pml4, 0, 4096);
                root_table[pml5_idx] = ((u64)new_pml4 & ~0xFFFULL) | 0x003;
            }
            active_pml4 = (u64*)(root_table[pml5_idx] & ~0xFFFULL);
        }

        if (active_pml4[pml4_idx] == 0) {
            alloc_addr = 0xFFFFFFFFFFFFFFFF;
            if (bs->AllocatePages(AllocateMaxAddress, EfiRuntimeServicesData, 1, &alloc_addr) != EFI_SUCCESS) return 0;
            u64* new_pdpt = (u64*)alloc_addr;
            MemSet(new_pdpt, 0, 4096);
            active_pml4[pml4_idx] = ((u64)new_pdpt & ~0xFFFULL) | 0x003;
        }
        u64* cur_pdpt = (u64*)(active_pml4[pml4_idx] & ~0xFFFULL);

        Bool IsReserved = 0;
        for (u64 r = 0; r < RsvdAddrListCount; r++) {
            u64 RsvdAlignedStartAddr = (RsvdStartAddrList[r] / 0x40000000) * 0x40000000;
            u64 RsvdAlignedEndAddr   = ((RsvdEndAddrList[r] + 0x3FFFFFFF) / 0x40000000) * 0x40000000;
            if (phys >= RsvdAlignedStartAddr && phys < RsvdAlignedEndAddr) {
                IsReserved = 1;
                break;
            }
        }

        if (phys >= code_start_1gb && phys < code_end_1gb) {
            cur_pdpt[pdpt_idx] = phys | 0x183;
        } else if (phys >= 0xFC000000 && phys <= 0x100000000){
            cur_pdpt[pdpt_idx] = phys | 0x193;
        } else if (
            (
                phys >= (((u64)GopBack) & ~((PAGE_SIZE_1GB) - 1)) && 
                phys < ((((u64)GopBack+GopMode.FrameBufferSize) + (PAGE_SIZE_1GB) - 1) & ~((PAGE_SIZE_1GB) - 1))
            ) ||
            (
                phys >= (((u64)GopOut) & ~((PAGE_SIZE_1GB) - 1)) && 
                phys < ((((u64)GopOut+GopMode.FrameBufferSize) + (PAGE_SIZE_1GB) - 1) & ~((PAGE_SIZE_1GB) - 1))
            )
        ){
            cur_pdpt[pdpt_idx] = phys | 0x18B;
        } else if (IsReserved){
            cur_pdpt[pdpt_idx] = phys | 0x180;
        } else if (phys >= (PhysMem+0xE0000)){
            cur_pdpt[pdpt_idx] = phys | 0x193;
        } else {
            cur_pdpt[pdpt_idx] = phys | 0x183;
        }
    }
    Cr3=(u64)root_table;
    return root_table;
}

#pragma clang optimize off
__attribute__((target("no-avx,no-avx2,no-sse,no-sse3")))
EFI_STATUS ExitBootServices_Safe(EFI_HANDLE ImageHandle) {
    EFI_STATUS status;
    UINTN MapKey;
    UINTN MemoryMapSize = 0;
    EFI_MEMORY_DESCRIPTOR *MemoryMap = NULL;
    UINTN DescriptorSize;
    u32 DescriptorVersion;
    
    status = bs->GetMemoryMap(&MemoryMapSize, NULL, &MapKey, &DescriptorSize, &DescriptorVersion);
    if (status != EFI_BUFFER_TOO_SMALL) return status;
    
    MemoryMapSize += 4096;
    status = bs->AllocatePool(EfiRuntimeServicesData, MemoryMapSize, (void**)&MemoryMap);
    if (EFI_ERROR(status)) return status;
    
    status = bs->GetMemoryMap(&MemoryMapSize, MemoryMap, &MapKey, &DescriptorSize, &DescriptorVersion);
    if (EFI_ERROR(status)) {
        bs->FreePool(MemoryMap);
        return status;
    }
    
    while (1) {
        status = bs->ExitBootServices(ImageHandle, MapKey);
        if (status == EFI_SUCCESS) break;
        
        MemoryMapSize = 0;
        status = bs->GetMemoryMap(&MemoryMapSize, NULL, &MapKey, &DescriptorSize, &DescriptorVersion);
        if (status != EFI_BUFFER_TOO_SMALL) {
            bs->FreePool(MemoryMap);
            return status;
        }
        MemoryMapSize += 4096;
        bs->FreePool(MemoryMap);
        status = bs->AllocatePool(EfiLoaderData, MemoryMapSize, (void**)&MemoryMap);
        if (EFI_ERROR(status)) return status;
        
        status = bs->GetMemoryMap(&MemoryMapSize, MemoryMap, &MapKey, &DescriptorSize, &DescriptorVersion);
        if (EFI_ERROR(status)) {
            bs->FreePool(MemoryMap);
            return status;
        }
    }
    
    return EFI_SUCCESS;
}
#pragma clang optimize on

void InitPool(AllocPool* Pool, u64 PoolSize, void* Start){
    if (!Pool || !Start) return;
    AllocBlock *block = (AllocBlock*)Start;
    block->Size = PoolSize - sizeof(AllocBlock);
    block->is_free = 1;
    block->next = NULL;
    block->prev = NULL;
    Pool->Head = block;
}

void InitBlock(u64 PoolSize,u64 Pos){
    AllocBlock *block = (AllocBlock*)Pos;
    block->Size=PoolSize - sizeof(AllocBlock);
    block->is_free=1;
    block->next = NULL;
    PoolAddBlock(&KernelPool,block);
}

static u64 temporary_gdt[] = {
    0x0000000000000000,
    0x00209a0000000000,
    0x00cf9a000000ffff,
    0x00cf92000000ffff
};

struct {
    u16 limit;
    u64 base;
} __attribute__((packed)) gdtr = {
    .limit = sizeof(temporary_gdt) - 1,
    .base = (u64)temporary_gdt
};

#pragma clang optimize off
void LoadDynamicPageTable(u64* root_table) {
    DebugStr("LoadDynamicPageTable\n");
    DebugStr("root_table=0x");DebugU64((u64)root_table);DebugChar('\n');
    u64 pure_cr3 = ((u64)root_table) & ~0xFFFULL;

    if (!IsLa57Supported()) {
        DebugStr("No La57\n");
        DebugStr("pure_cr3=0x");DebugU64(pure_cr3);DebugChar('\n');
        DebugStr("pure_cr3 At 0x");DebugU64((u64)&pure_cr3);DebugChar('\n');
        __asm__ volatile(
            "movq %0, %%rax\n\t"
            "movq %%rax, %%cr3\n\t"
            :: "r"(pure_cr3) : "memory"
        );
        DebugStr("Finish\n");
        return;
    }

    DebugStr("Have La57\n");

    __asm__ volatile(
        "movq %0, %%rbx\n\t"
        "lgdt %1\n\t"
        "leaq 1f(%%rip), %%rax\n\t"
        "pushq $0x10\n\t"
        "pushq %%rax\n\t"
        "lretq\n\t"

        ".code32\n\t"
        "1:\n\t"
        "movl $0x18, %%eax\n\t"
        "movl %%eax, %%ds\n\t"
        "movl %%eax, %%ss\n\t"

        "movl %%cr0, %%eax\n\t"
        "btrl $31, %%eax\n\t"
        "movl %%eax, %%cr0\n\t"

        "movl %%cr4, %%eax\n\t"
        "btsl $12, %%eax\n\t"
        "movl %%eax, %%cr4\n\t"

        "movl %%ebx, %%cr3\n\t"

        "movl %%cr0, %%eax\n\t"
        "btsl $31, %%eax\n\t"
        "movl %%eax, %%cr0\n\t"

        "pushl $0x08\n\t"
        "pushl $2f\n\t"
        "lretl\n\t"

        ".code64\n\t"
        "2:\n\t"
        "nop\n\t"
        :
        : "r"(pure_cr3), "m"(gdtr)
        : "rax", "rbx", "memory"
    );
    DebugStr("Load Finish\n");
}

u64 GetTotalAvailableMemory(EFI_MEMORY_DESCRIPTOR* MemMap, UINTN EntryCount, UINTN DescriptorSize) {
    u64 Total = 0;
    
    for (UINTN i = 0; i < EntryCount; i++) {
        EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)((u8*)MemMap + (i * DescriptorSize));
        
        if (desc->Type == EfiConventionalMemory ||
            desc->Type == EfiBootServicesData ||
            desc->Type == EfiBootServicesCode ||
            desc->Type == EfiRuntimeServicesData ||
            desc->Type == EfiRuntimeServicesCode ||
            desc->Type == EfiLoaderCode ||
            desc->Type == EfiLoaderData ||
            desc->Type == EfiAcpiReclaimMemory ||
            desc->Type == EfiAcpiMemoryNVS ||
            desc->Type == EfiPalCode) {
            
            Total += desc->NumberOfPages * 4096;
        }
    }
    EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)((u8*)MemMap + (0 * DescriptorSize));
    Total += desc->PhysicalStart;
    
    return Total;
}
#pragma clang optimize on

void PrintU64ToConOut(EFI_SYSTEM_TABLE* sys, u64 num) {
    if (num == 0) {
        sys->ConOut->OutputString(sys->ConOut, L"0");
        return;
    }
    
    u16 buffer[32];
    int idx = 0;
    
    while (num > 0) {
        buffer[idx++] = L'0' + (num % 10);
        num /= 10;
    }
    
    for (int i = 0; i < idx / 2; i++) {
        u16 tmp = buffer[i];
        buffer[i] = buffer[idx - 1 - i];
        buffer[idx - 1 - i] = tmp;
    }
    
    buffer[idx] = L'\0';
    sys->ConOut->OutputString(sys->ConOut, buffer);
}

typedef struct {
    u32 Width;
    u32 Height;
} RESOLUTION_ENTRY;

RESOLUTION_ENTRY Resolutions[32];
u32 ResolutionCount = 0;

void CollectResolutions(u32* Count) {
    *Count = 0;
    for (u32 i = 0; i < Gop->Mode->MaxMode && *Count < 32; i++) {
        EFI_GOP_MODE_INFO *Info = NULL;
        u64 SizeOfInfo = 0;
        
        if (!EFI_ERROR(Gop->QueryMode(Gop, i, &SizeOfInfo, &Info))) {
            if (Info->HorizontalResolution < 800 || 
                Info->VerticalResolution < 600) {
                continue;
            }

            Bool Duplicate = 0;
            for (u32 j = 0; j < *Count; j++) {
                if (Resolutions[j].Width == (u32)Info->HorizontalResolution &&
                    Resolutions[j].Height == (u32)Info->VerticalResolution) {
                    Duplicate = 1;
                    break;
                }
            }
            if (!Duplicate) {
                Resolutions[*Count].Width = (u32)Info->HorizontalResolution;
                Resolutions[*Count].Height = (u32)Info->VerticalResolution;
                (*Count)++;
            }
        }
    }
    
    if (*Count == 0) {
        DebugStr("FATAL: No resolution >= 800x600 found!\n");
        while(1) __asm__("hlt");
    }
}

u32 SelectResolution(EFI_SYSTEM_TABLE* sys) {
    u32 Count = 0;
    CollectResolutions(&Count);
    
    if (Count == 0) {
        return 0;
    }
    
    Start:
    sys->ConOut->EnableCursor(sys->ConOut, 0);
    sys->ConOut->ClearScreen(sys->ConOut);
    sys->ConOut->SetCursorPosition(sys->ConOut, 0, 0);
    sys->ConOut->OutputString(sys->ConOut, L"Available Resolutions:\n\r\n\r");
    
    for (u32 i = 0; i < Count; i++) {
        sys->ConOut->OutputString(sys->ConOut, L"[");
        PrintU64ToConOut(sys, i);
        sys->ConOut->OutputString(sys->ConOut, L"] ");
        PrintU64ToConOut(sys, Resolutions[i].Width);
        sys->ConOut->OutputString(sys->ConOut, L" x ");
        PrintU64ToConOut(sys, Resolutions[i].Height);
        sys->ConOut->OutputString(sys->ConOut, L"\n\r");
    }
    sys->ConOut->EnableCursor(sys->ConOut, 1);
    sys->ConOut->OutputString(sys->ConOut, L"\n\rEnter resolution number: ");
    
    u16 InputBuffer[16];
    u32 InputLength = 0;
    EFI_INPUT_KEY Key;

    while (1) {
        u64 EventIndex;
        sys->BootServices->WaitForEvent(1, &sys->ConIn->WaitForKey, &EventIndex);
        sys->ConIn->ReadKeyStroke(sys->ConIn, &Key);
        
        if (Key.UnicodeChar >= L'0' && Key.UnicodeChar <= L'9') {
            if (InputLength < 15) {
                InputBuffer[InputLength++] = Key.UnicodeChar;
                InputBuffer[InputLength] = L'\0';
                u16 Temp[2] = {Key.UnicodeChar, L'\0'};
                sys->ConOut->OutputString(sys->ConOut, Temp);
            }
        }
        else if (Key.UnicodeChar == L'\b') {
            if (InputLength > 0) {
                InputLength--;
                InputBuffer[InputLength] = L'\0';
                sys->ConOut->OutputString(sys->ConOut, L"\b \b");
            }
        }
        else if (Key.UnicodeChar == L'\r' || Key.UnicodeChar == L'\n') {
            sys->ConOut->OutputString(sys->ConOut, L"\n\r");
            break;
        }
        else if (Key.UnicodeChar == L'\x1b') {
            sys->ConOut->OutputString(sys->ConOut, L"\n\rCancel, using default (0)\n\r");
            return 0;
        }
    }
    
    if (InputLength == 0) {
        goto Start;
    }
    
    u32 Selection = 0;
    for (u32 i = 0; i < InputLength; i++) {
        Selection = Selection * 10 + (InputBuffer[i] - L'0');
    }
    
    if (Selection >= Count) {
        sys->ConOut->OutputString(sys->ConOut, L"Invalid selection, using default (0)\n\r");
        return 0;
    }

    u32 GopModeIndex = 0;
    for (u32 i = 0; i < Gop->Mode->MaxMode; i++) {
        EFI_GOP_MODE_INFO *Info = NULL;
        u64 SizeOfInfo = 0;
        if (!EFI_ERROR(Gop->QueryMode(Gop, i, &SizeOfInfo, &Info))) {
            if ((u32)Info->HorizontalResolution == Resolutions[Selection].Width &&
                (u32)Info->VerticalResolution == Resolutions[Selection].Height) {
                GopModeIndex = i;
                break;
            }
        }
    }
    
    return GopModeIndex;
}

#pragma clang optimize off
__attribute__((target("no-avx,no-avx2,no-sse3,no-sse2,no-sse")))
EFI_STATUS EFIAPI Cefi_main(EFI_HANDLE image, EFI_SYSTEM_TABLE *sys) {
    DebugInit();
    CpuInfoResult CpuID0;
    __asm__ volatile (
        "pushq %%rbx\n\t"
        "cpuid\n\t"
        "movl %%ebx, %1\n\t"
        "popq %%rbx\n\t"
        : "=a" (CpuID0.Eax),
          "=b" (CpuID0.Ebx),
          "=c" (CpuID0.Ecx),
          "=d" (CpuID0.Edx)
        : "a" (0), 
          "c" (0)

    );
    CpuInfoResult CpuID1;
    __asm__ volatile (
        "pushq %%rbx\n\t"
        "cpuid\n\t"
        "movl %%ebx, %1\n\t"
        "popq %%rbx\n\t"
        : "=a" (CpuID1.Eax),
          "=b" (CpuID1.Ebx),
          "=c" (CpuID1.Ecx),
          "=d" (CpuID1.Edx)
        : "a" (1), 
          "c" (0)
    );
    if (CpuID0.Eax >= 0x07) {
        CpuInfoResult CpuID2;
        __asm__ volatile (
            "pushq %%rbx\n\t"
            "cpuid\n\t"
            "movl %%ebx, %1\n\t"
            "popq %%rbx\n\t"
            : "=a" (CpuID2.Eax),
            "=b" (CpuID2.Ebx),
            "=c" (CpuID2.Ecx),
            "=d" (CpuID2.Edx)
            : "a" (7), 
            "c" (0)
        );
        DebugStr("Have AVX2\n");
        SIMDInit();
    } else {
        DebugStr("No AVX2\n");
        sys->ConOut->EnableCursor(sys->ConOut, 0);
        sys->ConOut->SetAttribute(sys->ConOut, EFI_WHITE);
        sys->ConOut->ClearScreen(sys->ConOut);
        sys->ConOut->SetCursorPosition(sys->ConOut, 0, 0);
        sys->ConOut->OutputString(sys->ConOut, L"Your CPU havent AVX2/AVX, Please replace it with a CPU that supports AVX2.\n\r");
        sys->ConOut->OutputString(sys->ConOut, L"Press A: Return To UEFI Boot Menu\r\n");
        sys->ConOut->OutputString(sys->ConOut, L"Press B: Try Next Boot Options\r\n");
        sys->ConOut->OutputString(sys->ConOut, L"Press Any Other Key: Shutdown\r\n");
        EFI_INPUT_KEY Key;
        u64 EventIndex;
        sys->BootServices->WaitForEvent(1, &sys->ConIn->WaitForKey, &EventIndex);
        sys->ConIn->ReadKeyStroke(sys->ConIn, &Key);
        
        if (Key.UnicodeChar==L'A' || Key.UnicodeChar==L'a') sys->RuntimeServices->ResetSystem(EfiResetCold, EFI_SUCCESS, 0, NULL);
        else if (Key.UnicodeChar==L'B' || Key.UnicodeChar==L'b') sys->BootServices->Exit(image, EFI_ABORTED, 0, NULL);
        else sys->RuntimeServices->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, NULL);
    }
    bs=sys->BootServices;

    FPUInit();
    CalibrateTscUsingStall();
    InitClock();
#pragma clang optimize on
    
    u64 CodeStart;
    u64 CodeEnd;

    EFI_LOADED_IMAGE_PROTOCOL *Image = NULL;

    EFI_STATUS status = sys->BootServices->HandleProtocol(
        image,
        (EFI_GUID *)&gEfiLoadedImageProtocolGuid,
        (void **)&Image
    );

    MemCopy(&LoadedImage, Image, sizeof(EFI_LOADED_IMAGE_PROTOCOL));

    if (status == EFI_SUCCESS && Image != NULL) {
        u64 TrueImageBase = (u64)Image->ImageBase;
        u64 TrueImageSize = Image->ImageSize;

        DebugStr("\r\n[NeuroSamaOS] UEFI LOADED IMAGE BASE: ");
        DebugU64(TrueImageBase);
        DebugStr("\r\n[NeuroSamaOS] IMAGE SIZE: ");
        DebugU64(TrueImageSize);
        DebugChar(L'\n');
        CodeStart=TrueImageBase;
        CodeEnd=TrueImageBase+TrueImageSize;

    } else {
        DebugStr("\r\n[ERROR] HandleProtocol failed to fetch ImageBase! Code: ");
        DebugU64(status);
        DebugChar(L'\n');
    }

    u32 max_mode;
    u32 best_mode = 0;
    u32 best_columns = 0;
    u32 best_rows = 0;
    u8  found = 0;

    max_mode = sys->ConOut->Mode->MaxMode;

    for (u32 mode_number = 0; mode_number < max_mode; mode_number++) {
        u32 columns = 0;
        u32 rows = 0;

        status = sys->ConOut->QueryMode(sys->ConOut, mode_number, &columns, &rows);

        if (status == EFI_UNSUPPORTED)
            continue;

        if (status != EFI_SUCCESS)
            return status;

        if (!found || columns > best_columns) {
            best_columns = columns;
            best_rows = rows;
            best_mode = mode_number;
            found = 1;
        }
    }

    if (!found) sys->ConOut->SetMode(sys->ConOut, 0);
    else sys->ConOut->SetMode(sys->ConOut, best_mode);

    (void)best_rows;

    bs->LocateProtocol(&GopGuid, NULL, (void **)&Gop);

    u32 SelectedMode = SelectResolution(sys);

    EFI_STATUS Status = Gop->SetMode(Gop, SelectedMode);

    while (1){
        if (Gop->Mode->Mode != SelectedMode) {
            u32 CurrentMode = Gop->Mode->Mode;
            Status = Gop->SetMode(Gop, CurrentMode); 
            Status = Gop->SetMode(Gop, SelectedMode);
        }
        break;
    }
    DebugStr("SetMode Status: ");
    DebugU64(Status);
    DebugChar('\n');
    DebugStr("FrameBufferBase after SetMode: ");
    DebugU64(Gop->Mode->FrameBufferBase);
    DebugChar('\n');
    if (Status != EFI_SUCCESS)
    {
        DebugStr("\r\n[NeuroSamaOS] GOP Init Faild.\n");
        while(1) __asm__ volatile("cli\n\t hlt");
    }
    MemCopy(&GopMode,Gop->Mode,sizeof(EFI_GOP_MODE));
    MemCopy(&GopInfo,Gop->Mode->Info,sizeof(EFI_GOP_MODE_INFO));
    GopMode.FrameBufferSize=GopInfo.PixelsPerScanLine * GopInfo.VerticalResolution * sizeof(u32);
    GopOut=(u32*)(u64)GopMode.FrameBufferBase;
    DebugStr("Gop->Mode->Info->PixelFormat = ");
    DebugU32(Gop->Mode->Info->PixelFormat);
    DebugChar('\n');
    DebugStr("Gop->Mode->Info->HorizontalResolution = ");
    DebugU32(Gop->Mode->Info->HorizontalResolution);
    DebugChar('\n');
    DebugStr("Gop->Mode->Info->VerticalResolution = ");
    DebugU32(Gop->Mode->Info->VerticalResolution);
    DebugChar('\n');
    sys->ConOut->EnableCursor(sys->ConOut, 0);
    if (GopInfo.PixelFormat == PixelBltOnly){
        EFI_INPUT_KEY Key;
        sys->ConOut->ClearScreen(sys->ConOut);
        sys->ConOut->SetCursorPosition(sys->ConOut, 0, 10);
        
        sys->ConOut->OutputString(sys->ConOut, 
            L"You Fucking GPU Is Fucking PixelBltOnly Bro,\r\n");
        sys->ConOut->OutputString(sys->ConOut, 
            L"Dont Use This Fucking Shit GPU, Buy A New One Pls Bro\r\n");
        sys->ConOut->OutputString(sys->ConOut, 
            L"\r\nPress A: Return To UEFI Boot Menu\r\n");
        sys->ConOut->OutputString(sys->ConOut, 
            L"Press B: Try Next Boot Options\r\n");
        sys->ConOut->OutputString(sys->ConOut, 
            L"Press Any Other Key: Shutdown\r\n");

        u64 EventIndex;
        sys->BootServices->WaitForEvent(1, &sys->ConIn->WaitForKey, &EventIndex);
        sys->ConIn->ReadKeyStroke(sys->ConIn, &Key);
        
        if (Key.UnicodeChar==L'A' || Key.UnicodeChar==L'a') sys->RuntimeServices->ResetSystem(EfiResetCold, EFI_SUCCESS, 0, NULL);
        else if (Key.UnicodeChar==L'B' || Key.UnicodeChar==L'b') sys->BootServices->Exit(image, EFI_ABORTED, 0, NULL);
        else sys->RuntimeServices->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, NULL);
    }
    SystemMode=0;
    EFI_INPUT_KEY Key;
    Bool ModeSelected=0;
    sys->ConOut->EnableCursor(sys->ConOut, 0);
    sys->ConOut->SetCursorPosition(sys->ConOut, 0, 0);
    sys->ConOut->ClearScreen(sys->ConOut);
    while (!ModeSelected){
        sys->ConOut->EnableCursor(sys->ConOut, 0);
        sys->ConOut->SetAttribute(sys->ConOut, EFI_WHITE);
        sys->ConOut->ClearScreen(sys->ConOut);
        sys->ConOut->SetCursorPosition(sys->ConOut, 0, 0);
        
        sys->ConOut->OutputString(sys->ConOut, 
            L"Mode:\r\n");
        if (SystemMode == 0) {
            sys->ConOut->SetAttribute(sys->ConOut, EFI_HIGHLIGHT);
            sys->ConOut->OutputString(sys->ConOut, L"0 : Temple Mode\r\n");
            sys->ConOut->SetAttribute(sys->ConOut, EFI_WHITE);
        } else {
            sys->ConOut->OutputString(sys->ConOut, L"0 : Temple Mode\r\n");
        }
        
        if (SystemMode == 1) {
            sys->ConOut->SetAttribute(sys->ConOut, EFI_HIGHLIGHT);
            sys->ConOut->OutputString(sys->ConOut, L"1 : World Mode\r\n");
            sys->ConOut->SetAttribute(sys->ConOut, EFI_WHITE);
        } else {
            sys->ConOut->OutputString(sys->ConOut, L"1 : World Mode\r\n");
        }

        u64 EventIndex;
        sys->ConOut->EnableCursor(sys->ConOut, 0);
        sys->BootServices->WaitForEvent(1, &sys->ConIn->WaitForKey, &EventIndex);
        sys->ConIn->ReadKeyStroke(sys->ConIn, &Key);

        if (Status == EFI_SUCCESS) {
            if (Key.UnicodeChar == 0x0000) {
                if (Key.ScanCode==SCAN_DOWN || Key.ScanCode==SCAN_RIGHT) {
                    if (SystemMode < 1) {
                        SystemMode++;
                    }
                } else if (Key.ScanCode==SCAN_UP || Key.ScanCode==SCAN_LEFT) {
                    if (SystemMode > 0) {
                        SystemMode--;
                    }
                }
            }else if  (Key.UnicodeChar==L'\r'){
                ModeSelected=1;
            }
        }
        sys->ConOut->EnableCursor(sys->ConOut, 0);
    }
    sys->ConOut->ClearScreen(sys->ConOut);
    MemSet(KernelFonts, 0, sizeof(KernelFonts));
    for (u64 i = 0; i < 65536; i++) {
        if (i >= 0xD800 && i <= 0xDFFF) continue;
        if (i == 0x0000) continue;

        const unsigned char *g = _binary_unifont_bin_start + i * 32;

        for (u32 Row = 0; Row < 19; Row++)
            KernelFonts[i][Row] = 0;

        for (u32 Row = 0; Row < 16; Row++) {
            u16 bits = 0;
            for (u32 Col = 0; Col < 16; Col++) {
                u8 byte = g[Row * 2 + (Col >> 3)];
                if (byte & (0x80u >> (Col & 7)))
                    bits |= (u16)(0x8000u >> Col);
            }
            KernelFonts[i][Row + ((19 - 16) / 2)] = bits;
        }
    }

    GetAcpiAddrUEFI(bs, &AcpiSize, &FADTAddr, &DSDTAddr, &HPETAddr, &MADTAddr, &DMARAddr);
    SetHPETMMIO(HPETAddr);

    UINTN MemoryMapSize = 0;
    EFI_MEMORY_DESCRIPTOR *MemMap = NULL;
    UINTN MapKey, DescriptorSize;
    u32 DescriptorVersion;
    bs->GetMemoryMap(&MemoryMapSize, NULL, &MapKey, &DescriptorSize, &DescriptorVersion);
    MemoryMapSize += 2 * DescriptorSize;
    bs->AllocatePool(EfiLoaderData, MemoryMapSize, (void**)&MemMap);
    bs->GetMemoryMap(&MemoryMapSize, (void*)MemMap, &MapKey, &DescriptorSize, &DescriptorVersion);

    UINTN EntryCount = MemoryMapSize / DescriptorSize;
    u64 MaxPhysicalAddress = 0;
    u64 MaxAvailableAddress = 0;
    u64 LowAvailableAddress = 0;

    u64 RsvdStartAddrListAddr=0xFFFFFFFF;
    bs->AllocatePages(AllocateMaxAddress, EfiBootServicesData, EFI_SIZE_TO_PAGES(MemoryMapSize) , &RsvdStartAddrListAddr);
    u64* RsvdStartAddrList=(u64*)RsvdStartAddrListAddr;
    u64 RsvdEndAddrListAddr=0xFFFFFFFF;
    bs->AllocatePages(AllocateMaxAddress, EfiBootServicesData, EFI_SIZE_TO_PAGES(MemoryMapSize) , &RsvdEndAddrListAddr);
    u64* RsvdEndAddrList=(u64*)RsvdEndAddrListAddr;
    MemSet(RsvdStartAddrList,0,MemoryMapSize);
    MemSet(RsvdEndAddrList,0,MemoryMapSize);
    u64 RsvdCount=0;

    for (UINTN i = 0; i < EntryCount; i++) {
        EFI_MEMORY_DESCRIPTOR *desc = (EFI_MEMORY_DESCRIPTOR*)((u8*)MemMap + (i * DescriptorSize));
        u64 EndAddress = desc->PhysicalStart + (desc->NumberOfPages * 4096);

        if (EndAddress>0x9000 && desc->Type == EfiConventionalMemory && APBootloaderAddr==0x100000){
            bs->AllocatePages(AllocateMaxAddress, EfiRuntimeServicesCode, desc->NumberOfPages-1 ? desc->NumberOfPages-1 : desc->NumberOfPages, &APBootloaderAddr);
        }

        if (EndAddress > MaxPhysicalAddress) {
            MaxPhysicalAddress = EndAddress;
        }

        if (desc->Type == EfiConventionalMemory ||
            desc->Type == EfiBootServicesData ||
            desc->Type == EfiRuntimeServicesData ||
            desc->Type == EfiLoaderCode ||
            desc->Type == EfiLoaderData ||
            desc->Type == EfiBootServicesCode ||
            desc->Type == EfiAcpiReclaimMemory ||
            desc->Type == EfiAcpiMemoryNVS
        ) {
            if (EndAddress > MaxAvailableAddress) {
                MaxAvailableAddress = EndAddress;
            }
            if (desc->PhysicalStart < 0x100000000ULL && desc->PhysicalStart > 0x100000) {
                if (LowAvailableAddress == 0 || desc->PhysicalStart < LowAvailableAddress) {
                    LowAvailableAddress = desc->PhysicalStart;
                }
            }
        }

        if (desc->Type == EfiReservedMemoryType && (!(desc->PhysicalStart>=0xFC000000) && !(desc->PhysicalStart<0x100000000))){
            RsvdStartAddrList[RsvdCount]=desc->PhysicalStart;
            RsvdEndAddrList[RsvdCount]=EndAddress;
            RsvdCount++;
        }

        DebugStr("Memory Table[");DebugU64(i);DebugStr("]:");DebugStr(" Type: ");DebugU32(desc->Type);DebugChar(',');DebugStr(" PhysicalStart: ");DebugU64(desc->PhysicalStart);DebugStr(", VirtualStart: ");DebugU64(desc->VirtualStart);DebugStr(", NumberOfPages: ");DebugU64(desc->NumberOfPages);DebugStr(", Attribute: ");DebugU64(desc->Attribute);DebugChar('\n');
    }
    u64 RealMaxAddress = 0;
    u64 MaxMMIO = 0;

    for (UINTN i = 0; i < EntryCount; i++) {
        EFI_MEMORY_DESCRIPTOR *desc = (EFI_MEMORY_DESCRIPTOR*)((u8*)MemMap + (i * DescriptorSize));
        u64 EndAddress = desc->PhysicalStart + (desc->NumberOfPages * 4096);

        if (EndAddress > RealMaxAddress) {
            RealMaxAddress = EndAddress;
        }

        if (desc->Type == 11) {
            if (EndAddress > MaxMMIO) {
                MaxMMIO = EndAddress;
            }
        }
    }
    u64 TotalAvailableMemory = GetTotalAvailableMemory(MemMap, EntryCount, DescriptorSize);

    DebugStr("Total Available Memory: ");
    DebugU64(TotalAvailableMemory);
    DebugStr(" Bytes\n");

    u64 Error = (64ULL<<20);
    u64 MinimumB = (2ULL<<30) - Error;

    if (TotalAvailableMemory < MinimumB) {
        sys->ConOut->ClearScreen(sys->ConOut);
        sys->ConOut->SetCursorPosition(sys->ConOut, 0, 0);
        sys->ConOut->SetAttribute(sys->ConOut, EFI_RED);
        sys->ConOut->OutputString(sys->ConOut, L"========================================\r\n");
        sys->ConOut->OutputString(sys->ConOut, L"  Neuro-Sama OS - Memory Error\r\n");
        sys->ConOut->OutputString(sys->ConOut, L"========================================\r\n\r\n");
        sys->ConOut->SetAttribute(sys->ConOut, EFI_WHITE);
        sys->ConOut->OutputString(sys->ConOut, L"  Minimum memory required: 2080374784 Bytes\r\n");
        sys->ConOut->OutputString(sys->ConOut, L"  Detected memory:         ");
        
        u16 buf[32];
        int idx = 0;
        u64 num = TotalAvailableMemory;
        while (num > 0) {
            buf[idx++] = L'0' + (num % 10);
            num /= 10;
        }
        if (idx == 0) buf[idx++] = L'0';
        for (int j = 0; j < idx / 2; j++) {
            u16 tmp = buf[j];
            buf[j] = buf[idx - 1 - j];
            buf[idx - 1 - j] = tmp;
        }
        buf[idx] = L'\0';
        sys->ConOut->OutputString(sys->ConOut, buf);
        sys->ConOut->OutputString(sys->ConOut, L" Bytes\r\n\r\n");
        
        sys->ConOut->SetAttribute(sys->ConOut, EFI_RED);
        sys->ConOut->OutputString(sys->ConOut, L"  This system does not meet the\r\n");
        sys->ConOut->OutputString(sys->ConOut, L"  minimum hardware requirements.\r\n");
        sys->ConOut->OutputString(sys->ConOut, L"  If Keep Run In Less Then 2GB, The System Will Crash.(IDK Why Btw ¯\\_(ツ)_/¯)\r\n\r\n");
        sys->ConOut->SetAttribute(sys->ConOut, EFI_WHITE);
        sys->ConOut->OutputString(sys->ConOut, L"  Press any key to shutdown...\r\n");
        
        UINTN EventIndex;
        sys->BootServices->WaitForEvent(1, &sys->ConIn->WaitForKey, &EventIndex);
        sys->ConIn->ReadKeyStroke(sys->ConIn, &Key);
        
        sys->RuntimeServices->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, NULL);
        
        while(1) __asm__ volatile("cli\n\t hlt");
    }

    u64 MaxAddress = RealMaxAddress;
    if (MaxMMIO > MaxAddress) {
        MaxAddress = MaxMMIO;
    }
    if (0xFFFFFFFFFFFF > MaxAddress) {
        MaxAddress = 0xFFFFFFFFFFFF;
    }
    
    DebugStr("\nMaxPhysicalAddress: ");DebugU64(TotalAvailableMemory);DebugChar('\n');
    DebugStr("MaxAddress: ");DebugU64(MaxAddress);DebugChar('\n');
    DebugStr("BuildDynamicPageTable.\n");
    u64 *pml5 = BuildDynamicPageTable(MaxAddress, TotalAvailableMemory, CodeStart, CodeEnd, RsvdStartAddrList, RsvdEndAddrList, RsvdCount);
    if (!pml5) {
        DebugStr("BuildDynamicPageTable failed!\n");
        while(1) __asm__ volatile("cli\n\t hlt");
    }
    
    DebugStr("InitPool.\n");
    
    u64 KernelPoolAddr=0xFFFFFFFFULL;
    Status=bs->AllocatePages(AllocateMaxAddress, EfiRuntimeServicesData, EFI_SIZE_TO_PAGES((RealMaxAddress>>2)+sizeof(AllocBlock)), &KernelPoolAddr);
    if (EFI_ERROR(Status)) {
        KernelPoolAddr = MaxAddress;
        bs->AllocatePages(AllocateMaxAddress, EfiRuntimeServicesData, EFI_SIZE_TO_PAGES((RealMaxAddress>>2)+sizeof(AllocBlock)), &KernelPoolAddr);
    }
    KernelPoolHeadList=(u8*)KernelPoolAddr;
    MemSet(KernelPoolHeadList,0,(RealMaxAddress>>2)+sizeof(AllocBlock));

    u64 KernelStackAddr=0xFFFFFFFFULL;
    Status=bs->AllocatePages(AllocateMaxAddress, EfiRuntimeServicesData, EFI_SIZE_TO_PAGES(KERNEL_STACK_SIZE), &KernelStackAddr);
    if (EFI_ERROR(Status)) {
        KernelStackAddr = MaxAddress;
        bs->AllocatePages(AllocateMaxAddress, EfiRuntimeServicesData, EFI_SIZE_TO_PAGES(KERNEL_STACK_SIZE), &KernelStackAddr);
    }
    KernelStack=(u8*)KernelStackAddr;
    MemSet(KernelStack,0,KERNEL_STACK_SIZE);
    
    MemSet(GopOut,0xFF,GopMode.FrameBufferSize);
    __asm__("sfence");

    DebugStr("allocated at 0x");
    DebugU64((u64)KernelPoolHeadList);
    DebugStr("\n");
    DebugStr("KERNEL_POOL_PAGES = ");
    DebugU64(EFI_SIZE_TO_PAGES((RealMaxAddress>>2)+sizeof(AllocBlock)));
    DebugChar('\n');
    InitPool(&KernelPool, (RealMaxAddress>>2)+sizeof(AllocBlock), KernelPoolHeadList);
    DebugStr("KernelPool initialized at 0x");
    DebugU64((u64)KernelPool.Head);
    DebugStr("\n");
    DebugStr("KernelStack at: "); DebugU64((u64)KernelStack); DebugChar('\n');

    MemSet(GopOut,0xFF,GopMode.FrameBufferSize);
    __asm__("sfence");

    DebugStr("ExitBootServices.\n");
    ExitBootServices_Safe(image);
    __asm__ volatile("cli\n\t");
    DebugStr("MemFlash.\n");
    MemFullFlash();
    DebugStr("Mov pml4 to cr3.\n");
    LoadDynamicPageTable(pml5);
    __asm__("sfence");
    DebugStr("Init Stack");
    u64 CoreCount = GetActualCoreCount();
    if (CoreCount > 288) CoreCount = 288;
    if (CoreCount == 0) CoreCount = 1;
    u64 PerCoreStack = (KERNEL_STACK_SIZE / CoreCount) & ~0xFULL;
    if (PerCoreStack < 0x10000) PerCoreStack = 0x10000;

    for (u64 i = 0; i < CoreCount; i++) {
        APStackSPs[i] = (u64)KernelStack + (i + 1) * PerCoreStack;
        APStackBPs[i] = (u64)KernelStack + (i + 1) * PerCoreStack;
    }
    DebugStr("AllocStack.\n");
    GopBack = (u32*)(u64)AlignedAlloc(&KernelPool, GopInfo.PixelsPerScanLine * GopInfo.VerticalResolution * sizeof(u32), 64);

    LoadGDT();
    InitIDT();

    __asm__ volatile(
        "call kernel_main\n\t"
        :
        :
        : "memory"
    );
    return 0;
}

void AC97PollTask(AC97Controller* AC97) {
    if (!AC97) return;

    u16 sr = inw(AC97->DmaPort + AC97_PO_SR);
    outw(AC97->DmaPort + AC97_PO_SR, sr & 0x001F);

    u8 civ = inb(AC97->DmaPort + AC97_PO_CIV);
    u8 lvi = inb(AC97->DmaPort + AC97_PO_LVI);

    if (civ < lvi) {
        return;
    }
    if (GlobalAudioPCMNextSizeCountOfBytePlay==0) return;

    MemCopy(GlobalAudioPCM, GlobalAudioPCMNext, GlobalAudioPCMSizeCountOfByte);
    GlobalAudioPCMSizeCountOfBytePlay=GlobalAudioPCMNextSizeCountOfBytePlay;

    AC97StartPlayback(AC97, GlobalAudioPCMRate);
}

void* PollWMTask(void* Rsvd){
    MakeEvent();
    WMPollPoll(&KernelGlobalWindowManager);
    return Rsvd;
}

__attribute__((naked)) void GlobalCoreInit(){
    __asm__ volatile(
        ".code16gcc\n\t"
        "cli\n\t"
        "cld\n\t"

        "movw $0x3F8, %dx\n\t"
        "movb $'R', %al\n\t"
        "outb %al, %dx\n\t"
        
        "xorw %ax, %ax\n\t"
        "movw %ax, %ds\n\t"
        "movw %ax, %es\n\t"
        "movw %ax, %ss\n\t"
        "movw $0x7000, %sp\n\t"

        "movw $0x3F8, %dx\n\t"
        "movb $'I', %al\n\t"
        "outb %al, %dx\n\t"

        "movl $0x01, %eax\n\t"
        "cpuid\n\t"
        "shrl $24, %ebx\n\t"
        "movl %ebx, %esi\n\t"

        "lgdtl (GDT32-GlobalCoreInit+0x7000)\n\t"

        "movl %cr0, %eax\n\t"
        "orl $0x1, %eax\n\t"
        "movl %eax, %cr0\n\t"

        "pushl $0x08\n\t"
        "pushl $(1f - GlobalCoreInit + 0x7000)\n\t"
        "lretl\n\t"

        ".code32\n\t"
        "1:\n\t"
        "cli\n\t"
        "movw $0x10, %ax\n\t"
        "movw %ax, %ds\n\t"
        "movw %ax, %es\n\t"
        "movw %ax, %ss\n\t"
        "movl $0x8000, %esp\n\t"

        "movw $0x3F8, %dx\n\t"
        "movb $'P', %al\n\t"
        "outb %al, %dx\n\t"

        "movl %cr4, %eax\n\t"
        "orl $(1 << 5), %eax\n\t"
        "movl %eax, %cr4\n\t"

        "movl $0xC0000080, %ecx\n\t"
        "rdmsr\n\t"
        "orl $(1 << 8), %eax\n\t"
        "wrmsr\n\t"

        "movl $0x277, %ecx\n\t"
        "movl $0x00000000, %edx\n\t"
        "movl $0x00010006, %eax\n\t"
        "wrmsr\n\t"

        "movl (Cr3), %eax\n\t"
        "movl %eax, %cr3\n\t"

        "lgdtl (GDT64-GlobalCoreInit+0x7000)\n\t"

        "movl %cr0, %eax\n\t"
        "orl $(1 << 31), %eax\n\t"
        "movl %eax, %cr0\n\t"

        "pushl $0x08\n\t"
        "pushl $(2f - GlobalCoreInit + 0x7000)\n\t"
        "lretl\n\t"

        ".code64\n\t"
        "2:\n\t"
        "cli\n\t"
        "movw $0x10, %ax\n\t"
        "movw %ax, %ds\n\t"
        "movw %ax, %es\n\t"
        "movw %ax, %ss\n\t"
        "movq $0x9000, %rsp\n\t"
        "xorq %rbp, %rbp\n\t"

        "movw $0x3F8, %dx\n\t"
        "movb $'X', %al\n\t"
        "outb %al, %dx\n\t"

        "movw $0x3F8, %dx\n\t"
        "movb $'J', %al\n\t"
        "outb %al, %dx\n\t"

        "movq $GlobalAMPInit, %rax\n\t"
        "jmp *%rax\n\t"

        "3: hlt\n\t"
        "jmp 3b\n\t"
        ".align 8\n\t"
        "GDTTable32:\n\t"
        "   .quad 0x0000000000000000\n\t"
        "   .quad 0x00cf9a000000ffff\n\t"
        "   .quad 0x00cf92000000ffff\n\t"
        "GDTTable32End:\n\t"
        "GDTTable64:\n\t"
        "   .quad 0x0000000000000000\n\t"
        "   .quad 0x00af9a0000000000\n\t"
        "   .quad 0x00cf920000000000\n\t"
        "GDTTable64End:\n\t"
        "GDT32:\n\t"
        "   .word GDTTable32End-GDTTable32-1\n\t"
        "   .long GDTTable32-GlobalCoreInit+0x7000\n\t"
        "GDT64:\n\t"
        "   .word GDTTable64End-GDTTable64-1\n\t"
        "   .quad GDTTable64-GlobalCoreInit+0x7000\n\t"
        ".global GlobalCoreInitEnd\n\t"
        "GlobalCoreInitEnd:\n\t"
    );
}

extern void GlobalCoreInitEnd();

void SendSIPI(u64 ApicId, u64 TrampolineVector) {
    u8 Vector = (u8)(TrampolineVector >> 12);
    u64 apic_base = rdmsr(0x1B);
    
    Bool is_x2apic = ((apic_base & (1ULL << 11)) != 0) && ((apic_base & (1ULL << 10)) != 0);

    if (is_x2apic) {
        while (rdmsr(0x830) & (1ULL << 12)) { __asm__("pause"); }
        
        wrmsr(0x830, (ApicId << 32) | 0x00004500ULL);
        
        while (rdmsr(0x830) & (1ULL << 12)) { __asm__("pause"); }
        
        SystemBusySleepMs(10);
        
        wrmsr(0x830, (ApicId << 32) | 0x00004600ULL | Vector);
        while (rdmsr(0x830) & (1ULL << 12)) { __asm__("pause"); }
        
        SystemBusySleepUs(200);
        
        wrmsr(0x830, (ApicId << 32) | 0x00004600ULL | Vector);
        while (rdmsr(0x830) & (1ULL << 12)) { __asm__("pause"); }
        
    } else {
        volatile u32* IcrLow  = (volatile u32*)0xFEE00300;
        volatile u32* IcrHigh = (volatile u32*)0xFEE00310;
        
        while (*IcrLow & (1ULL << 12)) { __asm__("pause"); }
        
        *IcrHigh = (u32)((ApicId & 0xFF) << 24);
        *IcrLow  = 0x00004500;
        __asm__ volatile("mfence" ::: "memory");
        
        while (*IcrLow & (1ULL << 12)) { __asm__("pause"); }
        SystemBusySleepMs(10);
        
        while (*IcrLow & (1ULL << 12)) { __asm__("pause"); }
        *IcrHigh = (u32)((ApicId & 0xFF) << 24);
        *IcrLow  = 0x00004600 | Vector;
        __asm__ volatile("mfence" ::: "memory");
        
        while (*IcrLow & (1ULL << 12)) { __asm__("pause"); }
        SystemBusySleepUs(200);
        
        while (*IcrLow & (1ULL << 12)) { __asm__("pause"); }
        *IcrHigh = (u32)((ApicId & 0xFF) << 24);
        *IcrLow  = 0x00004600 | Vector;
        __asm__ volatile("mfence" ::: "memory");
        
        while (*IcrLow & (1ULL << 12)) { __asm__("pause"); }
    }
}

Bool IsX2ApicSupported(void) {
    u32 eax, ebx, ecx, edx;
    __asm__ volatile("cpuid" : "=a"(eax),"=b"(ebx),"=c"(ecx),"=d"(edx) : "a"(1));
    if (!(ecx & (1 << 21))) return false;

    u64 base = rdmsr(0x1B);
    if (!(base & (1ULL << 8))) return false;
    if (base & (1ULL << 11)) return true;
    return true;
}

#pragma clang optimize off
__attribute__((target("no-avx,no-avx2")))
void GlobalAMPMain(void) {
    __asm__ volatile("sti");
    APAPICIDsBusy[TmpCoreWakeID]=0;
    __asm__ volatile("sfence" ::: "memory");
    __asm__ volatile("mfence" ::: "memory");
    GOPClear(PACK_COLOR(0x00, 0x00, 0x00, 0xFF));
    GOPStringWH(0, 0, 800, 600, ScreenDebug, PACK_COLOR(0, 0, 0, 0), PACK_COLOR(0xFF, 0xFF, 0xFF, 0xFF), KernelFonts, 1);
    GOPFlash();
    while (1) __asm__ volatile("hlt");
}

#pragma clang optimize off
__attribute__((target("no-avx,no-avx2"))) void GlobalAMPInit(){
    __asm__ volatile("cli");

    u64 sp = APStackSPs[TmpCoreWakeID];
    u64 bp = APStackBPs[TmpCoreWakeID];
    if (sp < 0x1000 || sp > 0x800000000ULL) {
        while (1) __asm__ volatile("hlt");
    }
    __asm__ volatile(
        "movq %0, %%rsp\n\t"
        "movq %1, %%rbp\n\t"
        :: "r"(sp), "r"(bp) : "memory"
    );
    DebugStr("Stack At 0x"); DebugU64(sp); DebugChar('\n');
    DebugStr("\nCore AMP Init Start\n");

    InitIDTAP();

    DebugStr("Init Apic\n");

    DebugChar('0');
    Bool need_x2apic = (APAPICIDs[TmpCoreWakeID] > 255);

    Bool X2ApicSupported = 0;
    u32 eax, ebx, ecx, edx;
    __asm__ volatile("cpuid" : "=a"(eax),"=b"(ebx),"=c"(ecx),"=d"(edx) : "a"(1));
    if (ecx & (1 << 21)) {
        X2ApicSupported = 1;
    }
    DebugChar('1');

    u64 base = rdmsr(0x1B);
    Bool dmar_opt_out = false;
    if (DMARAddr != 0) {
        u8* dmar = (u8*)DMARAddr;
        if (dmar[0]=='D' && dmar[1]=='M' && dmar[2]=='A' && dmar[3]=='R') {
            u8 flags = dmar[0x25];
            dmar_opt_out = ((flags & 0x3) == 0x3);
        }
    }
    DebugChar('2');

    base |= (1ULL << 8);
    if (X2ApicSupported && need_x2apic && !dmar_opt_out) {
        base |=  (1ULL << 11);
        base |=  (1ULL << 10);
        wrmsr(0x1B, base);
        __asm__ volatile("mfence" ::: "memory");
    } else {
        base |=  (1ULL << 11);
        base &= ~(1ULL << 10);
        wrmsr(0x1B, base);
        __asm__ volatile("mfence" ::: "memory");
    }
    DebugChar('3');

    u64 apic_base_now = rdmsr(0x1B);
    Bool is_x2apic = ((apic_base_now & (1ULL << 11)) && (apic_base_now & (1ULL << 10)));
    if (is_x2apic && X2ApicSupported) {
        u64 svr_val = rdmsr(0x80F);
        svr_val |= (1ULL << 8) | 0xFF;
        wrmsr(0x80F, svr_val);
        wrmsr(0x808, 0x0);
        __asm__ volatile("mfence" ::: "memory");
    } else {
        DebugStr("ApicInit Core\n");
        u64 apic_addr = apic_base_now & ~0xFFFULL;
        if (apic_addr < 0x1000 || apic_addr > 0xFFFFFFFFULL) {
            DebugStr("APIC MMIO addr invalid, skip\n");
        } else {
            volatile u32* svr = (volatile u32*)(apic_addr + 0xF0);
            u32 val = *svr;
            val |= (1 << 8) | 0xFF;
            *svr = val;
            __asm__ volatile("mfence" ::: "memory");

            volatile u32* tpr = (volatile u32*)(apic_addr + 0x80);
            *tpr = 0x00;
            __asm__ volatile("mfence" ::: "memory");
        }
    }
    DebugChar('4');
    DebugStr("\nInited Apic\n");

    InitClock();
    FPUInit();
    SIMDInit();

    u32 safe_id = TmpCoreWakeID;
    if (safe_id >= 288) safe_id = 0;
    SetCurrentCoreId(safe_id);

    DebugStr("Core "); DebugU64(safe_id); DebugStr(" Init.\n");
    DebugStr("APIC ID=0x"); DebugU64(APAPICIDs[safe_id]); DebugChar('\n');

    __asm__ volatile("sti");

    GlobalAMPMain();
}
#pragma clang optimize on

void SendINIT(u32 TargetApicId) {
    DebugStr("SendINIT Start\n");
    DebugU64(TargetApicId); DebugChar('\n');

    u64 ApicBase = rdmsr(0x1B);
    Bool is_currently_x2apic = (ApicBase & (3ULL << 10)) == (3ULL << 10);

    if (is_currently_x2apic) {
        DebugStr("x2Apic mode active\n");
        DebugStr("Send INIT via MSR 0x830\n");
        wrmsr(0x830, ((u64)TargetApicId << 32) | 0x00004500ULL);
    } else {
        DebugStr("x1Apic (MMIO) mode active\n");
        volatile u32* IcrLow = (volatile u32*)0xFEE00300;
        volatile u32* IcrHigh = (volatile u32*)0xFEE00310;
        while (*IcrLow & (1 << 12)) {
            __asm__("pause");
        }

        *IcrHigh = (u32)((TargetApicId & 0xFF) << 24);
        
        *IcrLow = 0x00004500;

        while (*IcrLow & (1 << 12)) {
            __asm__("pause");
        }
    }
    DebugStr("SendINIT End\n");
}

void InitAllCore(){
    MADTHeader* MADT = (MADTHeader*)MADTAddr;
    if (!MADT) {
        APAPICIDs[0] = GetCurrentApicId();
        APCount = 1;
        return;
    }
    DebugStr("MADT=0x");DebugU64(MADTAddr);DebugChar('\n');
    u32 BspApicId = GetCurrentApicId();

    TmpGlobalRSI=(u64)&GlobalAMPInit;

    APAPICIDs[0] = BspApicId;
    APCount = 1;

    u8* ptr = (u8*)MADT + sizeof(MADTHeader);
    u8* end = (u8*)MADT + MADT->Length;

    while (ptr < end) {
        MADTEntryHeader* entry = (MADTEntryHeader*)ptr;
        
        if (entry->Length == 0) break; 

        if (entry->Type == 0) {
            MADTLocalApic* lapic = (MADTLocalApic*)entry;
            if (lapic->Flags & 1) {
                u32 apicId = lapic->ApicId;
                if (apicId != BspApicId) {
                    APAPICIDs[APCount++] = apicId;
                }
            }
        }
        else if (entry->Type == 9) {
            MADTLocalX2Apic* x2lapic = (MADTLocalX2Apic*)entry;
            if (x2lapic->Flags & 1) {
                u32 apicId = x2lapic->X2ApicId;
                if (apicId != BspApicId) {
                    APAPICIDs[APCount++] = apicId;
                }
            }
        }
        ptr += entry->Length;
        if (APCount >= 288) break;
    }

    BspApicId = GetCurrentApicId();

    Bool need_x2apic = (BspApicId > 255);
    for (u32 i = 0; i < APCount; i++) {
        if (APAPICIDs[i] > 255) {
            need_x2apic = true;
            break;
        }
    }

    Bool X2ApicSupported = 0;

    u32 eax, ebx, ecx, edx;
    __asm__ volatile("cpuid" : "=a"(eax),"=b"(ebx),"=c"(ecx),"=d"(edx) : "a"(1));
    if (!(ecx & (1 << 21))) {
        X2ApicSupported = 0;
        goto InitNow;
    }

    u64 base = rdmsr(0x1B);

    if (!(base & (1ULL << 8))) {
        base |= (1ULL << 8);
        base |= (1ULL << 11);
        base &= ~(1ULL << 10);
        wrmsr(0x1B, base);
    }

    X2ApicSupported = 1;

    InitNow:
    {
        Bool dmar_opt_out = false;
        if (DMARAddr != 0) {
            u8* dmar = (u8*)DMARAddr;
            if (dmar[0]=='D' && dmar[1]=='M' && dmar[2]=='A' && dmar[3]=='R') {
                u8 flags = dmar[0x25];
                dmar_opt_out = ((flags & 0x3) == 0x3);
            }
            if (need_x2apic && X2ApicSupported && !dmar_opt_out) {
                u64 apic_base = rdmsr(0x1B);
                if ((apic_base & (1ULL << 11)) == 0) {
                    apic_base |= (1ULL << 10);
                    apic_base |= (1ULL << 11);
                    apic_base |= (1ULL << 8);
                    wrmsr(0x1B, apic_base);
                }
            }else{
                u32 w = 1;
                for (u32 r = 1; r < APCount; r++) {
                    if (APAPICIDs[r] <= 255) {
                        APAPICIDs[w++] = APAPICIDs[r];
                    } else {
                        DebugStr("Drop APIC ID>255: "); DebugU64(APAPICIDs[r]); DebugChar('\n');
                    }
                }
                APCount = w;
            }
        }
    }

    DebugStr("BspApicId=0x");DebugU64(BspApicId);DebugChar('\n');
    DebugStr("APBootloaderAddr=0x");DebugU64(APBootloaderAddr);DebugChar('\n');
    MemCopy((void*)0x7000, GlobalCoreInit, (u64)GlobalCoreInitEnd - (u64)GlobalCoreInit);
    GOPClear(PACK_COLOR(0x00, 0xAA, 0xAA, 0xFF));
    GOPStringWH(0, 0, 800, 600, ScreenDebug, PACK_COLOR(0, 0, 0, 0), PACK_COLOR(0xFF, 0xFF, 0xFF, 0xFF), KernelFonts, 1);
    GOPFlash();
    __asm__ volatile("sfence");
    for (u32 LogicalIndex = 1; LogicalIndex < APCount; LogicalIndex++) {
        u32 TargetApicId = APAPICIDs[LogicalIndex];

        TmpCoreWakeID = LogicalIndex; 
        APAPICIDsBusy[LogicalIndex] = 1;

        SendSIPI(TargetApicId, 0x7000);

        DebugStr("\nSendSIPI Core\n");

        while (APAPICIDsBusy[TmpCoreWakeID]) {
            __asm__("pause");
        }
        DebugStr("Core Init Finish\n");
    }
    GOPClear(PACK_COLOR(0x00, 0xAA, 0xAA, 0xFF));
    GOPStringWH(0, 0, 800, 600, ScreenDebug, PACK_COLOR(0, 0, 0, 0), PACK_COLOR(0xFF, 0xFF, 0xFF, 0xFF), KernelFonts, 1);
    GOPFlash();
}

typedef struct {
    union{
        u8 A[];
        u8 B[][512];
    };
} TestStruct;

__attribute__((force_align_arg_pointer)) void kernel_main() {
    DebugStr("Jumped To KernelMain.\n");
    if (!GopBack) {
        DebugStr("ERROR: GopBack is NULL!\n");
        while(1) __asm__ volatile("cli\n\t hlt");
    }
    
    GOPClear(PACK_COLOR(0x00, 0x00, 0x00, 0xFF));
    GOPStringWH(0, 0, 800, 600, ScreenDebug, PACK_COLOR(0, 0, 0, 0), PACK_COLOR(0xFF, 0xFF, 0xFF, 0xFF), KernelFonts, 1);
    GOPFlash();

    GOPClear(PACK_COLOR(0x00, 0xAA, 0xAA, 0xFF));
    GOPStringWH(0, 0, 800, 600, ScreenDebug, PACK_COLOR(0, 0, 0, 0), PACK_COLOR(0xFF, 0xFF, 0xFF, 0xFF), KernelFonts, 1);
    GOPFlash();
    MemSet((u8*)APAPICIDsBusy,0,288);
    SetCurrentCoreId(0);
    DebugStr("Seted CoreID\n");
    GOPClear(PACK_COLOR(0x00, 0xAA, 0xAA, 0xFF));
    GOPStringWH(0, 0, 800, 600, ScreenDebug, PACK_COLOR(0, 0, 0, 0), PACK_COLOR(0xFF, 0xFF, 0xFF, 0xFF), KernelFonts, 1);
    GOPFlash();
    DebugStr("Init All Core\n");
    GOPClear(PACK_COLOR(0x00, 0xAA, 0xAA, 0xFF));
    GOPStringWH(0, 0, 800, 600, ScreenDebug, PACK_COLOR(0, 0, 0, 0), PACK_COLOR(0xFF, 0xFF, 0xFF, 0xFF), KernelFonts, 1);
    GOPFlash();
    if (GetActualCoreCount()>1) InitAllCore();
    DebugStr("Inited All Core\n");
    GOPClear(PACK_COLOR(0x00, 0xAA, 0xAA, 0xFF));
    GOPStringWH(0, 0, 800, 600, ScreenDebug, PACK_COLOR(0, 0, 0, 0), PACK_COLOR(0xFF, 0xFF, 0xFF, 0xFF), KernelFonts, 1);
    GOPFlash();
    WaitForAllAPNotBusy();

    DebugStr("Cores Init Finish\n");
    GOPClear(PACK_COLOR(0x00, 0x00, 0x00, 0xFF));
    GOPStringWH(0, 0, 800, 600, ScreenDebug, PACK_COLOR(0, 0, 0, 0), PACK_COLOR(0xFF, 0xFF, 0xFF, 0xFF), KernelFonts, 1);
    GOPFlash();
    
    if (!KernelPool.Head) {
        DebugStr("ERROR: KernelPool.Head is NULL!\n");
        while(1) __asm__ volatile("cli\n\t hlt");
    }
    
    DebugStr("KernelPool.Head at 0x");
    DebugU64((u64)KernelPool.Head);
    DebugStr(" Size: ");    
    DebugU64(KernelPool.Head->Size);
    DebugStr(" is_free: ");
    DebugU8(KernelPool.Head->is_free);
    DebugStr(" Next block Size: ");
    if (KernelPool.Head->next) {
        DebugU64(KernelPool.Head->next->Size);
    } else {
        DebugStr("NULL");
    }
    DebugChar('\n');

    GopMode.FrameBufferSize = GopInfo.PixelsPerScanLine * GopInfo.VerticalResolution * sizeof(u32);
    DebugStr("GopBack = ");
    DebugU64((u64)GopBack);
    DebugChar('\n');
    DebugStr("GopOut = ");
    DebugU64((u64)GopOut);
    DebugChar('\n');
    DebugStr("PixelsPerScanLine = ");
    DebugU32(GopInfo.PixelsPerScanLine);
    DebugChar('\n');
    DebugStr("VerticalResolution = ");
    DebugU32(GopInfo.VerticalResolution);
    DebugChar('\n');
    DebugStr("pixelCount = ");
    DebugU32(GopInfo.PixelsPerScanLine * GopInfo.VerticalResolution);
    DebugChar('\n');
    DebugStr("PixelFormat = ");
    DebugU32(GopInfo.PixelFormat);
    DebugChar('\n');
    MemFullFlash();

    outb(0x64, 0xD4);
    outb(0x60, 0xF4);

    while (inb(0x64) & 0x02);
    outb(0x64, 0xAA);
    while (!(inb(0x64) & 0x01));
    u8 Res = inb(0x60);
    if (Res==0x55){
        u8 ack = 0;
        for (int i = 0; i < 100000; i++) {
            if (inb(0x64) & 0x01) {
                ack = inb(0x60);
                break;
            }
        }
        DebugStr("Mouse enable ACK: ");
        DebugU8(ack);
        DebugStr("\n");
        GOPClear(PACK_COLOR(0x00, 0xFF, 0xFF, 0xFF));
        GOPFlash();
        while (inb(0x64) & 0x02);
        outb(0x60, 0xF4);
        ack = 0;
        for (int i = 0; i < 100000; i++) {
            if (inb(0x64) & 0x01) {
                ack = inb(0x60);
                if (ack == 0xFA) break;
            }
        }
        GOPClear(PACK_COLOR(0000, 0x00, 0xFF, 0xFF));
        GOPFlash();
    }

    outb(0x21, 0xFF);
    outb(0xA1, 0xFF);

    u64 ApicBase = rdmsr(0x1B);
    ApicBase |= (1 << 11); 
    wrmsr(0x1B, ApicBase);

    volatile u32* LocalApicSVR = (volatile u32*)0xFEE000F0ULL;
    *LocalApicSVR = (*LocalApicSVR) | (1 << 8) | 0xFF;

    for (u8 i = 0; i < 24; i++) {
        IOApicWrite(0x10 + (i * 2), 1 << 16, 0);
    }

    IOApicWrite(0x10+(1*2)+01, 0x00, 0x0);
    IOApicWrite(0x10+(1*2)+00, 0x21, 0x0);
    IOApicWrite(0x10+(12*2)+1, 0x00, 0x0);
    IOApicWrite(0x10+(12*2)+0, 0x2C, 0x0);
    u32 low = IOApicRead(0x28, 0);
    DebugStr("After write, low: 0x");
    DebugU32(low);
    DebugStr("\n");
    u32 high = IOApicRead(0x29, 0);
    DebugStr("IRQ12 high: 0x");
    DebugU32(high);
    DebugStr("\n");

    GOPClear(PACK_COLOR(0xA5, 0xA5, 0xA5, 0xFF));
    GOPFlash();

    u64 base = GetHPETMMIO(HPETAddr);
    DebugStr("HPET base from GetHPETMMIO: 0x");
    DebugU64(base);
    DebugChar('\n');

    AcpiHPETRegs *hpet = (AcpiHPETRegs*)base;

    DebugStr("HPET CapabilitiesAndIDRegister: 0x");
    DebugU64(hpet->roCapabilitiesAndIDRegister);
    DebugChar('\n');
    DebugStr("HPET ConfigurationRegister: 0x");
    DebugU64(hpet->rwConfigurationRegister);
    DebugChar('\n');
    DebugStr("HPET InterruptStatusRegister: 0x");
    DebugU64(hpet->rw1cInterruptStatusRegister);
    DebugChar('\n');
    DebugStr("HPET CounterValueRegister: 0x");
    DebugU64(hpet->rwCounterValueRegister);
    DebugChar('\n');
    DebugStr("HPET Timer0ConfigurationAndCapabilityRegister: 0x");
    DebugU64(hpet->rwTimer0ConfigurationAndCapabilityRegister);
    DebugChar('\n');
    DebugStr("HPET Timer0ComparatorValueRegister: 0x");
    DebugU64(hpet->rwTimer0ComparatorValueRegister);
    DebugChar('\n');
    DebugStr("HPET Timer0FSBInterruptRouteRegister: 0x");
    DebugU64(hpet->rwTimer0FSBInterruptRouteRegister);
    DebugChar('\n');

    hpet->rwConfigurationRegister &= ~1ULL;
    DebugStr("HPET Disabled\n");

    u64 cfg = hpet->rwTimer0ConfigurationAndCapabilityRegister;
    if ((cfg >> 32) & (1 << 4)) {
        DebugStr("Have Pin 4\n");
        cfg &= ~(0x3E0CULL); 
        
        cfg |= (4 << 9) | (1 << 2) | (1 << 4);
        
        hpet->rwTimer0ConfigurationAndCapabilityRegister = cfg;

        IOApicWrite(0x18, 0xEF, 0x0);
    } else {
        cfg &= ~(0x1FULL << 9);

        cfg |= (1 << 2) | (1 << 4);

        hpet->rwTimer0ConfigurationAndCapabilityRegister = cfg;

        IOApicWrite(0x14, 0xEF, 0x0);
    }
    DebugStr("Timer0 Config: 0x");
    DebugU64(hpet->rwTimer0ConfigurationAndCapabilityRegister);
    DebugChar('\n');

    DebugStr("TSC Frequency: ");
    DebugU64(GetTscFrequency());
    DebugChar('\n');

    u64 config_after = hpet->rwConfigurationRegister;
    DebugStr("HPET Config after enable: 0x");
    DebugU64(config_after);
    DebugStr("\n");

    CalibrateHPET();

    hpet->rwConfigurationRegister &= ~1ULL;

    cfg = hpet->rwTimer0ConfigurationAndCapabilityRegister;
    cfg |= (1ULL << 6);
    hpet->rwTimer0ConfigurationAndCapabilityRegister = cfg;

    u64 target_delay_us = 4166666ULL / 1000ULL;
    u64 hpet_cycles = target_delay_us * HPETFreqHz / 1000000ULL;

    u64 current_main = hpet->rwCounterValueRegister;
    hpet->rwTimer0ComparatorValueRegister = current_main + hpet_cycles;

    hpet->rwConfigurationRegister |= 1ULL;

    SystemGetTimeMillis();

    PCIScanBus();

    DebugStr("Init Task\n");
    TaskInit();

    GOPClear(PACK_COLOR(0x7F, 0x7F, 0x7F, 0xFF));
    GOPFlash();

    InitRandomString();
    DebugStr("Test Random String:\n");
    Size64 Time=rdrand()%26;
    for (Size64 Idx=0;Idx<Time;Idx++) {
        DebugStr(RandomString());
        DebugChar(' ');
    }
    DebugChar('\n');

    DebugStr("StringList[0]=");DebugStr(StringList[0]);DebugChar('\n');

    char *msg = "Neuro-Sama Number 1!!!";
    u32 msg_len = (u32)StrLen(msg) * 8;
    u32 total_width = GopInfo.HorizontalResolution;
    u32 repeat_count = total_width / msg_len;

    DebugStr("Main Kernel Poll Loop.\n");
    u64 Now = SystemGetTimeNano();

    InitAudioBuff();

    __asm__ volatile("sti");
    u64 rflags;
    __asm__ volatile("pushfq; pop %0" : "=r"(rflags));
    DebugStr("After sti: IF = "); DebugU32((rflags >> 9) & 1); DebugChar('\n');

    xHCIController* KernelXHCI=InitXHCI();
    DebugStr("KernelXHCI At=");DebugU64((u64)KernelXHCI);DebugChar('\n');

    AHCIController* KernelAHCI=InitAHCI();
    DebugStr("KernelAHCI At");DebugU64((u64)KernelAHCI);DebugChar('\n');

    NewShellWindow();
    NewNotepadWindow();
    Task PollWMTaskTask={
        .Active=1,
        .Arg=NULL,
        .Target=NULL,
        .IntervalNs=1000000,
        .NextWaitNs=0,
        .Name=L"WMTask",
        .CallFunc=PollWMTask,
        .DelFunc=NULL,
        .CoreID=1
    };
    TaskAdd(PollWMTaskTask, 1);
    DebugStr("WM Core=0x");DebugU64(PollWMTaskTask.CoreID % GetActualCoreCount());DebugChar('\n');

    DebugStr("Core Count:");DebugU64(GetActualCoreCount());DebugChar('\n');
    
    MemSet((u8*)APAPICIDsBusy,0,288);

    hpet->rwConfigurationRegister |= 1ULL;
    DebugStr("HPET Enabled\n");

    __asm__ volatile("sti");

    __asm__("int $0x21");
    Char=0;

    MemSet(GopOut,0xA5,GopMode.FrameBufferSize);

    DebugStr("WelCome To Temple, User.\n");

    TestStruct* TestStructObj=Alloc(&KernelPool, 1024);
    DebugStr("TestStructObj->A[] Base==0x");DebugU64((u64)&TestStructObj->A);DebugChar('\n');
    DebugStr("TestStructObj->B[][] Base==0x");DebugU64((u64)&TestStructObj->B);DebugChar('\n');
    DebugStr("TestStructObj->A[512] Base==0x");DebugU64((u64)&TestStructObj->A[512]);DebugChar('\n');
    DebugStr("TestStructObj->B[1][0] Base==0x");DebugU64((u64)&TestStructObj->B[1][0]);DebugChar('\n');
    Free(&KernelPool, TestStructObj);

    xHCITransferRequestBlock TRBs={{.U=0},{.U=0},{.CycleBit=1,.TRBType=9}};
    SendCommandRing(KernelXHCI,TRBs);

    PrintRegsValue(KernelXHCI);

    DebugStr("=== ICR test ===\n");
    volatile u32* IcrLow  = (volatile u32*)0xFEE00300;
    volatile u32* IcrHigh = (volatile u32*)0xFEE00310;
    while (*IcrLow & (1 << 12)) __asm__("pause");
    *IcrHigh = 0;
    *IcrLow  = 0x00004000 | 0xED;
    DebugStr("=== ICR sent ===\n");

    KernelXHCI->CommandRingCount=0;

    xHCITransferRequestBlock EnableSlotTRB = {0};
    EnableSlotTRB.NormalTRBField2.TRBType = 9;
    SendCommandRing(KernelXHCI, EnableSlotTRB);

    for (volatile u64 i = 0; i < 10000000; i++) __asm__("pause");

    DebugStr("After EnableSlot: CommandRingCount = "); DebugU32(KernelXHCI->CommandRingCount); DebugChar('\n');
    DebugStr("After EnableSlot: IMAN = 0x"); DebugU32(KernelXHCI->RuntimeRegs->Interrupter[0].IMAN.U); DebugChar('\n');
    DebugStr("After EnableSlot: ERDP = 0x"); DebugU64(KernelXHCI->RuntimeRegs->Interrupter[0].ERDP.U); DebugChar('\n');

    xHCIInputContext* InputCtx = (xHCIInputContext*)AlignedAlloc(&KernelPool, sizeof(xHCIInputContext), 64);
    MemSet((u8*)InputCtx, 0, sizeof(xHCIInputContext));

    xHCITransferRequestBlock* EP0Ring = (xHCITransferRequestBlock*)AlignedAlloc(&KernelPool, 64 * sizeof(xHCITransferRequestBlock), 64);
    MemSet((u8*)EP0Ring, 0, 64 * sizeof(xHCITransferRequestBlock));

    InputCtx->InputControl.Field0.U = 0;
    InputCtx->InputControl.Field1.U = (1U << 0) | (1U << 1);
    InputCtx->InputControl.Field2.U = 0;

    u32 PortNum = 5;
    InputCtx->SlotContext.SlotField0.RouteString = 0;
    InputCtx->SlotContext.SlotField0.Speed = KernelXHCI->OperationalRegs->PORTS[PortNum-1].PORTSC.rosPortSpeed;
    InputCtx->SlotContext.SlotField0.MultiTT = 0;
    InputCtx->SlotContext.SlotField0.Hub = 0;
    InputCtx->SlotContext.SlotField0.ContextEntries = 1;

    InputCtx->SlotContext.SlotField1.MaxExitLatency = 0;
    InputCtx->SlotContext.SlotField1.RootHubPortNumber = PortNum;
    InputCtx->SlotContext.SlotField1.NumberOfPorts = 0;

    InputCtx->SlotContext.SlotField2.ParentHubSlotID = 0;
    InputCtx->SlotContext.SlotField2.ParentPortNumber = 0;
    InputCtx->SlotContext.SlotField2.TThinkTime = 0;
    InputCtx->SlotContext.SlotField2.InterrupterTarget = 0;

    InputCtx->SlotContext.SlotField3.USBDeviceAddress = 0;
    InputCtx->SlotContext.SlotField3.SlotState = 0;

    InputCtx->EndpointContext[0].Field0.EndPointState = 0;
    InputCtx->EndpointContext[0].Field0.Mult = 0;
    InputCtx->EndpointContext[0].Field0.MaxPrimaryStreams = 0;
    InputCtx->EndpointContext[0].Field0.LinearStreamArray = 0;
    InputCtx->EndpointContext[0].Field0.Interval = 0;
    InputCtx->EndpointContext[0].Field0.MaxEndPointServiceTimeIntervalPayloadHigh = 0;

    InputCtx->EndpointContext[0].Field1.ErrorCount = 3;
    InputCtx->EndpointContext[0].Field1.EndPointType = 4;
    InputCtx->EndpointContext[0].Field1.HostInititeDisable = 0;
    InputCtx->EndpointContext[0].Field1.MaxBurstSize = 0;
    InputCtx->EndpointContext[0].Field1.MaxPacketSize = 8;

    InputCtx->EndpointContext[0].Field2.U = ((u64)EP0Ring & ~0xFULL) | 1;

    InputCtx->EndpointContext[0].Field3.AverageTRBLength = 8;
    InputCtx->EndpointContext[0].Field3.MaxEndpointServiceTimeIntervalPayloadLow = 0;

    xHCITransferRequestBlock AddrDevTRB = {0};
    AddrDevTRB.NormalTRBField0.Base = (u64)InputCtx;
    AddrDevTRB.NormalTRBField1.U = ((u32)1 << 24);
    AddrDevTRB.NormalTRBField2.TRBType = 11;

    SendCommandRing(KernelXHCI, AddrDevTRB);

    for (volatile u64 i = 0; i < 10000000; i++) __asm__("pause");

    DebugStr("After AddrDev: CommandRingCount = "); DebugU32(KernelXHCI->CommandRingCount); DebugChar('\n');
    DebugStr("After AddrDev: IMAN = 0x"); DebugU32(KernelXHCI->RuntimeRegs->Interrupter[0].IMAN.U); DebugChar('\n');
    DebugStr("After AddrDev: ERDP = 0x"); DebugU64(KernelXHCI->RuntimeRegs->Interrupter[0].ERDP.U); DebugChar('\n');
    DebugStr("After AddrDev: USBSTS = 0x"); DebugU32(KernelXHCI->OperationalRegs->USBSTS.U); DebugChar('\n');

    u64 LastFrameTime = SystemGetTimeNano();
    while (1){
        WaitForEvent();
        GOPClear(PACK_COLOR(0x00, 0x00, 0x00, 0xFF));
        for (u32 i = 0; i < repeat_count; i++) {
            GOPString(i * msg_len, 
                    GopInfo.VerticalResolution - 19, 
                    msg,
                    PACK_COLOR(0x00, 0x00, 0x00, 0x00),
                    PACK_COLOR(0xFF, 0xFF, 0xFF, 0xFF),
                    KernelFonts,
                    0);
        }
        Now = SystemGetTimeNano();
        
        WMPollDraw(&KernelGlobalWindowManager);

        TaskPoll();

        GOPRectFill((u32[2]){0, GopInfo.HorizontalResolution}, (u32[2]){0, 21}, PACK_COLOR(0x00, 0xAA, 0xAA, 0xFF));
        GOPString(4,2,"Neuro-SamaOS :>)",PACK_COLOR(0x00, 0x00, 0x00, 0x00),PACK_COLOR(0xFF, 0xFF, 0xFF, 0xFF),KernelFonts, 0);

        GOPRectFill((u32[2]){GlobalPS2Mouse.X-5,GlobalPS2Mouse.X+5}, (u32[2]){GlobalPS2Mouse.Y-5,GlobalPS2Mouse.Y+5}, PACK_COLOR(0x00, 0x00, 0x00, 0xFF));
        GOPRectFill((u32[2]){GlobalPS2Mouse.X-4,GlobalPS2Mouse.X+4}, (u32[2]){GlobalPS2Mouse.Y-4,GlobalPS2Mouse.Y+4}, PACK_COLOR(0xFF, 0xFF, 0xFF, 0xFF));
        
        GOPStringWH(0, 0, 800, 600, ScreenDebug, PACK_COLOR(0, 0, 0, 0), PACK_COLOR(0xFF, 0xFF, 0xFF, 0xFF), KernelFonts, 1);
        
        GOPFlash();

        Now=SystemGetTimeNano();
        DeltaMs = (f64)(Now - LastFrameTime) / 1000000.0;
        LastFrameTime=Now;

        WaitForAllAPNotBusy();

        Char = 0;
        Get = 0;
        Get0xE0Down = 0;
        TopF1To12Key = 0;
    }
}
