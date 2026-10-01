/* Actual compiler-emitted guard calls; no libstdc++ fallback implementation. */
#include <stdint.h>
#ifdef MYOS_CXX_GUARD_HOST_TEST
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
extern "C" unsigned guard_test_blocks;
static void pause_tick(){usleep(20000);}
#define require(condition) assert(condition)
#else
typedef void *pthread_t;
extern "C" int pthread_create(pthread_t *,const void *,void *(*)(void *),void *);
extern "C" int pthread_join(pthread_t,void **);
static_assert(sizeof(pthread_t)==8,"pinned Cobalt pthread identity");
struct timespec {int64_t tv_sec,tv_nsec;};
extern "C" int clock_nanosleep(int,int,const timespec *,timespec *);
static void pause_tick(){timespec t={0,20000000};clock_nanosleep(1,0,&t,0);}
static void require(bool condition){if(!condition)__builtin_trap();}
#endif
extern "C" int __cxa_guard_acquire(uint64_t *);
extern "C" void __cxa_guard_release(uint64_t *),__cxa_guard_abort(uint64_t *);
static unsigned constructions;
static uint64_t payload[16];
#ifndef GUARD_MODE
#define GUARD_MODE 0
#endif
static uint64_t contested;
static unsigned ready,retries;
static void *retry_worker(void *)
{
    __atomic_add_fetch(&ready,1,__ATOMIC_RELEASE);
    if(__cxa_guard_acquire(&contested)){
        __atomic_add_fetch(&retries,1,__ATOMIC_RELAXED);
        pause_tick();
        payload[0]=0xabcddcba;
        __cxa_guard_release(&contested);
    }
    require(payload[0]==0xabcddcba);
    return 0;
}
static int initialize()
{
    __atomic_add_fetch(&constructions,1,__ATOMIC_RELAXED);
    pause_tick();
    for(unsigned i=0;i<16;++i)payload[i]=0x12340000+i;
    return 731;
}
static int instance(){static int value=initialize();return value;}
static void *worker(void *)
{
    for(unsigned n=0;n<50;++n){
        require(instance()==731);
        for(unsigned i=0;i<16;++i)require(payload[i]==0x12340000+i);
    }
    return (void *)731;
}
static void exercise()
{
    /* Actual sleepers must retry after an aborted initializer, not observe
     * completion or lose the wakeup while the next initializer takes over. */
    for(unsigned wave=0;wave<3;++wave){
        contested=0;ready=0;retries=0;payload[0]=0;
        require(__cxa_guard_acquire(&contested)==1);
        pthread_t retry_threads[4];
        for(unsigned i=0;i<4;++i)require(!pthread_create(&retry_threads[i],0,retry_worker,0));
        while(__atomic_load_n(&ready,__ATOMIC_ACQUIRE)!=4)pause_tick();
        pause_tick();pause_tick();
        require(!(contested&255));
        __cxa_guard_abort(&contested);
        for(unsigned i=0;i<4;++i)require(!pthread_join(retry_threads[i],0));
        require(retries==1 && (contested&255)==1);
    }
    pthread_t threads[4];
    for(unsigned i=0;i<4;++i)require(!pthread_create(&threads[i],0,worker,0));
    for(unsigned i=0;i<4;++i){void *result=0;require(!pthread_join(threads[i],&result));require(result==(void *)731);}
    require(constructions==1 && instance()==731);
    uint64_t guard=0;
    require(__cxa_guard_acquire(&guard)==1 && !(guard&255));
    __cxa_guard_abort(&guard);require(!guard);
    require(__cxa_guard_acquire(&guard)==1 && !(guard&255));
    __cxa_guard_release(&guard);require((guard&255)==1);
    require(__cxa_guard_acquire(&guard)==0);
}
#ifdef MYOS_CXX_GUARD_HOST_TEST
int main(){exercise();require(guard_test_blocks>0);puts("PASS: HOST ONLY compiler C++ guard contention/publication/abort/retry; no native claim");}
#else
extern "C" int _start()
{
#if GUARD_MODE == 1
    require(__cxa_guard_acquire(&contested)==1);
    __cxa_guard_acquire(&contested); /* recursion: owned user fault */
    return 1;
#elif GUARD_MODE == 2
    require(__cxa_guard_acquire(&contested)==1);
    pthread_t threads[4];
    for(unsigned i=0;i<4;++i)require(!pthread_create(&threads[i],0,retry_worker,0));
    while(__atomic_load_n(&ready,__ATOMIC_ACQUIRE)!=4)pause_tick();
    pause_tick();pause_tick();
    const char message[]="C++ guard blocked cancel ready\n";
    register uint64_t a __asm__("rax")=1,d __asm__("rdi")=(uintptr_t)message;
    register uint64_t s __asm__("rsi")=sizeof(message)-1;
    __asm__ volatile("int $0x80":"+a"(a):"D"(d),"S"(s):"rdx","memory","cc");
    for(;;)pause_tick();
#else
    exercise();
    const char message[]="native compiler C++ guard contention/publication/abort/retry PASS\n";
    register uint64_t a __asm__("rax")=1,d __asm__("rdi")=(uintptr_t)message;
    register uint64_t s __asm__("rsi")=sizeof(message)-1;
    __asm__ volatile("int $0x80":"+a"(a):"D"(d),"S"(s):"rdx","memory","cc");
    return 61;
#endif
}
#endif
