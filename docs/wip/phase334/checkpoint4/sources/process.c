#include "boot.h"
#include "process.h"
#include "elf-launch.h"
_Static_assert(ELF_NAME_OFFSET+65+1031+80+32+32+ELF_HOST_RULE_SIZE<=4096,
               "owned native launcher arguments fit boot page");
#include "vm-ledger.h"
#include "elf.h"
#include "runtime-pack.h"
#include "native-ui.h"
#if PROCESS_RUNTIME_EXTERNAL
#include "asset.h"
#include "mbedtls/sha256.h"
#endif
#include "../build/platform-runtime-exports.h"
#define NX (1ull << 63)
static struct process processes[PROCESS_LIMIT];
static uint64_t next_pid = 1;
extern const unsigned char user_blob_start[], user_blob_end[];
extern const unsigned char platform_runtime_start[],platform_runtime_end[];

struct process *process_find(uint64_t pid)
{
    for (unsigned i = 0; i < PROCESS_LIMIT; ++i)
        if (processes[i].state != PROCESS_UNUSED && processes[i].pid == pid)
            return &processes[i];
    return 0;
}

static void release(struct process *process)
{
    if(process->files)fs_close_all(process->files);
    kfree(process->files);
    /* Only detached, noncurrent address spaces may reach this function. */
    process_signal_release(process);
    for (unsigned i = 0; i < process->page_count; ++i) if(process->image_pages?process->image_pages[i]:process->pages[i])page_free(process->image_pages?process->image_pages[i]:process->pages[i]);
    for(unsigned i=0;i<process->vm_page_count;++i)if(process->vm_pages[i])page_free(process->vm_pages[i]);
    for(unsigned i=0;i<PROCESS_THREAD_LIMIT;++i)kfree(process->threads[i].kernel_stack);
    kfree(process->image_pages);
    kfree(process->vm_pages);
    kfree(process->vm_areas);
    kfree(process->kernel_stack);
    kfree(process->tls_template);
    memset(process, 0, sizeof(*process));
}

static uint64_t create_image(const void *image,size_t image_size,uint64_t mode,uint64_t cookie,bool loaded)
{
    uint64_t flags = irq_save();
    struct process *process = 0;
    for (unsigned i = 0; i < PROCESS_LIMIT; ++i)
        if (processes[i].state == PROCESS_UNUSED) { process = &processes[i]; break; }
    if (!process) { irq_restore(flags); return 0; }
    if(!image || !image_size || image_size>PROCESS_CODE_PAGES*PAGE_SIZE){irq_restore(flags);return 0;}
    /* Keep growing file capacity out of the BIOS-reserved kernel BSS.
     * Allocate before publication; release() handles all partial failures. */
    process->files=kmalloc(FS_HANDLES*sizeof(*process->files));
    if(!process->files)goto failed;
    memset(process->files,0,FS_HANDLES*sizeof(*process->files));
    unsigned code_pages=loaded?(unsigned)((image_size+PAGE_SIZE-1)/PAGE_SIZE):1u;
    unsigned required=4+code_pages+2+(loaded?PROCESS_HEAP_PAGES:0);
    for (unsigned i = 0; i < required; ++i) {
        uintptr_t page = page_alloc();
        if (!page) goto failed;
        process->pages[process->page_count++] = page;
        memset((void *)page, 0, PAGE_SIZE);
    }
    process->kernel_stack = kmalloc(16384);
    if (!process->kernel_stack) goto failed;
    uint64_t *root = (uint64_t *)process->pages[0];
    uint64_t *pdpt = (uint64_t *)process->pages[1];
    uint64_t *pd = (uint64_t *)process->pages[2];
    uint64_t *pt = (uint64_t *)process->pages[3];
    root[0] = ((uint64_t *)boot_state.root)[0] & ~4ull;
    root[1] = process->pages[1] | 7;
    pdpt[0] = process->pages[2] | 7;
    pd[0] = process->pages[3] | 7;
    for(unsigned i=0;i<code_pages;++i){
        pt[i]=process->pages[4+i]|5; /* RX code/rodata. */
        size_t n=image_size-(size_t)i*PAGE_SIZE;
        if(n>PAGE_SIZE)n=PAGE_SIZE;
        memcpy((void *)process->pages[4+i],(const uint8_t *)image+(size_t)i*PAGE_SIZE,n);
    }
    unsigned data_index=4+code_pages, data_virt=loaded?PROCESS_CODE_PAGES:1u;
    pt[data_virt]=process->pages[data_index]|7|NX;
    /* Unmapped guard immediately below the stack. */
    pt[data_virt+2]=process->pages[data_index+1]|7|NX;
    if(loaded)for(unsigned i=0;i<PROCESS_HEAP_PAGES;++i)
        pt[32+i]=process->pages[data_index+2+i]|7|NX; /* 128 KiB user heap. */
    uint64_t *data = (uint64_t *)process->pages[data_index];
    if(!loaded){data[2]=cookie;data[3]=mode;}
    if (mode == 14) {
        /* Test-only buffers: RX -> RW crossing and a partial invalid request. */
        memcpy((void *)(process->pages[4] + PAGE_SIZE - 2), "AB", 2);
        memcpy((void *)process->pages[data_index], "CD", 2);
        memcpy((void *)(process->pages[5] + PAGE_SIZE - 4), "LEAK", 4);
    }
    /* RET is used by the NX-stack execution test. */
    *(uint8_t *)process->pages[data_index+1] = 0xc3;
    process->data_page = process->pages[data_index];
    process->root = process->pages[0];
    process->kernel_top = ((uintptr_t)process->kernel_stack + 16384) & ~15ull;
    process->frame = (struct frame *)(process->kernel_top - sizeof(struct frame));
    memset(process->frame, 0, sizeof(*process->frame));
    process->frame->rip = USER_BASE;
    process->frame->cs = 0x2b;
    process->frame->ss = 0x23;
    process->frame->rsp = USER_BASE + (loaded?(PROCESS_CODE_PAGES+2ull):3ull)*PAGE_SIZE + PAGE_SIZE - 8;
    process->main_stack_base=process->frame->rsp+8-PAGE_SIZE;
    process->main_stack_length=PAGE_SIZE;
    process->frame->flags = 0x202;       /* IF=1, IOPL=0. */
    process->pid = next_pid++;
    process->state = PROCESS_RUNNING;
    if (!scheduler_attach_process(process)) goto failed;
    uint64_t pid = process->pid;
    irq_restore(flags);
    return pid;
failed:
    release(process);
    irq_restore(flags);
    return 0;
}

uint64_t process_create(uint64_t mode,uint64_t cookie)
{
    return create_image(user_blob_start,(size_t)(user_blob_end-user_blob_start),mode,cookie,false);
}
uint64_t process_create_image(const void *code,size_t size)
{
    return create_image(code,size,0,0,true);
}

bool process_kill(uint64_t pid)
{
    uint64_t flags = irq_save();
    struct process *process = process_find(pid);
    bool ok = process && process != scheduler_current_process()
              && process->state == PROCESS_RUNNING;
    if (ok) { fs_close_all(process->files); scheduler_stop_process(process); process->state = PROCESS_ZOMBIE; }
    irq_restore(flags);
    return ok;
}

