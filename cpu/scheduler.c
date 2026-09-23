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
        current = next;
        struct process *process = tasks[next].process;
        uintptr_t root = process ? process->root : 0x1000;
        tss_set_stack(process ? process->kernel_top : 0x90000);
        /* All roots share supervisor-only kernel/heap mappings at PML4[0]. */
        __asm__ volatile ("mov %0,%%cr3" : : "r"(root) : "memory");
        return tasks[next].frame;
    }
    panic("no runnable task");
}
