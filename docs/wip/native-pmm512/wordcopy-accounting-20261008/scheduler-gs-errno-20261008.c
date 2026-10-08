#include "boot.h"
#include "process.h"
#include "tcp.h"
#include "dns.h"
#include "native-ui.h"
#include "audio.h"
#define KERNEL_TASKS 4
#define TASKS (KERNEL_TASKS + PROCESS_LIMIT * (1 + PROCESS_THREAD_LIMIT))

struct task {
    struct frame *frame;
    void (*entry)(void);
    void *stack;
    bool alive;
    struct process *process;
    struct process_thread *thread;
    bool blocked;
    uint64_t wait_address,wait_deadline;
    uint64_t fs_base;
};
static struct task tasks[TASKS];
/* Kernel/IRQ code must remain -mgeneral-regs-only. */
/* FXSAVE requires16-byte alignment. Kernel kmalloc guarantees that alignment.
 * Keep the BIOS kernel reservation unchanged as task capacity grows. */
static unsigned char (*fp)[512];
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
    fp=kmalloc(TASKS*sizeof(*fp));check(fp&& !((uintptr_t)fp&15),"scheduler FPU state allocation/alignment");
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
        tasks[i] = (struct task){.frame=frame,.entry=entry,.stack=stack,.alive=true};
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
        tasks[i] = (struct task){.frame=process->frame,.stack=process->kernel_stack,
            .alive=true,.process=process,.fs_base=process->main_fs_base};
        return true;
    }
    return false;
}

struct process *scheduler_current_process(void) { return tasks[current].process; }
struct process_thread *scheduler_current_thread(void){return tasks[current].thread;}

bool scheduler_attach_thread(struct process *process,struct process_thread *thread)
{
    for(unsigned i=KERNEL_TASKS;i<TASKS;++i){
        if(tasks[i].process)continue;
        fp_reset(i);
        tasks[i]=(struct task){.frame=thread->frame,.stack=thread->kernel_stack,
            .alive=true,.process=process,.thread=thread,.fs_base=thread->fs_base};
        return true;
    }
    return false;
}

bool scheduler_stop_thread(struct process *process,struct process_thread *thread)
{
    dns_job_release_owner(process,thread->tid);
    for(unsigned i=KERNEL_TASKS;i<TASKS;++i)
        if(tasks[i].process==process&&tasks[i].thread==thread){
            tasks[i].alive=false;tasks[i].blocked=false;return true;
        }
    return false;
}

bool scheduler_stop_process(struct process *process)
{
    audio_stream_release(process->pid);
    dns_job_release_owner(process,0);
    tcp_stream_release_owner(process);
    native_ui_release_owner(process);
    bool found=false;
    for (unsigned i = KERNEL_TASKS; i < TASKS; ++i) {
        if (tasks[i].process != process) continue;
        found=true;tasks[i].alive=false;tasks[i].blocked=false;
    }
    return found;
}

bool scheduler_detach_process(struct process *process)
{
    for(unsigned i=KERNEL_TASKS;i<TASKS;++i)
        if(tasks[i].process==process&&i==current)return false;
    bool found=false;
    for (unsigned i = KERNEL_TASKS; i < TASKS; ++i) {
        if (tasks[i].process != process) continue;
        found=true;
        memset(&tasks[i], 0, sizeof(tasks[i]));
    }
    return found;
}

bool scheduler_detach_thread(struct process *process,struct process_thread *thread)
{
    for(unsigned i=KERNEL_TASKS;i<TASKS;++i)
        if(tasks[i].process==process&&tasks[i].thread==thread){
            if(i==current)return false;
            memset(&tasks[i],0,sizeof(tasks[i]));return true;
        }
    return false;
}

struct frame *scheduler_wait(struct frame *frame,struct process *process,
                             uint64_t address,uint64_t deadline)
{
    struct task *task=&tasks[current];
    check(task->process==process&&!task->blocked,"invalid wait task");
    /* IRQ/syscall serialization makes pending-check and block atomic. */
    if(process_signal_pending(process)){frame->rax=(uint64_t)-4;return frame;}
    task->frame=frame;task->alive=false;task->blocked=true;
    task->wait_address=address;task->wait_deadline=deadline;
    return schedule(frame);
}