bool process_reap(uint64_t pid)
{
    uint64_t flags = irq_save();
    struct process *process = process_find(pid);
    bool ok = process && process->state == PROCESS_ZOMBIE
              && scheduler_detach_process(process);
    if (ok) release(process);
    irq_restore(flags);
    return ok;
}

struct frame *process_fault(struct frame *frame)
{
    struct process *process = scheduler_current_process();
    check(process && (frame->cs & 3) == 3, "user fault without process");
    process->fault_vector = frame->vector;
    process->fault_error = frame->error;
    process->fault_rip = frame->rip;
    if (frame->vector == 14)
        __asm__ volatile ("mov %%cr2,%0" : "=r"(process->fault_address));
    unsigned signal=frame->vector==6?4:(frame->vector==0||frame->vector==16||frame->vector==19)?8:
        (frame->vector==1||frame->vector==3)?5:frame->vector==17?7:11;
    if(process_signal_fault(process,signal))return frame;
    print("USER FAULT pid="); decimal(process->pid);
    print(" vector="); decimal(frame->vector);
    print(" error="); hex(frame->error);
    print(" rip=");hex(frame->rip);
    if(frame->vector==14){print(" address=");hex(process->fault_address);}
    print("\n");
    fs_close_all(process->files);
    scheduler_stop_process(process);
    process->state = PROCESS_ZOMBIE;
    /* Reclamation is deferred: this IRQ frame still occupies its kernel stack. */
    return schedule(frame);
}

/* Separate ELF64 path: page ownership is dynamic, legacy demos keep their ABI. */
#define ELF_SEGMENT_MAPPING (1u<<8)
#define IMAGE_WRITE_CEILING (1ull<<9)
#define IMAGE_EXEC_CEILING (1ull<<10)
#define IMAGE_SEGMENT_PAGE (1ull<<11)
static uintptr_t elf_page(struct process *p,unsigned capacity)
{
    if(p->page_count>=capacity)return 0;
    uintptr_t page=page_alloc();if(!page)return 0;
    p->image_pages[p->page_count++]=page;memset((void *)page,0,PAGE_SIZE);return page;
}
static uintptr_t elf_map_page(struct process *p,unsigned capacity,uint64_t address,uint32_t flags)
{
    uint64_t *table=(void *)p->root;
    for(int shift=39;shift>=21;shift-=9){
        uint64_t *e=&table[(address>>shift)&511];
        if(!(*e&1)){uintptr_t page=elf_page(p,capacity);if(!page)return 0;*e=page|7;}
        table=(void *)(uintptr_t)(*e&0x000ffffffffff000ull);
    }
    uint64_t *e=&table[(address>>12)&511];if(*e&1)return 0;
    uintptr_t page=elf_page(p,capacity);if(!page)return 0;
    *e=page|5|((flags&2)?2:0)|((flags&1)?0:NX);
    /* Only PT_LOAD segment pages may participate in image mprotect. Bootstrap,
     * stacks, native TLS and platform runtime keep their separate policies.
     * These software bits retain original rights even after PROT_NONE/R. */
    if(flags&ELF_SEGMENT_MAPPING)*e|=IMAGE_SEGMENT_PAGE|
        ((flags&2)?IMAGE_WRITE_CEILING:0)|((flags&1)?IMAGE_EXEC_CEILING:0);
    return page;
}
static struct process *stage_images(const struct elf_source *const sources[],const struct elf_plan *const plans[],unsigned images)
{
    if(!images||images>2)return 0;
    const struct elf_plan *plan=plans[0];
    uint64_t flags=irq_save();struct process *p=0;
    for(unsigned i=0;i<PROCESS_LIMIT;++i)if(processes[i].state==PROCESS_UNUSED){p=&processes[i];break;}
    if(!p){irq_restore(flags);return 0;}
    unsigned stack_pages=plan->stack_pages?plan->stack_pages:ELF_STACK_PAGES;
    if(stack_pages<ELF_STACK_PAGES||stack_pages>ELF_STARBOARD_STACK_PAGES){irq_restore(flags);return 0;}
    unsigned capacity=128+stack_pages-ELF_STACK_PAGES+(images==2);
    unsigned native_tls=0;
    for(unsigned image=0;image<images;++image){const struct elf_plan *owner=plans[image];
        if(owner->tls_size){if(++native_tls>1){irq_restore(flags);return 0;}
            capacity+=(PROCESS_THREAD_LIMIT+1u)*
                (unsigned)((owner->tls_offset+PAGE_SIZE-1)/PAGE_SIZE+1);}
        for(unsigned i=0;i<owner->count;++i){const struct elf_segment *s=&owner->segments[i];
            capacity+=(unsigned)(((s->address%PAGE_SIZE)+s->memsz+4095)/PAGE_SIZE);
        }
    }
    p->files=kmalloc(FS_HANDLES*sizeof(*p->files));
    if(!p->files)goto failed;
    memset(p->files,0,FS_HANDLES*sizeof(*p->files));
    p->image_pages=kmalloc((size_t)capacity*sizeof(uintptr_t));
    if(!p->image_pages)goto failed;
    p->image_capacity=capacity;
    p->root=elf_page(p,capacity);if(!p->root)goto failed;
    ((uint64_t *)p->root)[0]=((uint64_t *)boot_state.root)[0]&~4ull;
    for(unsigned image=0;image<images;++image){const struct elf_plan *owner=plans[image];
      const struct elf_source *source=sources[image];
      if(images==2){print("ELF pair mapping module: ");decimal(image);print("\n");}
      for(unsigned i=0;i<owner->count;++i){const struct elf_segment *s=&owner->segments[i];
        uint64_t end=s->address+s->memsz;
        for(uint64_t va=s->address&~4095ull;va<end;va+=PAGE_SIZE){
            uintptr_t page=elf_map_page(p,capacity,va,s->flags|ELF_SEGMENT_MAPPING);if(!page)goto failed;
            uint64_t begin=va>s->address?va:s->address,file_end=s->address+s->filesz;
            uint64_t stop=va+PAGE_SIZE<file_end?va+PAGE_SIZE:file_end;
            if(begin<stop&&!source->read(source->context,s->offset+begin-s->address,
               (void *)(page+begin-va),(size_t)(stop-begin)))goto failed;
        }
      }
    }
    /* Fixed top preserves legacy ELF addresses; grow downward with an unmapped
     * guard below the allocation. Starboard's 2 MiB is a budget, not a measured
     * Cobalt minimum. Image/VM/bootstrap ranges stay disjoint. */
    p->main_stack_length=(uint64_t)stack_pages*PAGE_SIZE;
    p->main_stack_base=USER_BASE+ELF_STACK_OFFSET+ELF_STACK_PAGES*PAGE_SIZE-p->main_stack_length;
    for(unsigned i=0;i<stack_pages;++i)
        if(!elf_map_page(p,capacity,p->main_stack_base+i*PAGE_SIZE,6))goto failed;
    if(!elf_map_page(p,capacity,USER_BASE+ELF_BOOT_OFFSET,5))goto failed;
    if(images==2&&!elf_map_page(p,capacity,ELF_PAIR_CONTROL_ADDRESS,6))goto failed;
    p->kernel_stack=kmalloc(16384);if(!p->kernel_stack)goto failed;
    p->kernel_top=((uintptr_t)p->kernel_stack+16384)&~15ull;
    p->frame=(struct frame *)(p->kernel_top-sizeof(struct frame));memset(p->frame,0,sizeof(*p->frame));
    p->frame->rip=plan->entry;p->frame->cs=0x2b;p->frame->ss=0x23;
    p->frame->rsp=USER_BASE+ELF_STACK_OFFSET+4*PAGE_SIZE-8;p->frame->flags=0x202;
    p->pid=next_pid++;p->state=PROCESS_LOADING;
    irq_restore(flags);return p;
failed:
    release(p);irq_restore(flags);return 0;
}
struct process *process_stage_elf(const struct elf_source *source,const struct elf_plan *plan)
{const struct elf_source *sources[]={source};const struct elf_plan *plans[]={plan};return stage_images(sources,plans,1);}
struct process *process_stage_elf_pair(const struct elf_source *const sources[2],const struct elf_plan *const plans[2])
{return stage_images(sources,plans,2);}

