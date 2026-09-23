#include "process.h"
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
    for (unsigned i = 0; i < process->page_count; ++i) page_free(process->pages[i]);
    kfree(process->kernel_stack);
    memset(process, 0, sizeof(*process));
}

uint64_t process_create(uint64_t mode, uint64_t cookie)
{
    uint64_t flags = irq_save();
    struct process *process = 0;
    for (unsigned i = 0; i < PROCESS_LIMIT; ++i)
        if (processes[i].state == PROCESS_UNUSED) { process = &processes[i]; break; }
    if (!process) { irq_restore(flags); return 0; }
    size_t image_size = (size_t)(user_blob_end - user_blob_start);
    check(image_size <= PAGE_SIZE, "user image exceeds code page");
    for (unsigned i = 0; i < PROCESS_PAGES; ++i) {
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
    root[0] = ((uint64_t *)0x1000)[0] & ~4ull;
    root[1] = process->pages[1] | 7;
    pdpt[0] = process->pages[2] | 7;
    pd[0] = process->pages[3] | 7;
    pt[0] = process->pages[4] | 5;       /* User code: RX. */
    pt[1] = process->pages[5] | 7 | NX;  /* User private data: RW, NX. */
    /* pt[2] is the unmapped guard immediately below the stack. */
    pt[3] = process->pages[6] | 7 | NX;  /* User stack: RW, NX. */
    memcpy((void *)process->pages[4], user_blob_start, image_size);
    uint64_t *data = (uint64_t *)process->pages[5];
    data[2] = cookie;
    data[3] = mode;
    if (mode == 14) {
        /* Test-only buffers: RX -> RW crossing and a partial invalid request. */
        memcpy((void *)(process->pages[4] + PAGE_SIZE - 2), "AB", 2);
        memcpy((void *)process->pages[5], "CD", 2);
        memcpy((void *)(process->pages[5] + PAGE_SIZE - 4), "LEAK", 4);
    }
    /* RET is used by the NX-stack execution test. */
    *(uint8_t *)process->pages[6] = 0xc3;
    process->data_page = process->pages[5];
    process->root = process->pages[0];
    process->kernel_top = ((uintptr_t)process->kernel_stack + 16384) & ~15ull;
    process->frame = (struct frame *)(process->kernel_top - sizeof(struct frame));
    memset(process->frame, 0, sizeof(*process->frame));
    process->frame->rip = USER_BASE;
    process->frame->cs = 0x2b;
    process->frame->ss = 0x23;
    process->frame->rsp = USER_STACK + PAGE_SIZE - 8;
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
