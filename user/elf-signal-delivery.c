#include <signal.h>
#include <pthread.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
/* Existing native VM service ABI; no host mman header or shared mmap claim. */
#define PROT_READ 1
#define PROT_WRITE 2
#define MAP_PRIVATE 2
#define MAP_ANONYMOUS 32
#define MAP_FAILED ((void *)(uintptr_t)-1)
#ifdef __cplusplus
extern "C" {
#endif
void *mmap(void *,size_t,int,int,int,int64_t);
int mprotect(void *,size_t,int);
int munmap(void *,size_t);
#ifdef __cplusplus
}
#endif
static void message(const char *text,size_t length)
{
    register uint64_t a __asm__("rax")=1,d __asm__("rdi")=(uintptr_t)text;
    register uint64_t s __asm__("rsi")=length;
    __asm__ volatile("int $0x80":"+a"(a):"D"(d),"S"(s):"rdx","memory","cc");
}
#define SAY(s) do{const char text[]=s;message(text,sizeof(text)-1);}while(0)
#define CHECK(x) do{if(!(x)){SAY("signal delivery failed: " #x "\n");__builtin_trap();}}while(0)
static volatile unsigned first,second,nesting,maximum;
static unsigned mode;
static volatile unsigned abort_calls;
static void *fault_page;
__attribute__((visibility("hidden"))) uint64_t signal_probe_count;
#ifdef __cplusplus
extern "C" {
#endif
int MyOSSignalProbe(void);
void MyOSSignalClobber(int);
#ifdef __cplusplus
}
#endif
static void second_handler(int number)
{CHECK(number==SIGUSR2);++second;}
static void first_handler(int number)
{
    CHECK(number==SIGUSR1);++first;
    sigset_t mask;CHECK(!pthread_sigmask(-1,0,&mask));
    CHECK(sigismember(&mask,SIGUSR1)==(mode!=2&&mode!=8));
    if(mode==1&&first==1){
        CHECK(sigismember(&mask,SIGUSR2)==1);
        CHECK(!raise(SIGUSR1)&&!raise(SIGUSR2)&&!raise(SIGUSR2));
        CHECK(first==1&&second==0);
    }else if(mode==2){
        if(++nesting>maximum)maximum=nesting;
        if(nesting==1)CHECK(!raise(SIGUSR1));
        --nesting;
    }else if(mode==3){
        struct timespec pause={0,20000000};CHECK(!clock_nanosleep(CLOCK_MONOTONIC,0,&pause,0));
    }else if(mode==7){
        register uint64_t a __asm__("rax")=49,d __asm__("rdi")=3,s __asm__("rsi")=0;
        __asm__ volatile("int $0x80":"+a"(a):"D"(d),"S"(s):"memory","cc");
        CHECK((int64_t)a==-EINVAL);
    }else if(mode==8){
        CHECK(!raise(SIGUSR1));
    }
}
static void fault_handler(int number)
{CHECK(number==SIGSEGV);++second;CHECK(!mprotect(fault_page,4096,PROT_READ|PROT_WRITE));}
static void exit_handler(int number)
{CHECK(number==SIGUSR1);pthread_exit((void *)(uintptr_t)95);}
static void abort_handler(int number)
{
    CHECK(number==SIGABRT);SAY("SIGABRT handler entered\n");
#if SIGNAL_CASE == 7
    register uint64_t a __asm__("rax")=3,d __asm__("rdi")=93;
    __asm__ volatile("int $0x80":"+a"(a):"D"(d):"memory","cc");
    __builtin_unreachable();
#else
    if(++abort_calls==1)CHECK(!raise(SIGABRT));
#endif
}
static void loop_handler(int number)
{
    CHECK(number==SIGUSR1);SAY("signal handler cancellation ready\n");
    for(;;){struct timespec pause={0,1000000};CHECK(!clock_nanosleep(CLOCK_MONOTONIC,0,&pause,0));}
}
static void *worker(void *argument)
{
    if(mode==4){CHECK(!raise(SIGUSR1));return argument;}
    if(mode==5)abort();
    if(mode==6){CHECK(!raise(SIGUSR1));__builtin_trap();}
    sigset_t mask;CHECK(!pthread_sigmask(-1,0,&mask));CHECK(sigismember(&mask,SIGUSR1)==1);
    CHECK(!raise(SIGUSR1));CHECK(!sigemptyset(&mask));
    CHECK(!pthread_sigmask(SIG_SETMASK,&mask,0));return argument;
}
#ifdef __cplusplus
extern "C"
#endif
int _start(void)
{
#if SIGNAL_CASE == 1
    abort();
#elif SIGNAL_CASE == 2
    CHECK(signal(SIGABRT,abort_handler)==SIG_DFL);abort();
#elif SIGNAL_CASE == 3
    CHECK(signal(SIGABRT,SIG_IGN)==SIG_DFL);abort();
#elif SIGNAL_CASE == 4
    sigset_t blocked;CHECK(!sigemptyset(&blocked)&&!sigaddset(&blocked,SIGABRT));
    CHECK(!pthread_sigmask(SIG_BLOCK,&blocked,0));abort();
#elif SIGNAL_CASE == 5
    CHECK(signal(SIGABRT,abort_handler)==SIG_DFL);mode=5;
    pthread_t thread;CHECK(!pthread_create(&thread,0,worker,0));pthread_join(thread,0);__builtin_trap();
#elif SIGNAL_CASE == 6
    struct sigaction reset={0};reset.sa_handler=first_handler;reset.sa_flags=SA_RESETHAND;
    CHECK(!sigaction(SIGUSR1,&reset,0)&&!raise(SIGUSR1)&&first==1);CHECK(!raise(SIGUSR1));__builtin_trap();
#elif SIGNAL_CASE == 7
    CHECK(signal(SIGABRT,abort_handler)==SIG_DFL);abort();
#elif SIGNAL_CASE == 8
    CHECK(signal(SIGUSR1,loop_handler)==SIG_DFL);mode=6;
    pthread_t thread;CHECK(!pthread_create(&thread,0,worker,0));pthread_join(thread,0);__builtin_trap();
#elif SIGNAL_CASE == 9
    CHECK(signal(SIGUSR1,loop_handler)==SIG_DFL);CHECK(!raise(SIGUSR1));__builtin_trap();
#elif SIGNAL_CASE == 10
    /* Return using a forged context token must not restore guest registers. */
    register uint64_t a __asm__("rax")=49,d __asm__("rdi")=3,s __asm__("rsi")=0;
    __asm__ volatile("int $0x80":"+a"(a):"D"(d),"S"(s):"memory","cc");
    CHECK((int64_t)a==-EINVAL);SAY("native forged sigreturn rejected PASS\n");return 94;
#elif SIGNAL_CASE == 11
    CHECK(signal(SIGUSR1,exit_handler)==SIG_DFL);mode=4;
    for(unsigned i=0;i<16;++i){pthread_t thread;void *result=0;
        CHECK(!pthread_create(&thread,0,worker,0));CHECK(!pthread_join(thread,&result)&&result==(void *)(uintptr_t)95);}
    SAY("native handler pthread_exit/slot reuse PASS\n");return 95;
#elif SIGNAL_CASE == 12
    struct sigaction recursive={0};recursive.sa_handler=first_handler;recursive.sa_flags=SA_NODEFER;
    CHECK(!sigaction(SIGUSR1,&recursive,0));mode=8;CHECK(!raise(SIGUSR1));__builtin_trap();
#else
    (void)abort_handler;(void)loop_handler;
    CHECK(sizeof(struct sigaction)==152&&sizeof(siginfo_t)==128);
    CHECK(signal(SIGUSR1,first_handler)==SIG_DFL&&signal(SIGUSR2,second_handler)==SIG_DFL);
    struct {uint64_t before;struct sigaction action;uint64_t after;} output={0};
    output.before=123;output.after=456;
    CHECK(!sigaction(SIGUSR1,0,&output.action)&&output.action.sa_handler==first_handler);
    CHECK(output.before==123&&output.after==456);
    struct sigaction unsupported={0};unsupported.sa_handler=first_handler;
    unsupported.sa_flags=SA_SIGINFO;errno=0;
    CHECK(sigaction(SIGUSR1,&unsupported,&output.action)==-1&&errno==95);
    CHECK(output.action.sa_handler==first_handler&&output.before==123&&output.after==456);
    unsupported.sa_flags=SA_RESTART;CHECK(sigaction(SIGUSR1,&unsupported,0)==-1&&errno==95);
    CHECK(signal(SIGKILL,first_handler)==SIG_ERR&&errno==EINVAL);
    CHECK(signal(SIGSTOP,first_handler)==SIG_ERR&&errno==EINVAL);
    CHECK(raise(0)==-1&&errno==EINVAL&&raise(128)==-1&&errno==EINVAL);
    /* Invalid sigreturn while a context is live must leave it usable. */
    mode=7;CHECK(!raise(SIGUSR1));first=0;mode=0;
    errno=66;CHECK(!raise(SIGUSR1)&&first==1&&errno==66);
    sigset_t blocked;CHECK(!sigemptyset(&blocked)&&!sigaddset(&blocked,SIGUSR1));
    CHECK(!pthread_sigmask(SIG_BLOCK,&blocked,0));
    CHECK(!raise(SIGUSR1)&&!raise(SIGUSR1)&&!raise(SIGUSR1)&&first==1);
    CHECK(!pthread_sigmask(SIG_UNBLOCK,&blocked,0)&&first==2);
    CHECK(!pthread_sigmask(SIG_BLOCK,&blocked,0)&&!raise(SIGUSR1));
    CHECK(signal(SIGUSR1,SIG_IGN)==first_handler&&signal(SIGUSR1,first_handler)==SIG_IGN);
    CHECK(!pthread_sigmask(SIG_UNBLOCK,&blocked,0)&&first==2);
    struct sigaction nested={0};nested.sa_handler=first_handler;CHECK(!sigaddset(&nested.sa_mask,SIGUSR2));
    CHECK(!sigaction(SIGUSR1,&nested,0));mode=1;first=0;
    CHECK(!raise(SIGUSR1)&&first==2&&second==1);
    nested.sa_flags=SA_NODEFER;CHECK(!sigemptyset(&nested.sa_mask)&&!sigaction(SIGUSR1,&nested,0));
    mode=2;first=0;CHECK(!raise(SIGUSR1)&&first==2&&maximum==2&&!nesting);
    nested.sa_flags=0;CHECK(!sigaction(SIGUSR1,&nested,0));mode=3;
    CHECK(!raise(SIGUSR1));
    mode=0;CHECK(signal(SIGUSR1,MyOSSignalClobber)==first_handler);
    CHECK(MyOSSignalProbe()==0&&signal_probe_count==1);
    CHECK(signal(SIGUSR1,first_handler)==MyOSSignalClobber);
    CHECK(!pthread_sigmask(SIG_BLOCK,&blocked,0));
    pthread_t thread;unsigned before=first;
    CHECK(!pthread_create(&thread,0,worker,(void *)(uintptr_t)42));void *result=0;
    CHECK(!pthread_join(thread,&result)&&result==(void *)(uintptr_t)42&&first==before+1);
    sigset_t mask;CHECK(!pthread_sigmask(-1,0,&mask)&&sigismember(&mask,SIGUSR1)==1);
    CHECK(!pthread_sigmask(SIG_UNBLOCK,&blocked,0));
    fault_page=mmap(0,4096,PROT_READ,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);CHECK(fault_page!=MAP_FAILED);
    CHECK(signal(SIGSEGV,fault_handler)==SIG_DFL);before=second;
    *(volatile unsigned char *)fault_page=42;
    CHECK(*(volatile unsigned char *)fault_page==42&&second==before+1);
    CHECK(!munmap(fault_page,4096));
    SAY("native self signals/pending/nesting/fault/context PASS\n");return 90;
#endif
}