bool process_map_runtime(struct process *p)
{
    if(!p||p->state!=PROCESS_LOADING)return false;
    size_t size=PLATFORM_RUNTIME_BYTES;
    if(!size||size>PROCESS_RUNTIME_TLS_BASE-PROCESS_RUNTIME_BASE)return false;
    unsigned capacity=p->image_capacity;
    if(!capacity)return false;
    /* Decode and verify privately, before mapping any executable page. The
     * generated raw size is independent of the untrusted packed header. */
    unsigned char *raw=kmalloc(size);
    if(!raw)return false;
#if PROCESS_RUNTIME_EXTERNAL
    struct asset_file file;
    static const unsigned char expected_hash[32]=PLATFORM_RUNTIME_PACKED_SHA256;
    unsigned char digest[32];
    size_t packed_size=PLATFORM_RUNTIME_PACKED_BYTES;
    /* Pin this build's exact packed identity; the asset cannot substitute a
     * runtime with different exported addresses, TLS ABI or permissions. */
    unsigned char *packed=0;
    bool valid=packed_size>=16&&packed_size<=65536+65536/128+16&&
        !asset_runtime_open(&file)&&file.size==packed_size;
    if(valid){packed=kmalloc(packed_size);valid=packed!=0;}
    if(valid)valid=asset_read(&file,0,packed,packed_size)&&
        !mbedtls_sha256(packed,packed_size,digest,0);
    if(valid){unsigned difference=0;
        for(unsigned i=0;i<sizeof(digest);++i)difference|=digest[i]^expected_hash[i];
        valid=!difference&&runtime_unpack(packed,packed_size,raw,size);}
    if(packed)kfree(packed);
    if(!valid){print("Runtime asset: unavailable or invalid build identity\n");
        kfree(raw);return false;}
#else
    if(!runtime_unpack(platform_runtime_start,
            (size_t)(platform_runtime_end-platform_runtime_start),raw,size)){
        kfree(raw);return false;
    }
#endif
    for(size_t offset=0;offset<size;offset+=PAGE_SIZE){
        uintptr_t page=elf_map_page(p,capacity,PROCESS_RUNTIME_BASE+offset,5);
        if(!page){kfree(raw);return false;}
        size_t count=size-offset;if(count>PAGE_SIZE)count=PAGE_SIZE;
        memcpy((void *)page,raw+offset,count);
    }
    kfree(raw);
    /* One private errno page for the main task and every possible worker. */
    for(unsigned i=0;i<=PROCESS_THREAD_LIMIT;++i){
        uint64_t address=PROCESS_RUNTIME_TLS_BASE+(uint64_t)i*PAGE_SIZE;
        uintptr_t page=elf_map_page(p,capacity,address,6);
        if(!page)return false;
        /* errno0..3; GS slot index padding4..7; launcher owns offset56. */
        *(uint32_t *)(page+4)=i;
    }
    return true;
}

static uint64_t *image_leaf(struct process *p,uint64_t address)
{
    if(!p||p->state!=PROCESS_LOADING||address<USER_BASE||address>=USER_BASE+(1ull<<30))return 0;
    uint64_t *t=(void *)p->root;
    for(int shift=39;shift>=21;shift-=9){uint64_t e=t[(address>>shift)&511];
        if((e&5)!=5||(e&128))return 0;t=(void *)(uintptr_t)(e&0x000ffffffffff000ull);}
    uint64_t *e=&t[(address>>12)&511];return (*e&5)==5?e:0;
}
bool process_image_copy(struct process *p,uint64_t address,void *buffer,size_t size,bool write)
{
    if(address>UINT64_MAX-size)return false;
    uint8_t *bytes=buffer;
    while(size){uint64_t *e=image_leaf(p,address);if(!e||(write&&!(*e&2)))return false;
        size_t n=PAGE_SIZE-address%PAGE_SIZE;if(n>size)n=size;
        void *physical=(void *)(uintptr_t)((*e&0x000ffffffffff000ull)+address%PAGE_SIZE);
        if(write)memcpy(physical,bytes,n);else memcpy(bytes,physical,n);
        bytes+=n;address+=n;size-=n;
    }return true;
}
bool process_image_readonly(struct process *p,uint64_t address,uint64_t size)
{
    if(address>UINT64_MAX-size)return false;
    for(uint64_t a=address&~4095ull;a<address+size;a+=PAGE_SIZE){uint64_t *e=image_leaf(p,a);if(!e)return false;*e&=~(2ull|IMAGE_WRITE_CEILING);}
    return true; /* This staged CR3 has never been active: no stale TLB entries. */
}

static bool vm_range(uint64_t address,size_t length)
{
    return length&&!(address&(PAGE_SIZE-1))&&!(length&(PAGE_SIZE-1))&&
           ((length<=PROCESS_VM_END-PROCESS_VM_BASE&&address>=PROCESS_VM_BASE&&address<=PROCESS_VM_END-length)||
            (length<=PROCESS_VM_MID_END-PROCESS_VM_MID_BASE&&address>=PROCESS_VM_MID_BASE&&address<=PROCESS_VM_MID_END-length)||
            (length<=PROCESS_VM_HIGH_END-PROCESS_VM_HIGH_BASE&&address>=PROCESS_VM_HIGH_BASE&&address<=PROCESS_VM_HIGH_END-length));
}
static uintptr_t vm_new_page(struct process *p)
{
    uintptr_t page=page_alloc();
    if(!page)return 0;
    if(!vm_track(p,page)){page_free(page);return 0;}
    memset((void *)page,0,PAGE_SIZE);return page;
}
static uint64_t *vm_leaf(struct process *p,uint64_t address,bool create)
{
    uint64_t *table=(void *)p->root;
    for(int shift=39;shift>=21;shift-=9){
        uint64_t *entry=&table[(address>>shift)&511];
        if(!(*entry&1)){
            if(!create)return 0;
            uintptr_t page=vm_new_page(p);if(!page)return 0;
            *entry=page|7;
        }
        if((*entry&5)!=5||(*entry&128))return 0;
        table=(void *)(uintptr_t)(*entry&0x000ffffffffff000ull);
    }
    return &table[(address>>12)&511];
}
static void vm_flush(uint64_t address)
{
#ifdef PROCESS_VM_UNIT
    (void)address; /* Host contract tests cannot execute privileged INVLPG. */
#else
    __asm__ volatile("invlpg (%0)"::"r"(address):"memory");
#endif
}
/* Skip absent page-table subtrees rather than walking billions of reserved
 * pages. next is always greater than address and callers cap it at their end. */
