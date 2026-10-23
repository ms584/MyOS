#include <efi.h>
#include <efilib.h>
#include "boot.h"
#include "framebuffer.h"
#include "uefi_assets.h"

struct memory_region {UINT64 base,length;UINT32 type,attributes;};
extern void uefi_enter(struct boot_info *,UINT64) __attribute__((noreturn));
static void out(UINT16 p,UINT8 c){__asm__ volatile("outb %0,%1"::"a"(c),"Nd"(p));}
static UINT8 in(UINT16 p){UINT8 c;__asm__ volatile("inb %1,%0":"=a"(c):"Nd"(p));return c;}
static void log_text(const char *s){while(*s){for(unsigned n=0;n<100000;++n)if(in(0x3fd)&32)break;out(0x3f8,*s++);}}
static void log_hex(UINT64 v){char s[19]="0x";for(unsigned i=0;i<16;++i)s[2+i]="0123456789abcdef"[(v>>(60-i*4))&15];s[18]=0;log_text(s);}
static EFI_STATUS fail(const CHAR16 *s,EFI_STATUS status){Print(L"MyOS UEFI: %s (%r)\r\n",s,status);log_text("UEFI ERROR\n");return status;}
static EFI_STATUS alloc(EFI_BOOT_SERVICES *bs,UINTN pages,EFI_PHYSICAL_ADDRESS *p){
    *p=0xffffffff;return uefi_call_wrapper(bs->AllocatePages,4,AllocateMaxAddress,EfiLoaderData,pages,p);
}
EFI_STATUS efi_main(EFI_HANDLE image,EFI_SYSTEM_TABLE *system)
{
    InitializeLib(image,system);
    out(0x3f9,0);out(0x3fb,0x80);out(0x3f8,1);out(0x3f9,0);out(0x3fb,3);
    log_text("MYOS UEFI LOADER\n");
    EFI_BOOT_SERVICES *bs=system->BootServices;
    uefi_call_wrapper(bs->SetWatchdogTimer,4,0,0,0,NULL);
    UINT32 a,b,c,d,max_leaf;unsigned physical_bits=36;
    __asm__ volatile("cpuid":"=a"(a),"=b"(b),"=c"(c),"=d"(d):"a"(0x80000000u));
    max_leaf=a;
    if(a<0x80000001u)return fail(L"extended CPU features unavailable",EFI_UNSUPPORTED);
    __asm__ volatile("cpuid":"=a"(a),"=b"(b),"=c"(c),"=d"(d):"a"(0x80000001u));
    if(!(d&(1u<<20)))return fail(L"NX required",EFI_UNSUPPORTED);
    if(max_leaf>=0x80000008u){
        __asm__ volatile("cpuid":"=a"(a),"=b"(b),"=c"(c),"=d"(d):"a"(0x80000008u));
        physical_bits=a&255u;
    }
    UINT64 current_stack;__asm__ volatile("mov %%rsp,%0":"=r"(current_stack));
    if((UINT64)uefi_enter>=0x100000000ull || current_stack>=0x100000000ull)
        return fail(L"loader must be below 4 GiB",EFI_UNSUPPORTED);
    EFI_GUID guid=EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop=NULL;
    EFI_STATUS status=uefi_call_wrapper(bs->LocateProtocol,3,&guid,NULL,(void **)&gop);
    if(EFI_ERROR(status))return fail(L"GOP unavailable",status);
    UINT32 selected=0xffffffff;
    for(UINT32 i=0;i<gop->Mode->MaxMode;++i){
        EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *m=NULL;UINTN size=0;
        status=uefi_call_wrapper(gop->QueryMode,4,gop,i,&size,&m);
        if(!EFI_ERROR(status)){
            if(m->HorizontalResolution==800 && m->VerticalResolution==600 &&
               m->PixelFormat==PixelBlueGreenRedReserved8BitPerColor)selected=i;
            uefi_call_wrapper(bs->FreePool,1,m);
        }
        if(selected!=0xffffffff)break;
    }
    if(selected==0xffffffff)return fail(L"800x600 BGR GOP mode required",EFI_UNSUPPORTED);
    status=uefi_call_wrapper(gop->SetMode,2,gop,selected);
    if(EFI_ERROR(status))return fail(L"SetMode",status);
    UINT64 fb=gop->Mode->FrameBufferBase;
    UINT64 pitch=(UINT64)gop->Mode->Info->PixelsPerScanLine*4;
    UINT64 span=pitch*600;
    /* Keep facts visible on the target before any allocation/handoff failure. */
    Print(L"MyOS UEFI GOP: base=0x%lx size=0x%lx\r\n",fb,(UINT64)gop->Mode->FrameBufferSize);
    Print(L"MyOS UEFI GOP: pitch=%lu physical-bits=%u\r\n",pitch,physical_bits);
    log_text("UEFI GOP base=");log_hex(fb);log_text(" size=");log_hex(gop->Mode->FrameBufferSize);
    log_text(" pitch=");log_hex(pitch);log_text("\n");
    if(gop->Mode->Info->HorizontalResolution!=800 || gop->Mode->Info->VerticalResolution!=600 ||
       gop->Mode->Info->PixelFormat!=PixelBlueGreenRedReserved8BitPerColor || pitch<3200 || pitch>16384)
        return fail(L"unsupported GOP geometry",EFI_UNSUPPORTED);
    if(gop->Mode->FrameBufferSize<span)return fail(L"framebuffer smaller than mode",EFI_UNSUPPORTED);
    if(!framebuffer_range(fb,span,physical_bits))return fail(L"unsupported framebuffer physical range",EFI_UNSUPPORTED);
    /* Reserve kernel+BSS. Stack and ISA DMA receive separately owned pages;
       low firmware memory beyond the kernel reservation may be unavailable.
       Never overwrite unowned
       firmware memory. Platforms unable to reserve this region stop here. */
    EFI_PHYSICAL_ADDRESS kernel=0x10000;
    status=uefi_call_wrapper(bs->AllocatePages,4,AllocateAddress,EfiLoaderData,0x70,&kernel);
    if(EFI_ERROR(status))return fail(L"kernel low-memory reservation",status);
    EFI_PHYSICAL_ADDRESS tables,info_address,map_address,stack_address,dma_allocation;
    if(EFI_ERROR((status=alloc(bs,7,&tables))) || EFI_ERROR((status=alloc(bs,4,&info_address))) ||
       EFI_ERROR((status=alloc(bs,16,&map_address))) ||
       EFI_ERROR((status=alloc(bs,MYOS_INITIAL_STACK_BYTES/4096u,&stack_address))))
        return fail(L"loader allocation",status);
    /* Allocate 64 KiB below the ISA 24-bit limit, then select an aligned 16 KiB
       subrange. It is entirely owned and cannot cross a 64 KiB DMA boundary. */
    dma_allocation=0xffffffu;
    status=uefi_call_wrapper(bs->AllocatePages,4,AllocateMaxAddress,EfiLoaderData,16,&dma_allocation);
    if(EFI_ERROR(status))return fail(L"ISA DMA allocation",status);
    UINT64 dma=(dma_allocation+MYOS_SB16_DMA_BYTES-1)&~((UINT64)MYOS_SB16_DMA_BYTES-1);
    struct boot_info *info=(void *)(UINTN)info_address;
    SetMem(info,4*4096,0);
    struct memory_region *regions=(void *)(UINTN)(info_address+4096);
    UINT8 *font=(void *)(UINTN)(info_address+3*4096);
    CopyMem((void *)(UINTN)kernel,(void *)kernel_blob,sizeof(kernel_blob));
    CopyMem(font,(void *)font_blob,sizeof(font_blob));
    *info=(struct boot_info){MYOS_BOOT_MAGIC,tables,stack_address+MYOS_INITIAL_STACK_BYTES,
        (UINT64)font,(UINT64)regions,0,800,600,(UINT32)pitch,fb,dma};
    UINT64 *pt=(void *)(UINTN)tables;SetMem(pt,7*4096,0);
    pt[0]=(tables+4096)|3;
    for(unsigned i=0;i<4;++i)pt[512+i]=(tables+(2+i)*4096)|3;
    for(unsigned i=0;i<2048;++i)pt[1024+i]=((UINT64)i*0x200000)|0x83;
    if(framebuffer_high(fb,span)){
        /* A separate shared supervisor mapping works before memory_init and
           under Ring3 CR3s. Keep physical framebuffer in the existing ABI. */
        if(!framebuffer_map(pt+6*512,fb,span,physical_bits))return fail(L"framebuffer mapping",EFI_UNSUPPORTED);
        pt[512+(MYOS_FRAMEBUFFER_WINDOW>>30)]=(tables+6*4096)|3;
        /* A range straddling4GiB also has a low identity alias. Match UC/NX. */
        for(UINT64 i=fb>>21;i<2048 && i<=((fb+span-1)>>21);++i)
            pt[1024+i]=(i*0x200000)|MYOS_FRAMEBUFFER_FLAGS;
        log_text("UEFI HIGH FRAMEBUFFER MAPPED\n");
    }
    UINTN length,key,stride;UINT32 version;
    for(unsigned attempt=0;attempt<3;++attempt){
        length=16*4096;
        status=uefi_call_wrapper(bs->GetMemoryMap,5,&length,(void *)(UINTN)map_address,&key,&stride,&version);
        if(EFI_ERROR(status) || stride<sizeof(EFI_MEMORY_DESCRIPTOR) || length%stride){
            log_text("UEFI MEMORY MAP ERROR\n");for(;;)__asm__ volatile("cli; hlt");
        }
        info->count=0;
        for(UINTN at=0;at<length;at+=stride){
            EFI_MEMORY_DESCRIPTOR *m=(void *)(UINTN)(map_address+at);
            if(!m->NumberOfPages)continue;
            if(info->count==256){log_text("UEFI MAP TOO LARGE\n");for(;;)__asm__ volatile("cli; hlt");}
            regions[info->count++]=(struct memory_region){m->PhysicalStart,m->NumberOfPages*4096,
                m->Type==EfiConventionalMemory?1:2,1};
        }
        status=uefi_call_wrapper(bs->ExitBootServices,2,image,key);
        if(!EFI_ERROR(status)){
            log_text("UEFI EXIT BOOT SERVICES OK\n");
            uefi_enter(info,tables);
        }
        /* Refresh only the map after a stale key; no allocation or console
           service is used after the first ExitBootServices attempt. */
        if(status!=EFI_INVALID_PARAMETER)break;
    }
    log_text("UEFI EXIT BOOT SERVICES FAILED\n");
    for(;;)__asm__ volatile("cli; hlt");
}