unsigned scheduler_wake(struct process *process,uint64_t address,unsigned count)
{
    unsigned woke=0;
    if(!count)return 0;
    for(unsigned i=KERNEL_TASKS;i<TASKS&&woke<count;++i){
        struct task *task=&tasks[i];
        if(task->process!=process||!task->blocked||task->wait_address!=address)continue;
        task->blocked=false;task->alive=true;task->frame->rax=0;++woke;
    }
    return woke;
}

void scheduler_start(void) { enabled = true; }

/* Only the boot/shell task may retire the disposable kernel probes. Their
 * register/preemption checks have already run; keeping CPU-bound probes alive
 * in a normal image needlessly competes with real application threads. */
void scheduler_finish_boot_tests(void)
{
    uint64_t flags=irq_save();
    check(current==0,"boot probe cleanup from boot task");
    for(unsigned i=1;i<KERNEL_TASKS;++i){
        void *stack=tasks[i].stack;
        memset(&tasks[i],0,sizeof(tasks[i]));
        fp_reset(i);
        if(stack)kfree(stack);
    }
    irq_restore(flags);
}

bool scheduler_interrupt(struct process *process,struct process_thread *thread)
{
    for(unsigned i=KERNEL_TASKS;i<TASKS;++i){
        struct task *task=&tasks[i];
        if(task->process!=process||task->thread!=thread)continue;
        if(!task->alive&&!task->blocked)return false;
        if(task->blocked){task->frame->rax=(uint64_t)-4;
            task->blocked=false;task->alive=true;}
        return true;
    }
    return false;
}

struct frame *schedule(struct frame *frame)
{
    if (!enabled) return frame;
    tasks[current].frame = frame;
    /* 100 Hz sampled CPU accounting: charge only the task interrupted by
     * the PIT, including its kernel service time. Sleeping tasks get no
     * samples; yields/syscalls never manufacture elapsed CPU time. */
    if(tasks[current].process&&frame->vector==32){
        struct process *p=tasks[current].process;
        uint64_t *local=tasks[current].thread?&tasks[current].thread->cpu_nanos:&p->main_cpu_nanos;
        const uint64_t sample=10000000ull,ceiling=INT64_MAX;
        p->cpu_nanos=p->cpu_nanos>ceiling-sample?ceiling:p->cpu_nanos+sample;
        *local=*local>ceiling-sample?ceiling:*local+sample;
    }
    if (tasks[current].process && (frame->cs & 3) == 3 && frame->vector == 32)
        ++tasks[current].process->preemptions;
    for(unsigned i=KERNEL_TASKS;i<TASKS;++i){
        struct task *task=&tasks[i];
        if(task->blocked&&task->wait_deadline&&ticks>=task->wait_deadline){
            task->blocked=false;task->alive=true;task->frame->rax=(uint64_t)-110;
        }
    }
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
        tss_set_stack(process ? (tasks[next].thread?tasks[next].thread->kernel_top:
                      process->kernel_top) : boot_state.stack);
        uint64_t fs=tasks[next].fs_base;
        __asm__ volatile("wrmsr"::"c"(0xc0000100u),"a"((uint32_t)fs),
                         "d"((uint32_t)(fs>>32)):"memory");
        /* GS is the private runtime identity, never the ELF TLS module (FS).
         * No kernel code dereferences GS; kernel tasks explicitly clear it. */
        uint64_t gs=process?process_errno_location(process):0;
        __asm__ volatile("wrmsr"::"c"(0xc0000101u),"a"((uint32_t)gs),
                         "d"((uint32_t)(gs>>32)):"memory");
        /* All roots share supervisor-only kernel/heap mappings at PML4[0]. */
        __asm__ volatile ("mov %0,%%cr3" : : "r"(root) : "memory");
        return tasks[next].frame;
    }
    panic("no runnable task");
}