static uint64_t *vm_leaf_span(struct process *p,uint64_t address,uint64_t *next)
{
    uint64_t *table=(void *)p->root;
    for(int shift=39;shift>=21;shift-=9){
        uint64_t entry=table[(address>>shift)&511];
        if((entry&5)!=5||(entry&128)){
            *next=(address|( (1ull<<shift)-1))+1;return 0;
        }
        table=(void *)(uintptr_t)(entry&0x000ffffffffff000ull);
    }
    *next=address+PAGE_SIZE;return &table[(address>>12)&511];
}
static void vm_prune_path(struct process *p,uint64_t address)
{
    uint64_t *table=(void *)p->root,*parents[3];uintptr_t pages[3];
    unsigned depth=0;
    for(int shift=39;shift>=21;shift-=9){
        uint64_t *entry=&table[(address>>shift)&511];
        if((*entry&5)!=5||(*entry&128))break;
        parents[depth]=entry;pages[depth]=*entry&0x000ffffffffff000ull;++depth;
        table=(void *)pages[depth-1];
    }
    while(depth){--depth;uintptr_t page=pages[depth];
        if(!vm_owned(p,page))break;bool empty=true;uint64_t *entries=(void *)page;
        for(unsigned i=0;i<512;++i)if(entries[i]){empty=false;break;}
        if(!empty)break;*parents[depth]=0;vm_forget(p,page);page_free(page);
    }
}
static void vm_drop_pages(struct process *p,uint64_t address,size_t length,bool staged_only)
{
    uint64_t end=address+length,last_removed=0;bool pending_prune=false;
    for(uint64_t a=address;a<end;){uint64_t next,*entry=vm_leaf_span(p,a,&next);
        if(entry&&(*entry&1)&&(!staged_only||(*entry&(1ull<<9)))){
            uintptr_t page=*entry&0x000ffffffffff000ull;
            *entry=0;vm_flush(a);vm_forget(p,page);page_free(page);
            last_removed=a;pending_prune=true;
        }
        a=next<end?next:end;
        /* A leaf table covers 2 MiB. It cannot become empty until the last
         * removed leaf in this span, so avoid rescanning it after every page.
         * Data-page release/TLB invalidation stay immediate; prune before
         * crossing the span (or returning), including staged rollback. */
        if(pending_prune&&(a==end||(a>>21)!=(last_removed>>21))){
            vm_prune_path(p,last_removed);pending_prune=false;
        }
    }
}
#define VM_FILE_PAGE (1ull<<10)
#define VM_FILE_SHARED_PAGE (1ull<<11)
static int vm_commit(struct process *p,uint64_t address,size_t length,unsigned prot)
{
    uint64_t end=address+length;
    if(prot){
        size_t resident=0;
        for(uint64_t a=address;a<end;){uint64_t next,*entry=vm_leaf_span(p,a,&next);
            if(entry&&(*entry&1))++resident;a=next<end?next:end;
        }
        size_t needed=length/PAGE_SIZE-resident;
        if(needed>pages_free()){
            print("VM commit ENOMEM needed-pages=");decimal(needed);
            print(" free-pages=");decimal(pages_free());print("\n");return -12;
        }
        for(uint64_t a=address;a<end;a+=PAGE_SIZE){
            uint64_t *entry=vm_leaf(p,a,true);
            if(!entry){vm_drop_pages(p,address,length,true);vm_prune_path(p,a);return -12;}
            if(*entry&1)continue;
            uintptr_t page=vm_new_page(p);
            if(!page){vm_drop_pages(p,address,length,true);vm_prune_path(p,a);return -12;}
            /* Stage inaccessible pages until the whole commit succeeds.
             * Rollback touches only new pages, never earlier contents/rights. */
            *entry=page|1|NX|(1ull<<9);
        }
    }
    for(uint64_t a=address;a<end;){uint64_t next,*entry=vm_leaf_span(p,a,&next);
        if(entry&&(*entry&1)){
            uintptr_t page=*entry&0x000ffffffffff000ull;
            uint64_t backing=*entry&(VM_FILE_PAGE|VM_FILE_SHARED_PAGE);
            *entry=page|backing|1|((prot&PROCESS_PROT_READ)?4:0)|((prot&PROCESS_PROT_WRITE)?2:0)|
                   ((prot&PROCESS_PROT_EXEC)?0:NX);vm_flush(a);
        }
        a=next<end?next:end;
    }
    return 0;
}

int process_vm_unmap(struct process *p,uint64_t address,size_t length)
{
    if(!vm_range(address,length))return -22;
    if(!vm_areas_remove(p->vm_areas,address,address+length))return -12;
    vm_drop_pages(p,address,length,false);
    if(p->vm_areas&&!p->vm_areas->count){kfree(p->vm_areas);p->vm_areas=0;}
    return 0;
}

static int image_protect(struct process *p,uint64_t address,size_t length,unsigned prot)
{
    if(!p->image_pages||address<USER_BASE||address>=PROCESS_VM_BASE||
       length>PROCESS_VM_BASE-address)return -12;
    /* Preflight every page before changing any permission. Never allocate or
     * accept a page solely because it happens to be mapped in this CR3. */
    for(uint64_t a=address;a<address+length;a+=PAGE_SIZE){uint64_t next,*entry=vm_leaf_span(p,a,&next);
        if(!entry||!(*entry&1)||!(*entry&IMAGE_SEGMENT_PAGE))return -12;
        if(((prot&PROCESS_PROT_WRITE)&&!(*entry&IMAGE_WRITE_CEILING))||
           ((prot&PROCESS_PROT_EXEC)&&!(*entry&IMAGE_EXEC_CEILING)))return -13;
    }
    for(uint64_t a=address;a<address+length;a+=PAGE_SIZE){uint64_t next,*entry=vm_leaf_span(p,a,&next);
        *entry=(*entry&~(6ull|NX))|((prot&PROCESS_PROT_READ)?4ull:0)|
               ((prot&PROCESS_PROT_WRITE)?2ull:0)|((prot&PROCESS_PROT_EXEC)?0:NX);
        vm_flush(a);
    }
    return 0;
}
int process_vm_protect(struct process *p,uint64_t address,size_t length,unsigned prot)
{
    if(!length||(address&(PAGE_SIZE-1))||(length&(PAGE_SIZE-1))||
       address>UINT64_MAX-length||(prot&~7u)||
       ((prot&PROCESS_PROT_WRITE)&&(prot&PROCESS_PROT_EXEC)))return -22;
    if(!vm_range(address,length))return image_protect(p,address,length,prot);
    if(!vm_areas_cover(p->vm_areas,address,address+length))return -12;
    /* Preflight mixed ranges before any permission changes. */
    for(uint64_t a=address;a<address+length;){uint64_t next,*entry=vm_leaf_span(p,a,&next);
        if(entry&&(*entry&VM_FILE_PAGE)){
            if(prot&PROCESS_PROT_EXEC)return -95;
            if((prot&PROCESS_PROT_WRITE)&&(*entry&VM_FILE_SHARED_PAGE))return -13;
        }
        a=next<address+length?next:address+length;
    }
    return vm_commit(p,address,length,prot);
}

