#include <signal.h>
#include <pthread.h>
#include <stddef.h>
#include <time.h>
#define CHECK(x) do { if(!(x)){const char text[]="signal SDK failed: " #x "\n";message(text,sizeof(text)-1);__builtin_trap();} } while(0)
static void message(const char *text,size_t length)
{
    register uint64_t a __asm__("rax")=1,d __asm__("rdi")=(uintptr_t)text;
    register uint64_t s __asm__("rsi")=length;
    __asm__ volatile("int $0x80":"+a"(a):"D"(d),"S"(s):"rdx","memory","cc");
}
static struct { uint64_t before;sigset_t value;uint64_t after; } guarded;
static unsigned ready,release_workers;
static void equal(const sigset_t *a,const sigset_t *b)
{for(unsigned i=0;i<16;++i)CHECK(a->bits[i]==b->bits[i]);}
static void guards(void)
{CHECK(guarded.before==UINT64_C(0x0123456789abcdef)&&guarded.after==UINT64_C(0xfedcba9876543210));}
static void *worker(void *argument)
{
    unsigned id=(unsigned)(uintptr_t)argument;
    sigset_t observed,own;
    errno=79;CHECK(!pthread_sigmask(-1,0,&observed));
    CHECK(sigismember(&observed,SIGUSR1)==1&&sigismember(&observed,127)==1);
    CHECK(sigismember(&observed,SIGKILL)==0&&sigismember(&observed,SIGSTOP)==0);
    CHECK(!sigemptyset(&own)&&!sigaddset(&own,64+(int)id));
    CHECK(!pthread_sigmask(SIG_SETMASK,&own,0)&&errno==79);
    __atomic_add_fetch(&ready,1,__ATOMIC_RELEASE);
    while(!__atomic_load_n(&release_workers,__ATOMIC_ACQUIRE)){
        struct timespec pause={0,1000000};CHECK(!clock_nanosleep(CLOCK_MONOTONIC,0,&pause,0));
    }
    for(unsigned j=0;j<16;++j){
        struct timespec pause={0,1000000};CHECK(!clock_nanosleep(CLOCK_MONOTONIC,0,&pause,0));
        CHECK(!pthread_sigmask(SIG_BLOCK,0,&observed));equal(&own,&observed);
    }
    CHECK(errno==79);return argument;
}
#ifdef __cplusplus
extern "C"
#endif
int _start(void)
{
    guarded.before=UINT64_C(0x0123456789abcdef);guarded.after=UINT64_C(0xfedcba9876543210);
    errno=73;CHECK(!sigemptyset(&guarded.value));
    for(int signal=1;signal<=127;++signal){
        CHECK(!sigaddset(&guarded.value,signal));
        for(int other=1;other<=127;++other)CHECK(sigismember(&guarded.value,other)==(signal==other));
        for(unsigned i=2;i<16;++i)CHECK(!guarded.value.bits[i]);
        CHECK(!sigdelset(&guarded.value,signal));guards();
    }
    CHECK(errno==73&&!sigfillset(&guarded.value));guards();
    CHECK(guarded.value.bits[0]==UINT64_MAX&&guarded.value.bits[1]==(UINT64_MAX>>1));
    sigset_t snapshot=guarded.value;
    const int invalid[]={-2147483647,-1,0,128,2147483647};
    for(unsigned i=0;i<sizeof(invalid)/sizeof(invalid[0]);++i){
        errno=0;CHECK(sigaddset(&guarded.value,invalid[i])==-1&&errno==EINVAL);equal(&snapshot,&guarded.value);
        errno=0;CHECK(sigdelset(&guarded.value,invalid[i])==-1&&errno==EINVAL);equal(&snapshot,&guarded.value);
        errno=0;CHECK(sigismember(&guarded.value,invalid[i])==-1&&errno==EINVAL);guards();
    }
    CHECK(sigemptyset(0)==-1&&errno==EINVAL&&sigfillset(0)==-1&&errno==EINVAL);
    CHECK(sigaddset(0,1)==-1&&errno==EINVAL&&sigdelset(0,1)==-1&&errno==EINVAL);
    CHECK(sigismember(0,1)==-1&&errno==EINVAL);
    for(unsigned i=0;i<16;++i)guarded.value.bits[i]=UINT64_MAX;
    errno=61;CHECK(!pthread_sigmask(SIG_SETMASK,&guarded.value,&guarded.value)&&errno==61);
    for(unsigned i=0;i<16;++i)CHECK(!guarded.value.bits[i]);
    guards();
    CHECK(!pthread_sigmask(-123,0,&guarded.value));
    CHECK(guarded.value.bits[0]==(UINT64_MAX&~((UINT64_C(1)<<8)|(UINT64_C(1)<<18))));
    CHECK(guarded.value.bits[1]==(UINT64_MAX>>1));for(unsigned i=2;i<16;++i)CHECK(!guarded.value.bits[i]);
    snapshot=guarded.value;CHECK(pthread_sigmask(-1,&snapshot,&guarded.value)==EINVAL&&errno==61);
    equal(&snapshot,&guarded.value);guards();
    CHECK(!sigemptyset(&snapshot)&&!sigaddset(&snapshot,SIGUSR1)&&!sigaddset(&snapshot,127));
    CHECK(!pthread_sigmask(SIG_SETMASK,&snapshot,0));
    pthread_t threads[8];
    for(unsigned i=0;i<8;++i)CHECK(!pthread_create(&threads[i],0,worker,(void *)(uintptr_t)i));
    while(__atomic_load_n(&ready,__ATOMIC_ACQUIRE)!=8){
        struct timespec pause={0,1000000};CHECK(!clock_nanosleep(CLOCK_MONOTONIC,0,&pause,0));
    }
    CHECK(!pthread_sigmask(SIG_BLOCK,0,&guarded.value));equal(&snapshot,&guarded.value);guards();
    __atomic_store_n(&release_workers,1,__ATOMIC_RELEASE);
    for(unsigned i=0;i<8;++i){void *result=0;CHECK(!pthread_join(threads[i],&result)&&result==(void *)(uintptr_t)i);}
    CHECK(!pthread_sigmask(SIG_UNBLOCK,&snapshot,0));CHECK(!pthread_sigmask(SIG_BLOCK,0,&guarded.value));
    for(unsigned i=0;i<16;++i)CHECK(!guarded.value.bits[i]);
    guards();
    const char passed[]="native signal SDK masks/inheritance/isolation PASS (no delivery)\n";
    message(passed,sizeof(passed)-1);return 89;
}
