#include "os.h"
#define TASKS 4
struct task {struct frame *frame;void (*entry)(void);void *stack;bool alive;};
static struct task tasks[TASKS];
static unsigned current;
static bool enabled;
volatile uint64_t task_counts[2];
static _Noreturn void task_entry(void){
    tasks[current].entry();
    __asm__ volatile("cli":::"memory");tasks[current].alive=false;
    for(;;)__asm__ volatile("sti; hlt");
}
void scheduler_init(void){tasks[0].alive=true;current=0;enabled=false;}
bool task_create(void (*entry)(void)){
    uint64_t f=irq_save();
    for(unsigned i=1;i<TASKS;++i)if(!tasks[i].stack){
        void *stack=kmalloc(16384);if(!stack){irq_restore(f);return false;}
        uintptr_t top=((uintptr_t)stack+16384)&~15ull;top-=8;*(uint64_t *)top=0;
        struct frame *frame=(struct frame *)(top-sizeof(struct frame));memset(frame,0,sizeof(*frame));
        frame->rip=(uintptr_t)task_entry;frame->cs=0x18;frame->ss=0x10;frame->rsp=top;frame->flags=0x202;
        tasks[i]=(struct task){frame,entry,stack,true};irq_restore(f);return true;
    }irq_restore(f);return false;
}
void scheduler_start(void){enabled=true;}
struct frame *schedule(struct frame *f){
    if(!enabled)return f;
    tasks[current].frame=f;
    for(unsigned step=1;step<=TASKS;++step){unsigned n=(current+step)%TASKS;if(tasks[n].alive){current=n;return tasks[n].frame;}}
    panic("no runnable task");
}