int64_t process_vm_map(struct process *p,uint64_t hint,size_t length,unsigned prot)
{
    if(!length||length>PROCESS_VM_HIGH_END-PROCESS_VM_HIGH_BASE||length>SIZE_MAX-(PAGE_SIZE-1) ||
       (prot&~7u)||((prot&PROCESS_PROT_WRITE)&&(prot&PROCESS_PROT_EXEC)))return -22;
    length=(length+PAGE_SIZE-1)&~(size_t)(PAGE_SIZE-1);
    uint64_t address=hint;
    if(hint){if(!vm_range(hint,length))return -22;
        if(vm_areas_overlap(p->vm_areas,hint,hint+length))return -17;
    }else{
        address=vm_areas_gap(p->vm_areas,PROCESS_VM_BASE,PROCESS_VM_END,length);
        if(!address)address=vm_areas_gap(p->vm_areas,PROCESS_VM_MID_BASE,PROCESS_VM_MID_END,length);
        if(!address)address=vm_areas_gap(p->vm_areas,PROCESS_VM_HIGH_BASE,PROCESS_VM_HIGH_END,length);
        if(!address)return -12;
    }
    if(!p->vm_areas){p->vm_areas=kmalloc(sizeof(*p->vm_areas));if(!p->vm_areas)return -12;
        memset(p->vm_areas,0,sizeof(*p->vm_areas));}
    if(!vm_areas_add(p->vm_areas,address,address+length)){
        print("VM reserve ENOMEM areas=");decimal(p->vm_areas->count);
        print(" limit=");decimal(VM_AREA_LIMIT);print(" pages=");decimal(length/PAGE_SIZE);print("\n");return -12;
    }
    int result=vm_commit(p,address,length,prot);
    if(result){check(!process_vm_unmap(p,address,length),"VM reservation rollback");return result;}
    return (int64_t)address;
}

int64_t process_vm_map_hint(struct process *p,uint64_t hint,size_t length,unsigned prot)
{
    if(!length||length>SIZE_MAX-(PAGE_SIZE-1))return -22;
    size_t bytes=(length+PAGE_SIZE-1)&~(size_t)(PAGE_SIZE-1);
    hint&=~(uint64_t)(PAGE_SIZE-1);
    /* Nonfixed hints are best effort, never replacement. Unusable/colliding
     * hints fall back to bounded pools. Exact/NOREPLACE use the old syscall. */
    if(hint&&vm_range(hint,bytes)&&!vm_areas_overlap(p->vm_areas,hint,hint+bytes))
        return process_vm_map(p,hint,length,prot);
    return process_vm_map(p,0,length,prot);
}
int64_t process_vm_map_fixed(struct process *p,uint64_t address,size_t length,unsigned prot)
{
    if(!vm_range(address,length)||(prot&~7u)||
       ((prot&PROCESS_PROT_WRITE)&&(prot&PROCESS_PROT_EXEC)))return -22;
    int result=process_vm_unmap(p,address,length);if(result)return result;
    return process_vm_map(p,address,length,prot);
}

bool process_vm_mapped(struct process *p,uint64_t address,size_t length)
{
    return vm_range(address,length)&&vm_areas_cover(p->vm_areas,address,address+length);
}
bool process_vm_resident(struct process *p,uint64_t address)
{uint64_t *entry=vm_leaf(p,address,false);return entry&&(*entry&1);}

static int64_t map_shared_backing(struct process *p,const struct vm_file_request *r,size_t length,unsigned rights)
{
    /* Reserve VA only. A shared alias must not require a temporary private
     * copy of every backing frame before installing the real references. */
    int64_t mapped=(r->flags&VM_FILE_FIXED)?
        process_vm_map_fixed(p,r->address,length,0):
        process_vm_map(p,r->address,length,0);
    if(mapped<0)return mapped;
    int error=0;
    for(size_t offset=0;offset<length;offset+=PAGE_SIZE){
        uint64_t *entry=vm_leaf(p,(uint64_t)mapped+offset,true);
        if(!entry){error=-12;break;}
        uintptr_t shared=fs_shared_page(p->files,r->descriptor,(size_t)r->offset+offset);
        if(!shared){error=-12;break;}
        check(!(*entry&1),"shared mapping reserved leaf");
        if(!vm_track(p,shared)){page_free(shared);error=-12;break;}
        /* Inaccessible until all references exist and protection succeeds. */
        *entry=shared|1|NX|VM_FILE_PAGE|((rights&FS_WRITE)?0:VM_FILE_SHARED_PAGE);
        vm_flush((uint64_t)mapped+offset);
    }
    if(!error)error=process_vm_protect(p,(uint64_t)mapped,length,r->protection);
    if(error){check(!process_vm_unmap(p,(uint64_t)mapped,length),"shared mapping rollback");return error;}
    return mapped;
}
/* Immutable backing cannot change; readonly shared copies observe identical
 * bytes. Private writable copies are owned and never written back. No mutable
 * shared/COW/demand-SIGBUS claim. Atomic syscall on this single-CPU kernel. */
