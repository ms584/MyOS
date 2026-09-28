#include "boot.h"
#include "process.h"
#define KERNEL_TASKS 4
#define TASKS (KERNEL_TASKS + PROCESS_LIMIT)

struct task {
    struct frame *frame;
    void (*entry)(void);
    void *stack;
    bool alive;
    struct process *process;
};
static struct task tasks[TASKS];
/* Kernel/IRQ code must remain -mgeneral-regs-only. */
static unsigned char fp[TASKS][512] __attribute__((aligned(16)));
static void fp_reset(unsigned slot)
{
    memset(fp[slot],0,512);
    fp[slot][0]=0x7f; fp[slot][1]=3;
    fp[slot][24]=0x80; fp[slot][25]=0x1f;
}
static void fp_init(void)
{
    unsigned a=1,b,c,d;
    __asm__ volatile("cpuid":"+a"(a),"=b"(b),"=c"(c),"=d"(d));
    check((d&((1u<<0)|(1u<<24)|(1u<<25)|(1u<<26)))==
          ((1u<<0)|(1u<<24)|(1u<<25)|(1u<<26)), "CPU requires x87/FXSR/SSE2");
    uintptr_t cr0,cr4;
    __asm__ volatile("mov %%cr0,%0":"=r"(cr0));
    cr0=(cr0&~((1ul<<2)|(1ul<<3)))|(1ul<<1)|(1ul<<5);
    __asm__ volatile("mov %0,%%cr0"::"r"(cr0):"memory");
    __asm__ volatile("mov %%cr4,%0":"=r"(cr4));
    /* AVX/XSAVE state is not supported yet. */
    cr4=(cr4&~(1ul<<18))|(1ul<<9)|(1ul<<10);
    __asm__ volatile("mov %0,%%cr4"::"r"(cr4):"memory");
    for(unsigned i=0;i<TASKS;++i)fp_reset(i);
    __asm__ volatile("fxrstor64 %0"::"m"(fp[0]):"memory");
}
static unsigned current;
static bool enabled;
volatile uint64_t task_counts[2];

static _Noreturn void task_entry(void)
{
    tasks[current].entry();
    __asm__ volatile ("cli" : : : "memory");
    tasks[current].alive = false;
    for (;;) __asm__ volatile ("sti; hlt");
}

void scheduler_init(void)
{
    fp_init();
    tasks[0].alive = true;
    current = 0;
    enabled = false;
}

bool task_create(void (*entry)(void))
{
    uint64_t flags = irq_save();
    for (unsigned i = 1; i < KERNEL_TASKS; ++i) {
        if (tasks[i].stack) continue;
        void *stack = kmalloc(16384);
        if (!stack) { irq_restore(flags); return false; }
        uintptr_t top = ((uintptr_t)stack + 16384) & ~15ull;
        top -= 8;
        *(uint64_t *)top = 0;
        struct frame *frame = (struct frame *)(top - sizeof(struct frame));
        memset(frame, 0, sizeof(*frame));
        frame->rip = (uintptr_t)task_entry;
        frame->cs = 0x18;
        frame->ss = 0x10;
        frame->rsp = top;
        frame->flags = 0x202;
        fp_reset(i);
        tasks[i] = (struct task){frame, entry, stack, true, 0};
        irq_restore(flags);
        return true;
    }
    irq_restore(flags);
    return false;
}

bool scheduler_attach_process(struct process *process)
{
    /* Caller holds interrupts disabled until all process state is published. */
    for (unsigned i = KERNEL_TASKS; i < TASKS; ++i) {
        if (tasks[i].process) continue;
        fp_reset(i);
        tasks[i] = (struct task){process->frame, 0, process->kernel_stack, true, process};
        return true;
    }
    return false;
}

struct process *scheduler_current_process(void) { return tasks[current].process; }

bool scheduler_stop_process(struct process *process)
{
    for (unsigned i = KERNEL_TASKS; i < TASKS; ++i) {
        if (tasks[i].process != process) continue;
        tasks[i].alive = false;
        return true;
    }
    return false;
}

bool scheduler_detach_process(struct process *process)
{
    for (unsigned i = KERNEL_TASKS; i < TASKS; ++i) {
        if (tasks[i].process != process) continue;
        if (i == current) return false;
        memset(&tasks[i], 0, sizeof(tasks[i]));
        return true;
    }
    return false;
}

void scheduler_start(void) { enabled = true; }

struct frame *schedule(struct frame *frame)
{
    if (!enabled) return frame;
    tasks[current].frame = frame;
    if (tasks[current].process && (frame->cs & 3) == 3 && frame->vector == 32)
        ++tasks[current].process->preemptions;
    for (unsigned step = 1; step <= TASKS; ++step) {
        unsigned next = (current + step) % TASKS;
        if (!tasks[next].alive) continue;
        if(next!=current){
            __asm__ volatile("fxsave64 %0":"=m"(fp[current])::"memory");
            __asm__ volatile("fxrstor64 %0"::"m"(fp[next]):"memory");
        }
        current = next;
        struct process *process = tasks[next].process;
        uintptr_t root = process ? process->root : boot_state.root;
        tss_set_stack(process ? process->kernel_top : boot_state.stack);
        /* All roots share supervisor-only kernel/heap mappings at PML4[0]. */
        __asm__ volatile ("mov %0,%%cr3" : : "r"(root) : "memory");
        return tasks[next].frame;
    }
    panic("no runnable task");
}
