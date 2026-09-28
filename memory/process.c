#include "boot.h"
#include "process.h"
#include "elf.h"
#define NX (1ull << 63)
static struct process processes[PROCESS_LIMIT];
static uint64_t next_pid = 1;
extern const unsigned char user_blob_start[], user_blob_end[];

struct process *process_find(uint64_t pid)
{
    for (unsigned i = 0; i < PROCESS_LIMIT; ++i)
        if (processes[i].state != PROCESS_UNUSED && processes[i].pid == pid)
            return &processes[i];
    return 0;
}

static void release(struct process *process)
{
    fs_close_all(process->files);
    /* Only detached, noncurrent address spaces may reach this function. */
    for (unsigned i = 0; i < process->page_count; ++i) page_free(process->image_pages?process->image_pages[i]:process->pages[i]);
    kfree(process->image_pages);
    kfree(process->kernel_stack);
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
    print("USER FAULT pid="); decimal(process->pid);
    print(" vector="); decimal(frame->vector);
    print(" error="); hex(frame->error); print("\n");
    fs_close_all(process->files);
    scheduler_stop_process(process);
    process->state = PROCESS_ZOMBIE;
    /* Reclamation is deferred: this IRQ frame still occupies its kernel stack. */
    return schedule(frame);
}

/* Separate ELF64 path: page ownership is dynamic, legacy demos keep their ABI. */
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
    *e=page|5|((flags&2)?2:0)|((flags&1)?0:NX);return page;
}
uint64_t process_create_elf(const struct elf_source *source,const struct elf_plan *plan)
{
    uint64_t flags=irq_save();struct process *p=0;
    for(unsigned i=0;i<PROCESS_LIMIT;++i)if(processes[i].state==PROCESS_UNUSED){p=&processes[i];break;}
    if(!p){irq_restore(flags);return 0;}
    unsigned capacity=128;
    for(unsigned i=0;i<plan->count;++i){const struct elf_segment *s=&plan->segments[i];
        capacity+=(unsigned)(((s->address%PAGE_SIZE)+s->memsz+4095)/PAGE_SIZE);
    }
    p->image_pages=kmalloc((size_t)capacity*sizeof(uintptr_t));
    if(!p->image_pages)goto failed;
    p->root=elf_page(p,capacity);if(!p->root)goto failed;
    ((uint64_t *)p->root)[0]=((uint64_t *)boot_state.root)[0]&~4ull;
    for(unsigned i=0;i<plan->count;++i){const struct elf_segment *s=&plan->segments[i];
        uint64_t end=s->address+s->memsz;
        for(uint64_t va=s->address&~4095ull;va<end;va+=PAGE_SIZE){
            uintptr_t page=elf_map_page(p,capacity,va,s->flags);if(!page)goto failed;
            uint64_t begin=va>s->address?va:s->address,file_end=s->address+s->filesz;
            uint64_t stop=va+PAGE_SIZE<file_end?va+PAGE_SIZE:file_end;
            if(begin<stop&&!source->read(source->context,s->offset+begin-s->address,
               (void *)(page+begin-va),(size_t)(stop-begin)))goto failed;
        }
    }
    /* Four NX stack pages with a guard beneath them; no executable stack. */
    for(unsigned i=0;i<4;++i)if(!elf_map_page(p,capacity,USER_BASE+ELF_STACK_OFFSET+i*PAGE_SIZE,6))goto failed;
    p->kernel_stack=kmalloc(16384);if(!p->kernel_stack)goto failed;
    p->kernel_top=((uintptr_t)p->kernel_stack+16384)&~15ull;
    p->frame=(struct frame *)(p->kernel_top-sizeof(struct frame));memset(p->frame,0,sizeof(*p->frame));
    p->frame->rip=plan->entry;p->frame->cs=0x2b;p->frame->ss=0x23;
    p->frame->rsp=USER_BASE+ELF_STACK_OFFSET+4*PAGE_SIZE-8;p->frame->flags=0x202;
    p->pid=next_pid++;p->state=PROCESS_RUNNING;
    if(!scheduler_attach_process(p))goto failed;
    uint64_t pid=p->pid;irq_restore(flags);return pid;
failed:
    release(p);irq_restore(flags);return 0;
}