int64_t process_vm_file(struct process *p,const struct vm_file_request *r)
{
    if(!r->length||r->length>SIZE_MAX-(PAGE_SIZE-1)||
       (r->offset&(PAGE_SIZE-1))||r->offset>INT64_MAX||
       (r->protection&~7u)||(r->flags&~(VM_FILE_FIXED|VM_FILE_SHARED)))return -22;
    size_t length=(r->length+PAGE_SIZE-1)&~(size_t)(PAGE_SIZE-1);
    if(r->address&&!vm_range(r->address,length))return -22;
    if((r->flags&VM_FILE_FIXED)&&!r->address)return -22;
    struct fs_entry info;int error=fs_fstat(p->files,r->descriptor,&info);
    if(error)return error;
    if(info.type!=FS_FILE||!(info.reserved&3))return -95;
    if(r->protection&PROCESS_PROT_EXEC)return -95;
    unsigned rights=0;
    if(info.reserved&2){error=fs_shared_rights(p->files,r->descriptor,&rights);if(error)return error;
        if(!(rights&FS_READ))return -13;
        if((r->flags&VM_FILE_SHARED)&&(r->protection&PROCESS_PROT_WRITE)&&!(rights&FS_WRITE))return -13;
    }else if((r->flags&VM_FILE_SHARED)&&(r->protection&PROCESS_PROT_WRITE))return -13;
    /* Reject whole pages beyond EOF explicitly until SIGBUS is supported. */
    if(r->offset>=info.size||info.size-r->offset>SIZE_MAX-(PAGE_SIZE-1))return -95;
    uint64_t extent=info.size-r->offset;
    if(length>((extent+PAGE_SIZE-1)&~(uint64_t)(PAGE_SIZE-1)))return -95;
    if((info.reserved&2)&&(r->flags&VM_FILE_SHARED))return map_shared_backing(p,r,length,rights);
    int64_t saved=fs_seek(p->files,r->descriptor,0,1);if(saved<0)return saved;
    int64_t mapped=(r->flags&VM_FILE_FIXED)?
        process_vm_map_fixed(p,r->address,length,PROCESS_PROT_READ):
        process_vm_map(p,r->address,length,PROCESS_PROT_READ);
    if(mapped<0)return mapped;
    int64_t seek=fs_seek(p->files,r->descriptor,(int64_t)r->offset,0);
    if(seek<0)error=(int)seek;
    /* Final page includes real backing bytes past the unrounded request. */
    size_t count=length<extent?length:(size_t)extent;
    for(size_t done=0;!error&&done<count;){
        uint64_t *entry=vm_leaf(p,(uint64_t)mapped+done,false);
        if(!entry||!(*entry&1)){error=-12;break;}
        size_t chunk=PAGE_SIZE;if(chunk>count-done)chunk=count-done;
        int64_t read=fs_read(p->files,r->descriptor,(void *)(uintptr_t)(*entry&0x000ffffffffff000ull),chunk);
        if(read!=(int64_t)chunk){error=read<0?(int)read:-5;break;}done+=chunk;
    }
    int64_t restored=fs_seek(p->files,r->descriptor,saved,0);
    if(!error&&restored<0)error=(int)restored;
    if(!error)error=process_vm_protect(p,(uint64_t)mapped,length,r->protection);
    if(error){check(!process_vm_unmap(p,(uint64_t)mapped,length),"file VM rollback");return error;}
    for(uint64_t a=(uint64_t)mapped;a<(uint64_t)mapped+length;a+=PAGE_SIZE){
        uint64_t *entry=vm_leaf(p,a,false);
        *entry|=VM_FILE_PAGE|((r->flags&VM_FILE_SHARED)?VM_FILE_SHARED_PAGE:0);
    }
    return mapped;
}

int process_vm_advise(struct process *p,uint64_t address,size_t length,unsigned advice)
{
    if(!process_vm_mapped(p,address,length))return -12;
    if(advice!=0&&advice!=1&&advice!=2&&advice!=3&&advice!=4&&
       advice!=8&&advice!=16&&advice!=17)return -22;
    /* No anonymous zeroing of file data. Preflight before any mixed mutation. */
    if(advice==4||advice==8)for(uint64_t a=address;a<address+length;){
        uint64_t next,*entry=vm_leaf_span(p,a,&next);
        if(entry&&(*entry&VM_FILE_PAGE))return -95;
        a=next<address+length?next:address+length;
    }
    if(advice==4||advice==8)for(uint64_t a=address;a<address+length;){
        uint64_t next,*entry=vm_leaf_span(p,a,&next);
        if(entry&&(*entry&1))memset((void *)(uintptr_t)(*entry&0x000ffffffffff000ull),0,PAGE_SIZE);
        a=next<address+length?next:address+length;
    }
    return 0;
}

uint64_t process_errno_location(struct process *p)
{
    if(!p)return 0;
    struct process_thread *thread=scheduler_current_thread();
    if(!thread)return PROCESS_RUNTIME_TLS_BASE;
    for(unsigned i=0;i<PROCESS_THREAD_LIMIT;++i)
        if(&p->threads[i]==thread)return PROCESS_RUNTIME_TLS_BASE+(uint64_t)(i+1)*PAGE_SIZE;
    return 0;
}

bool process_user_access(struct process *p,uint64_t address,size_t length,unsigned prot)
{
    if(!length||address>UINT64_MAX-(length-1)||address>=(1ull<<47))return false;
    while(length){
        uint64_t *entry=vm_leaf(p,address,false);
        if(!entry||(*entry&5)!=5)return false;
        if((prot&PROCESS_PROT_WRITE)&&!(*entry&2))return false;
        if((prot&PROCESS_PROT_EXEC)&&(*entry&NX))return false;
        size_t count=PAGE_SIZE-(address&(PAGE_SIZE-1));
        if(count>length)count=length;
        address+=count;length-=count;
    }
    return true;
}

bool process_copy_user(struct process *p,uint64_t address,void *buffer,size_t length,bool write)
{
    if(!process_user_access(p,address,length,write?PROCESS_PROT_WRITE:PROCESS_PROT_READ))return false;
    uint8_t *bytes=buffer;
    while(length){uint64_t *e=vm_leaf(p,address,false);
        size_t count=PAGE_SIZE-address%PAGE_SIZE;if(count>length)count=length;
        void *physical=(void *)(uintptr_t)((*e&0x000ffffffffff000ull)+address%PAGE_SIZE);
        if(write)memcpy(physical,bytes,count);else memcpy(bytes,physical,count);
        address+=count;bytes+=count;length-=count;
    }
    return true;
}

/* TLS mappings are process-owned bounded slots, never anonymous mmap regions.
 * Save a relocated immutable template before constructors; each worker gets
 * a fresh copy, not the main thread's possibly modified values. */
