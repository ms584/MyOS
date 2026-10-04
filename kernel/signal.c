#include "syscall.h"
#include "platform-runtime-exports.h"

/* Native self-delivery subset. No cross-task sends, job control, alternate
 * stacks, SA_SIGINFO, restart policy or realtime signal queues yet. */
#define NODEFER 0x40000000u
#define RESETHAND 0x80000000u
#define UNBLOCKABLE ((1ull<<8)|(1ull<<18))
struct user_action { uint64_t handler,mask[16];uint32_t flags,padding;uint64_t restorer; };
_Static_assert(sizeof(struct user_action)==152,"pinned sigaction ABI");
struct action { uint64_t handler,mask[2];uint32_t flags; };
struct context {
    unsigned char fp[512] __attribute__((aligned(16)));
    struct frame frame;
    uint64_t mask[2],token;
    struct context *previous;
};
struct signal_task { uint64_t mask[2],pending;struct context *context;unsigned depth; };
struct process_signal {
    struct action actions[32];
    struct signal_task tasks[PROCESS_THREAD_LIMIT+1];
    unsigned termination_signal;
};
static unsigned slot(struct process *p,struct process_thread *thread)
{return thread?(unsigned)(thread-p->threads)+1:0;}
static struct signal_task *self(struct process *p)
{return &p->signals->tasks[slot(p,scheduler_current_thread())];}
static bool ensure(struct process *p)
{
    if(p->signals)return true;
    p->signals=kmalloc(sizeof(*p->signals));
    if(p->signals)memset(p->signals,0,sizeof(*p->signals));
    return p->signals!=0;
}
static bool valid(unsigned signal)
{return signal>0&&signal<32&&!(signal>=19&&signal<=22);}
static bool ignored(unsigned signal,const struct action *action)
{return action->handler==1||(!action->handler&&(signal==17||signal==18||signal==23||signal==28));}
static void clear(struct signal_task *task)
{
    while(task->context){struct context *context=task->context;
        task->context=context->previous;kfree(context);}
    memset(task,0,sizeof(*task));
}
void process_signal_release(struct process *p)
{
    if(!p->signals)return;
    for(unsigned i=0;i<=PROCESS_THREAD_LIMIT;++i)clear(&p->signals->tasks[i]);
    kfree(p->signals);p->signals=0;
}
void process_signal_reset_thread(struct process *p,struct process_thread *thread,bool inherit)
{
    if(!p->signals)return;
    struct signal_task *task=&p->signals->tasks[slot(p,thread)];clear(task);
    if(inherit)memcpy(task->mask,self(p)->mask,sizeof(task->mask));
}
static struct frame *terminate(struct process *p,struct frame *frame,unsigned signal)
{
    if(p->signals)p->signals->termination_signal=signal;
    print("USER SIGNAL pid=");decimal(p->pid);print(" signal=");decimal(signal);print("\n");
    fs_close_all(p->files);scheduler_stop_process(p);p->state=PROCESS_ZOMBIE;
    return process_signal_deliver(schedule(frame));
}
struct frame *process_signal_deliver(struct frame *frame)
{
    struct process *p=scheduler_current_process();
    if(!p||!p->signals||p->state!=PROCESS_RUNNING||(frame->cs&3)!=3)return frame;
    struct signal_task *task=self(p);
    while(task->pending&~task->mask[0]){
        unsigned signal=(unsigned)__builtin_ctzll(task->pending&~task->mask[0])+1;
        uint64_t bit=1ull<<(signal-1);task->pending&=~bit;
        struct action action=p->signals->actions[signal];
        if(ignored(signal,&action))continue;
        if(!action.handler)return terminate(p,frame,signal);
        if(task->depth==4||frame->rsp<160||
           !process_user_access(p,action.handler,1,PROCESS_PROT_EXEC))return terminate(p,frame,11);
        /* Respect even foreign user code's 128-byte red zone. Handler entry
         * RSP is 8 mod16, with a trusted restorer and user-address token. */
        uint64_t stack=((frame->rsp-128-16)&~15ull)-8;
        uint64_t words[]={PLATFORM_MYOSSIGNALRETURN,stack};
        if(!process_user_access(p,stack,sizeof(words),PROCESS_PROT_WRITE))return terminate(p,frame,11);
        struct context *context=kmalloc(sizeof(*context));
        if(!context)return terminate(p,frame,11);
        __asm__ volatile("fxsave64 %0":"=m"(context->fp)::"memory");
        context->frame=*frame;memcpy(context->mask,task->mask,sizeof(task->mask));
        context->token=stack;context->previous=task->context;
        if(!process_copy_user(p,stack,words,sizeof(words),true)){kfree(context);return terminate(p,frame,11);}
        task->context=context;++task->depth;
        task->mask[0]|=action.mask[0];task->mask[1]|=action.mask[1];
        if(!(action.flags&NODEFER))task->mask[0]|=bit;
        task->mask[0]&=~UNBLOCKABLE;
        if(action.flags&RESETHAND)p->signals->actions[signal]=(struct action){0};
        frame->rip=action.handler;frame->rsp=stack;frame->rdi=signal;
        /* A handler must start with DF clear even when the interrupted code
         * has DF set. The original complete flags live only in kernel state. */
        frame->flags&=~(1ull<<10);
        return frame;
    }
    return frame;
}
bool process_signal_fault(struct process *p,unsigned signal)
{
    if(!p->signals)return false;
    struct signal_task *task=self(p);struct action *action=&p->signals->actions[signal];
    if(action->handler<=1||(task->mask[0]&(1ull<<(signal-1))))return false;
    task->pending|=1ull<<(signal-1);return true;
}
struct frame *process_signal_call(struct process *p,struct frame *frame)
{
    int result=0;unsigned command=(unsigned)frame->rdi,argument=(unsigned)frame->rsi;
    if(command==0){ /* sigaction: number, new, old */
        struct user_action action={0},old={0};
        if(!valid(argument)){result=-22;goto done;}
        if(frame->r10&&!process_user_access(p,frame->r10,sizeof(old),PROCESS_PROT_WRITE)){result=-14;goto done;}
        if(frame->rdx){
            if(argument==9){result=-22;goto done;}
            if(!process_copy_user(p,frame->rdx,&action,sizeof(action),false)){result=-14;goto done;}
            if((action.flags&~(NODEFER|RESETHAND))||action.restorer){result=-95;goto done;}
            if(action.handler>1&&!process_user_access(p,action.handler,1,PROCESS_PROT_EXEC)){result=-14;goto done;}
            if(!ensure(p)){result=-12;goto done;}
        }
        if(p->signals){struct action *current=&p->signals->actions[argument];
            old.handler=current->handler;memcpy(old.mask,current->mask,sizeof(current->mask));old.flags=current->flags;
            if(frame->rdx){*current=(struct action){action.handler,{action.mask[0]&~UNBLOCKABLE,
                action.mask[1]&0x7fffffffffffffffull},action.flags};
                if(ignored(argument,current))for(unsigned i=0;i<=PROCESS_THREAD_LIMIT;++i)
                    p->signals->tasks[i].pending&=~(1ull<<(argument-1));
            }
        }
        if(frame->r10)(void)process_copy_user(p,frame->r10,&old,sizeof(old),true);
    }else if(command==1){ /* pthread_sigmask: how, new, old; errno untouched */
        uint64_t mask[16]={0},old[16]={0};
        if(frame->rdx){
            if(argument>2){result=-22;goto done;}
            if(!process_copy_user(p,frame->rdx,mask,sizeof(mask),false)){result=-14;goto done;}
        }
        if(frame->r10&&!process_user_access(p,frame->r10,sizeof(old),PROCESS_PROT_WRITE)){result=-14;goto done;}
        if(frame->rdx&&!ensure(p)){result=-12;goto done;}
        if(p->signals){struct signal_task *task=self(p);memcpy(old,task->mask,sizeof(task->mask));
            if(frame->rdx){for(unsigned i=0;i<2;++i){
                if(argument==0)task->mask[i]|=mask[i];
                else if(argument==1)task->mask[i]&=~mask[i];else task->mask[i]=mask[i];}
                task->mask[0]&=~UNBLOCKABLE;task->mask[1]&=0x7fffffffffffffffull;
            }
        }
        if(frame->r10)(void)process_copy_user(p,frame->r10,old,sizeof(old),true);
    }else if(command==2||command==4){ /* self raise / force default SIGABRT */
        if(!valid(argument)||(command==4&&argument!=6)){result=-22;goto done;}
        if(command==4)return terminate(p,frame,6);
        if(!ensure(p)){result=-12;goto done;}
        if(!ignored(argument,&p->signals->actions[argument]))self(p)->pending|=1ull<<(argument-1);
    }else if(command==3){ /* Restore the top kernel-owned context, never guest registers. */
        struct signal_task *task=p->signals?self(p):0;
        struct context *context=task?task->context:0;
        if(!context||context->token!=frame->rsi){result=-22;goto done;}
        *frame=context->frame;memcpy(task->mask,context->mask,sizeof(task->mask));
        task->context=context->previous;--task->depth;
        __asm__ volatile("fxrstor64 %0"::"m"(context->fp):"memory");
        kfree(context);return frame;
    }else result=-38;
done:
    frame->rax=(uint64_t)(int64_t)result;return frame;
}
