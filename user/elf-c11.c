#include "c11/threads.h"
#include <stdint.h>
#include <errno.h>
#define CHECK(x) do { if(!(x))__builtin_trap(); } while(0)
static mtx_t mutex;
static cnd_t condition;
static once_flag once=ONCE_FLAG_INIT;
static tss_t key;
static unsigned initialized,ready,released,destructors;
static thrd_t main_thread;
static void message(const char *text,unsigned length)
{
    register uint64_t a __asm__("rax")=1,d __asm__("rdi")=(uintptr_t)text;
    register uint64_t s __asm__("rsi")=length;
    __asm__ volatile("int $0x80":"+a"(a):"D"(d),"S"(s):"rdx","memory","cc");
}
static void initialize(void){++initialized;}
static void destructor(void *raw)
{
    uintptr_t value=(uintptr_t)raw;
    __atomic_add_fetch(&destructors,1,__ATOMIC_SEQ_CST);
    if(value<100)CHECK(tss_set(key,(void *)(value+100))==thrd_success);
}
static int worker(void *raw)
{
    unsigned who=(unsigned)(uintptr_t)raw;
    CHECK(thrd_equal(thrd_current(),main_thread)==0);
    CHECK(tss_get(key)==0&&tss_set(key,(void *)(uintptr_t)(who+1))==thrd_success);
    call_once(&once,initialize);
    CHECK(mtx_lock(&mutex)==thrd_success);
    ++ready;CHECK(cnd_broadcast(&condition)==thrd_success);
    while(!released)CHECK(cnd_wait(&condition,&mutex)==thrd_success);
    CHECK(mtx_unlock(&mutex)==thrd_success);
    if(who&1)thrd_exit(-(int)who-10);
    return (int)who+10;
}
int _start(void)
{
    _Static_assert(sizeof(mtx_t)==80&&sizeof(cnd_t)==96&&sizeof(once_flag)==64,"pinned C11 types");
    _Static_assert(mtx_plain==1&&mtx_recursive==2&&mtx_timed==4,"Mesa flags");
    main_thread=thrd_current();CHECK(thrd_equal(main_thread,main_thread));
    CHECK(mtx_init(&mutex,99)==thrd_error);
    CHECK(mtx_init(&mutex,mtx_plain)==thrd_success&&cnd_init(&condition)==thrd_success);
    CHECK(tss_create(&key,destructor)==thrd_success);
    thrd_t threads[8];
    for(unsigned i=0;i<8;++i)CHECK(thrd_create(&threads[i],worker,(void *)(uintptr_t)i)==thrd_success);
    CHECK(mtx_lock(&mutex)==thrd_success);
    while(ready<8)CHECK(cnd_wait(&condition,&mutex)==thrd_success);
    thrd_t extra=(void *)0x123;CHECK(thrd_create(&extra,worker,0)==thrd_error&&extra==(void *)0x123);
#ifdef C11_CANCEL
    CHECK(mtx_unlock(&mutex)==thrd_success);
    const char cancel[]="native C11 blocked cancel ready\n";
    message(cancel,sizeof(cancel)-1);
    CHECK(thrd_join(threads[0],0)==thrd_success);
    __builtin_trap();
#endif
    released=1;CHECK(cnd_broadcast(&condition)==thrd_success&&mtx_unlock(&mutex)==thrd_success);
    for(unsigned i=0;i<8;++i){int result=0;CHECK(thrd_join(threads[i],&result)==thrd_success);
        CHECK(result==((i&1)?-(int)i-10:(int)i+10));}
    CHECK(initialized==1&&destructors==16);
    mtx_destroy(&mutex);
    CHECK(thrd_create(&extra,0,0)==thrd_error);
    errno=123;struct timespec invalid={0,1000000000},remain={9,9};
    CHECK(thrd_sleep(&invalid,&remain)==-2&&errno==EINVAL);
    CHECK(remain.tv_sec==9&&remain.tv_nsec==9);
    struct timespec short_wait={0,20000000};CHECK(thrd_sleep(&short_wait,&remain)==0);
    CHECK(remain.tv_sec==0&&remain.tv_nsec==0);
    struct timespec now;CHECK(timespec_get(&now,TIME_UTC)==TIME_UTC&&now.tv_sec>1000000000);
    CHECK(timespec_get(&now,TIME_MONOTONIC)==TIME_MONOTONIC);
    now=(struct timespec){55,66};CHECK(timespec_get(&now,TIME_ACTIVE)==0&&now.tv_sec==55&&now.tv_nsec==66);
    CHECK(mtx_init(&mutex,mtx_timed|mtx_recursive)==thrd_success);
    CHECK(mtx_lock(&mutex)==thrd_success&&mtx_timedlock(&mutex,&invalid)==thrd_success);
    CHECK(mtx_unlock(&mutex)==thrd_success&&mtx_unlock(&mutex)==thrd_success);
    mtx_destroy(&mutex);CHECK(mtx_init(&mutex,mtx_timed)==thrd_success);
    CHECK(mtx_lock(&mutex)==thrd_success);
    CHECK(mtx_trylock(&mutex)==thrd_busy);
    CHECK(timespec_get(&now,TIME_UTC)==TIME_UTC);now.tv_nsec+=30000000;
    if(now.tv_nsec>=1000000000){++now.tv_sec;now.tv_nsec-=1000000000;}
    CHECK(mtx_timedlock(&mutex,&now)==thrd_timedout);
    CHECK(cnd_timedwait(&condition,&mutex,&now)==thrd_timedout);
    CHECK(mtx_unlock(&mutex)==thrd_success);
    pthread_condattr_t attr;CHECK(!pthread_condattr_init(&attr));
    CHECK(!pthread_condattr_setclock(&attr,CLOCK_MONOTONIC));
    cnd_destroy(&condition);CHECK(!pthread_cond_init(&condition,&attr));
    CHECK(!pthread_condattr_destroy(&attr));
    CHECK(mtx_lock(&mutex)==thrd_success);
    CHECK(timespec_get(&now,TIME_MONOTONIC)==TIME_MONOTONIC);
    CHECK(cnd_timedwait(&condition,&mutex,&now)==thrd_timedout);
    CHECK(mtx_unlock(&mutex)==thrd_success);
    mtx_destroy(&mutex);cnd_destroy(&condition);tss_delete(key);
    const char passed[]="native Mesa C11 pthread ABI/exit/once/TSS/timed waits PASS\n";
    message(passed,sizeof(passed)-1);
    return 70;
}