static void tls_write(struct process *p,uint64_t address,const void *source,size_t length)
{
    const unsigned char *bytes=source;
    while(length){uint64_t *e=vm_leaf(p,address,false);check(e&&(*e&7)==7,"TLS writable mapping");
        size_t n=PAGE_SIZE-address%PAGE_SIZE;if(n>length)n=length;
        memcpy((void *)(uintptr_t)((*e&0x000ffffffffff000ull)+address%PAGE_SIZE),bytes,n);
        address+=n;bytes+=n;length-=n;
    }
}
static uint64_t tls_reset(struct process *p,unsigned slot,bool initialize)
{
    uint64_t base=PROCESS_ELF_TLS_BASE+(uint64_t)slot*p->tls_stride;
    uint64_t mapped=p->tls_stride-PAGE_SIZE,tp=base+mapped-PAGE_SIZE;
    for(uint64_t a=base;a<base+mapped;a+=PAGE_SIZE){uint64_t *e=vm_leaf(p,a,false);
        check(e&&(*e&7)==7,"TLS slot mapping");memset((void *)(uintptr_t)(*e&0x000ffffffffff000ull),0,PAGE_SIZE);}
    if(initialize){
        tls_write(p,tp-p->tls_offset,p->tls_template,p->tls_size);
        uint64_t tcb[]={tp,0,tp-p->tls_offset,p->tls_size};tls_write(p,tp,tcb,sizeof(tcb));
    }
    uint64_t active=initialize?tp:0;
    tls_write(p,PROCESS_RUNTIME_TLS_BASE+(uint64_t)slot*PAGE_SIZE+48,&active,8);
    return tp;
}
bool process_install_tls(struct process *p,const struct elf_plan *plan)
{
    if(!plan->tls_size)return true;
    p->tls_template=kmalloc(plan->tls_size);if(!p->tls_template)return false;
    memset(p->tls_template,0,plan->tls_size);
    if(!process_image_copy(p,plan->tls_address,p->tls_template,plan->tls_filesz,false))return false;
    p->tls_offset=plan->tls_offset;p->tls_size=plan->tls_size;
    p->tls_stride=((p->tls_offset+PAGE_SIZE-1)&~(PAGE_SIZE-1ull))+2*PAGE_SIZE;
    for(unsigned slot=0;slot<=PROCESS_THREAD_LIMIT;++slot){
        uint64_t base=PROCESS_ELF_TLS_BASE+(uint64_t)slot*p->tls_stride;
        for(uint64_t a=base;a<base+p->tls_stride-PAGE_SIZE;a+=PAGE_SIZE)
            if(!elf_map_page(p,p->image_capacity,a,6))return false;
        if(!slot)p->main_fs_base=tls_reset(p,slot,true);
    }
    return true;
}
static void thread_release(struct process *p,struct process_thread *thread)
{
    uint64_t user_stack=thread->user_stack;size_t user_length=thread->user_stack_length;
    check(thread->state==PROCESS_THREAD_ZOMBIE,"release live thread");
    check(scheduler_detach_thread(p,thread),"detach user thread");
    process_signal_reset_thread(p,thread,false);
    if(p->tls_size)tls_reset(p,(unsigned)(thread-p->threads)+1,false);
    kfree(thread->kernel_stack);memset(thread,0,sizeof(*thread));
    if(user_stack)check(!process_vm_unmap(p,user_stack,user_length),"detached stack release");
}

void process_cleanup_detached(struct process *p)
{
    struct process_thread *current=scheduler_current_thread();
    for(unsigned i=0;i<PROCESS_THREAD_LIMIT;++i)
        if(&p->threads[i]!=current&&p->threads[i].state==PROCESS_THREAD_ZOMBIE&&p->threads[i].detached)
            thread_release(p,&p->threads[i]);
}
int64_t process_thread_create(struct process *p,uint64_t entry,uint64_t arg,
                              uint64_t stack,uint64_t tls)
{
    process_cleanup_detached(p);
    if(!process_user_access(p,entry,1,PROCESS_PROT_EXEC)||stack<8||
       !process_user_access(p,stack-8,8,PROCESS_PROT_WRITE)||
       (tls&&!process_user_access(p,tls,8,PROCESS_PROT_READ)))return -14;
    struct process_thread *thread=0;
    for(unsigned i=0;i<PROCESS_THREAD_LIMIT;++i)
        if(p->threads[i].state==PROCESS_THREAD_UNUSED){thread=&p->threads[i];break;}
    if(!thread)return -11;
    if(p->tls_size&&tls)return -22; /* Installed module owns FS, no replacement. */
    thread->kernel_stack=kmalloc(16384);
    if(!thread->kernel_stack)return -12;
    thread->kernel_top=((uintptr_t)thread->kernel_stack+16384)&~15ull;
    thread->frame=(struct frame *)(thread->kernel_top-sizeof(struct frame));
    memset(thread->frame,0,sizeof(*thread->frame));
    thread->frame->rip=entry;thread->frame->cs=0x2b;thread->frame->ss=0x23;
    thread->frame->rsp=(stack&~15ull)-8;thread->frame->rdi=arg;thread->frame->flags=0x202;
    thread->fs_base=p->tls_size?tls_reset(p,(unsigned)(thread-p->threads)+1,true):tls;
    thread->tid=(p->pid<<32)|++p->next_tid;
    thread->state=PROCESS_THREAD_RUNNING;
    process_signal_reset_thread(p,thread,true);
    if(!scheduler_attach_thread(p,thread)){
        if(p->tls_size)tls_reset(p,(unsigned)(thread-p->threads)+1,false);
        kfree(thread->kernel_stack);memset(thread,0,sizeof(*thread));return -11;
    }
    return (int64_t)thread->tid;
}

struct frame *process_thread_exit(struct process *p,struct frame *frame,int32_t status)
{
    struct process_thread *thread=scheduler_current_thread();
    if(!thread){frame->rax=(uint64_t)-1;return frame;}
    process_signal_reset_thread(p,thread,false);
    thread->exit_status=status;thread->state=PROCESS_THREAD_ZOMBIE;
    check(scheduler_stop_thread(p,thread),"stop current user thread");
    return schedule(frame);
}

int process_thread_reap(struct process *p,uint64_t tid,int32_t *status)
{
    for(unsigned i=0;i<PROCESS_THREAD_LIMIT;++i){
        struct process_thread *thread=&p->threads[i];
        if(thread->state==PROCESS_THREAD_UNUSED||thread->tid!=tid)continue;
        if(thread->detached)return -22;
        if(thread->state!=PROCESS_THREAD_ZOMBIE)return -11;
        *status=thread->exit_status;thread_release(p,thread);return 0;
    }
    return -3;
}

int process_thread_detach(struct process *p,uint64_t tid,uint64_t stack,size_t length)
{
    if(!vm_range(stack,length)||!process_vm_mapped(p,stack,length))return -22;
    for(unsigned i=0;i<PROCESS_THREAD_LIMIT;++i){
        struct process_thread *thread=&p->threads[i];
        if(thread->state==PROCESS_THREAD_UNUSED||thread->tid!=tid)continue;
        if(thread->detached)return -22;
        thread->detached=true;thread->user_stack=stack;thread->user_stack_length=length;
        if(thread->state==PROCESS_THREAD_ZOMBIE)thread_release(p,thread);
        return 0;
    }
    return -3;
}

struct frame *process_wait(struct process *p,struct frame *frame,
                           uint64_t address,uint64_t deadline)
{
    uint64_t value,*entry=vm_leaf(p,address,false);
    if((address&7)||!process_user_access(p,address,8,PROCESS_PROT_READ)||!entry){
        frame->rax=(uint64_t)-14;return frame;
    }
    memcpy(&value,(void *)(uintptr_t)((*entry&0x000ffffffffff000ull)+
           (address&(PAGE_SIZE-1))),8);
    if(value!=frame->rsi){frame->rax=(uint64_t)-11;return frame;}
    frame->rax=0;return scheduler_wait(frame,p,address,deadline);
}

