#include "util/u_thread.h"
#define CHECK(x) do { if(!(x))__builtin_trap(); } while(0)
static util_barrier barrier;
static util_semaphore complete;
static unsigned serial[48],payload[8],ready;
static uintptr_t locals[8];
static __THREAD_INITIAL_EXEC uint64_t dispatch_identity=0xabcdef;
static void message(const char *text,unsigned length)
{
    register uint64_t a __asm__("rax")=1,d __asm__("rdi")=(uintptr_t)text;
    register uint64_t s __asm__("rsi")=length;
    __asm__ volatile("int $0x80":"+a"(a):"D"(d),"S"(s):"rdx","memory","cc");
}
static int worker(void *raw)
{
    unsigned who=(unsigned)(uintptr_t)raw;
    CHECK(dispatch_identity==0xabcdef);dispatch_identity=100+who;
    locals[who]=(uintptr_t)&dispatch_identity;
    sigset_t mask;CHECK(!pthread_sigmask(SIG_SETMASK,0,&mask));
    for(int signal=1;signal<=127;++signal)
        CHECK(sigismember(&mask,signal)==(signal!=SIGSYS&&signal!=SIGSEGV&&signal!=SIGKILL&&signal!=SIGSTOP));
    for(unsigned i=2;i<16;++i)CHECK(mask.bits[i]==0);
    u_thread_setname("renderer-worker-long-name");
    char name[16];CHECK(!pthread_getname_np(thrd_current(),name,sizeof(name)));
    CHECK(strlen(name)==15);
    const char expected[]="renderer-worker";
    for(unsigned i=0;i<16;++i)CHECK(name[i]==expected[i]);
    __atomic_add_fetch(&ready,1,__ATOMIC_RELEASE);
    for(unsigned wave=0;wave<24;++wave){
        payload[who]=wave+1;
        if(util_barrier_wait(&barrier))__atomic_add_fetch(&serial[wave*2],1,__ATOMIC_RELAXED);
        for(unsigned i=0;i<8;++i){CHECK(payload[i]==wave+1);
            CHECK(locals[i]!=0);if(i!=who)CHECK(locals[i]!=locals[who]);}
        CHECK(dispatch_identity==100+who);
        if((who+wave)%3==0){struct timespec delay={0,1000000};CHECK(!thrd_sleep(&delay,0));}
        if(util_barrier_wait(&barrier))__atomic_add_fetch(&serial[wave*2+1],1,__ATOMIC_RELAXED);
    }
    if(!who)util_semaphore_signal(&complete);
    return (int)who+80;
}
int _start(void)
{
    _Static_assert(sizeof(sigset_t)==128,"pinned masks");
    sigset_t original,empty,after;CHECK(!sigemptyset(&empty));
    CHECK(!pthread_sigmask(SIG_SETMASK,&empty,&original));
    util_semaphore_init(&complete,0);
#ifdef UTIL_FAULT
    util_barrier_init(&barrier,0);__builtin_trap();
#endif
    util_barrier_init(&barrier,
#ifdef UTIL_CANCEL
                      9
#else
                      8
#endif
                      );
    thrd_t threads[8];
    for(unsigned i=0;i<8;++i){CHECK(u_thread_create(&threads[i],worker,(void *)(uintptr_t)i)==thrd_success);
        CHECK(!pthread_sigmask(SIG_SETMASK,0,&after));
        for(unsigned j=0;j<16;++j)CHECK(after.bits[j]==0);}
#ifdef UTIL_CANCEL
    while(__atomic_load_n(&ready,__ATOMIC_ACQUIRE)!=8){
        struct timespec delay={0,1000000};CHECK(!thrd_sleep(&delay,0));}
    const char cancel[]="native Mesa util barrier cancel ready\n";message(cancel,sizeof(cancel)-1);
    CHECK(thrd_join(threads[0],0)==thrd_success);__builtin_trap();
#endif
    util_semaphore_wait(&complete);
    for(unsigned i=0;i<8;++i){int result;CHECK(thrd_join(threads[i],&result)==thrd_success&&result==(int)i+80);}
    for(unsigned wave=0;wave<48;++wave)CHECK(serial[wave]==1);
    CHECK(dispatch_identity==0xabcdef);
    CHECK(util_get_current_cpu()==-1&&util_thread_get_time_nano(thrd_current())==0);
    const uint32_t affinity=1;CHECK(!util_set_current_thread_affinity(&affinity,0,32));
    util_barrier_destroy(&barrier);util_semaphore_destroy(&complete);
    CHECK(!pthread_sigmask(SIG_SETMASK,&original,0));
    const char passed[]="native Mesa util barrier/semaphore/TLS/masks/names PASS\n";
    message(passed,sizeof(passed)-1);return 71;
}