unsigned process_wake(struct process *p,uint64_t address,unsigned count)
{
    if((address&7)||!process_user_access(p,address,8,PROCESS_PROT_READ))return 0;
    return scheduler_wake(p,address,count);
}
bool process_notify(struct process *p,uint64_t address)
{
    /* IRQ/syscall context: mappings cannot change during validation + update.
     * Never dereference a VA in whichever unrelated CR3 currently runs. */
    if(!p||p->state!=PROCESS_RUNNING||(address&7)||
       !process_user_access(p,address,8,PROCESS_PROT_WRITE))return false;
    uint64_t *entry=vm_leaf(p,address,false);
    uint64_t *value=(void *)(uintptr_t)((*entry&0x000ffffffffff000ull)+(address&(PAGE_SIZE-1)));
    ++*value;scheduler_wake(p,address,~0u);return true;
}
void process_discard_elf(struct process *p)
{
    uint64_t f=irq_save();check(p&&p->state==PROCESS_LOADING,"discard staged ELF");release(p);irq_restore(f);
}
static char *window_dimension(char *destination,uint32_t value)
{
    char reversed[10];unsigned count=0;
    do{reversed[count++]=(char)('0'+value%10);value/=10;}while(value);
    while(count)*destination++=reversed[--count];return destination;
}
static bool start_images(struct process *p,const struct elf_start *start,const struct elf_start *dependency)
{
    extern const unsigned char elf_start_code[],elf_start_end[];
    extern const unsigned char elf_pair_start_code[],elf_pair_start_end[];
    uint64_t f=irq_save();
    if(!p||!start||p->state!=PROCESS_LOADING){irq_restore(f);return false;}
    uint64_t *e=image_leaf(p,USER_BASE+ELF_BOOT_OFFSET);check(e!=0,"ELF bootstrap map");
    const unsigned char *code=dependency?elf_pair_start_code:elf_start_code;
    size_t size=dependency?(size_t)(elf_pair_start_end-elf_pair_start_code):(size_t)(elf_start_end-elf_start_code);
    check(size<=512,"ELF bootstrap size");
    unsigned char *boot=(void *)(uintptr_t)(*e&0x000ffffffffff000ull);
    memcpy(boot,code,size);
    if(start->name){
        size_t n=0,u=0;while(n<=64&&start->name[n])++n;
        if(start->url)while(u<=1024&&start->url[u])++u;
        if(!n||n>64||u>1024){irq_restore(f);return false;}
        uint64_t base=USER_BASE+ELF_BOOT_OFFSET;
        uint64_t *args=(void *)(boot+ELF_ARGV_OFFSET);char *name=(void *)(boot+ELF_NAME_OFFSET),*url=name+n+1;
        memcpy(name,start->name,n+1);args[0]=base+ELF_NAME_OFFSET;
        for(unsigned i=1;i<ELF_ARGV_SLOTS;++i)args[i]=0;
        p->frame->rdi=1;p->frame->rsi=base+512;
        if(start->url){memcpy(url,"--url=",6);memcpy(url+6,start->url,u+1);
            args[1]=base+(uint64_t)(url-(char *)boot);p->frame->rdi=2;
            p->frame->r10=args[1]+6;}
        /* The pinned Cobalt shell otherwise requests1920x1080 regardless of
         * the native display. Configure its documented WxH switch, keeping
         * requested size, returned size and real backing surface identical.
         * These argv strings stay in the existing readonly owned boot page. */
        struct native_ui_info display;
        if(start->handler&&!native_ui_query(p,0,&display)&&display.width&&display.height){
            char *option=start->url?url+u+7:url;
            static const char prefix[]="--content-shell-host-window-size=";
            memcpy(option,prefix,sizeof(prefix)-1);
            char *end=window_dimension(option+sizeof(prefix)-1,display.width);
            *end++='x';end=window_dimension(end,display.height);*end=0;
            args[p->frame->rdi++]=base+(uint64_t)(option-(char *)boot);
            if(start->native_browser){
                /* Explicit native Cobalt profile: interpreter-only JS keeps
                 * W^X intact. HTTPS remains enforced, including test routes. */
                static const char js[]="--js-flags=--jitless";
                static const char https[]="--https-enforcement=true";
                char *next=end+1;memcpy(next,js,sizeof(js));
                args[p->frame->rdi++]=base+(uint64_t)(next-(char *)boot);
                next+=sizeof(js);memcpy(next,https,sizeof(https));
                args[p->frame->rdi++]=base+(uint64_t)(next-(char *)boot);
                next+=sizeof(https);
                if(start->native_trace){
                    static const char logging[]="--enable-logging=stderr",verbose[]="--v=1";
                    /* Normal5+trace2+optional route1 fits8 argv and NULL slot. */
                    _Static_assert(ELF_ARGV_SLOTS>=9,"native trace argv/null bound");
                    memcpy(next,logging,sizeof(logging));args[p->frame->rdi++]=base+(uint64_t)(next-(char *)boot);
                    next+=sizeof(logging);memcpy(next,verbose,sizeof(verbose));
                    args[p->frame->rdi++]=base+(uint64_t)(next-(char *)boot);next+=sizeof(verbose);
                }
                if(start->host_mapping){
                    if(!elf_host_rule(start->host_mapping,next,ELF_HOST_RULE_SIZE)){
                        irq_restore(f);return false;}
                    args[p->frame->rdi++]=base+(uint64_t)(next-(char *)boot);
                }
            }
        }
        p->frame->rcx=start->handler;p->frame->r8=start->preflight;p->frame->r9=start->sabi;
    }
    p->frame->r12=start->array;p->frame->r13=start->count;
    p->frame->r14=start->entry?start->entry:p->frame->rip;p->frame->r15=start->init;
    p->frame->rbx=start->fini;p->frame->rbp=start->fini_array;p->frame->rdx=start->fini_count;
    p->frame->r11=image_leaf(p,PROCESS_RUNTIME_TLS_BASE)?PROCESS_RUNTIME_TLS_BASE:0;
    if(dependency){
        if(!p->frame->r11){irq_restore(f);return false;}
        struct elf_pair_control control={.magic=ELF_PAIR_MAGIC,.count=2};
        const struct elf_start *owners[]={start,dependency};
        for(unsigned i=0;i<2;++i){const struct elf_start *s=owners[i];
            control.images[i]=(struct elf_image_lifecycle){s->init,s->array,s->count,
                s->fini,s->fini_array,s->fini_count,s->cxa,0};}
        if(!process_image_copy(p,ELF_PAIR_CONTROL_ADDRESS,&control,sizeof(control),true)){
            irq_restore(f);return false;}
        p->frame->rax=ELF_PAIR_CONTROL_ADDRESS;
        p->frame->r15=PLATFORM_MYOSFINALIZEIMAGES;
    }
    p->frame->rip=USER_BASE+ELF_BOOT_OFFSET;
    if(!scheduler_attach_process(p)){irq_restore(f);return false;}
    p->state=PROCESS_RUNNING;irq_restore(f);return true;
}
bool process_start_elf(struct process *p,const struct elf_start *start)
{return start_images(p,start,0);}
bool process_start_elf_pair(struct process *p,const struct elf_start *starts)
{return start_images(p,&starts[0],&starts[1]);}
uint64_t process_create_elf(const struct elf_source *source,const struct elf_plan *plan)
{
    struct process *p=process_stage_elf(source,plan);if(!p)return 0;
    struct elf_start start={0};
    if(!process_start_elf(p,&start)){process_discard_elf(p);return 0;}return p->pid;
}
