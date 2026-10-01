#include <cstdint>
#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "rwlock.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"rwlock SDK failed: %s\n",#x);__builtin_trap();}}while(0)
#define ALIAS(t) static_assert(__is_same(std::t,::t),"C++ integer alias")
ALIAS(int8_t);ALIAS(int16_t);ALIAS(int32_t);ALIAS(int64_t);
ALIAS(uint8_t);ALIAS(uint16_t);ALIAS(uint32_t);ALIAS(uint64_t);
ALIAS(int_least8_t);ALIAS(int_least16_t);ALIAS(int_least32_t);ALIAS(int_least64_t);
ALIAS(uint_least8_t);ALIAS(uint_least16_t);ALIAS(uint_least32_t);ALIAS(uint_least64_t);
ALIAS(int_fast8_t);ALIAS(int_fast16_t);ALIAS(int_fast32_t);ALIAS(int_fast64_t);
ALIAS(uint_fast8_t);ALIAS(uint_fast16_t);ALIAS(uint_fast32_t);ALIAS(uint_fast64_t);
ALIAS(intptr_t);ALIAS(uintptr_t);ALIAS(intmax_t);ALIAS(uintmax_t);
static struct {std::uint64_t before;u_rwlock lock;std::uint64_t after;} guarded;
#ifndef RWLOCK_CANCEL
static pthread_mutex_t control=PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t condition;
static int readers,release_readers,writer_started,writer_entered;
static unsigned counter,mirror,writers;
#endif
static int fail_clock;
extern "C" int __real_clock_gettime(int,struct timespec *);
extern "C" int __wrap_clock_gettime(int clock,struct timespec *out)
{if(fail_clock){errno=EIO;return -1;}return __real_clock_gettime(clock,out);}
#ifndef RWLOCK_CANCEL
static void wait_for(int *flag,int value)
{
    struct timespec deadline;CHECK(!clock_gettime(CLOCK_REALTIME,&deadline));deadline.tv_sec+=3;
    while(*flag!=value)CHECK(!pthread_cond_timedwait(&condition,&control,&deadline));
}
static void *reader(void *)
{
    CHECK(!u_rwlock_rdlock(&guarded.lock));
    CHECK(!pthread_mutex_lock(&control));++readers;CHECK(!pthread_cond_broadcast(&condition));
    while(!release_readers)CHECK(!pthread_cond_wait(&condition,&control));
    CHECK(!pthread_mutex_unlock(&control)&&!u_rwlock_rdunlock(&guarded.lock));return 0;
}
static void *writer(void *)
{
    CHECK(!pthread_mutex_lock(&control));writer_started=1;
    CHECK(!pthread_cond_broadcast(&condition)&&!pthread_mutex_unlock(&control));
    CHECK(!u_rwlock_wrlock(&guarded.lock));__atomic_store_n(&writer_entered,1,__ATOMIC_RELEASE);
    CHECK(!u_rwlock_wrunlock(&guarded.lock));return 0;
}
static void *stress(void *raw)
{
    unsigned id=(unsigned)(std::uintptr_t)raw;
    for(unsigned i=0;i<64;++i){
        if(id<2){
            CHECK(!u_rwlock_wrlock(&guarded.lock));CHECK(++writers==1);++counter;
            struct timespec pause={0,1};CHECK(!clock_nanosleep(CLOCK_MONOTONIC,0,&pause,0));
            mirror=counter^0x5a5a;CHECK(!--writers);CHECK(!u_rwlock_wrunlock(&guarded.lock));
        }else{
            CHECK(!u_rwlock_rdlock(&guarded.lock));CHECK(!writers&&mirror==(counter^0x5a5a));
            CHECK(!u_rwlock_rdunlock(&guarded.lock));
        }
    }
    return raw;
}
#endif
#ifdef RWLOCK_CANCEL
static void *blocked(void *)
{puts("rwlock blocked reader ready");u_rwlock_rdlock(&guarded.lock);__builtin_trap();}
#endif
extern "C" int _start(void)
{
    memset(&guarded,0xa5,sizeof(guarded));CHECK(!u_rwlock_init(&guarded.lock));
#ifdef RWLOCK_CANCEL
    CHECK(!u_rwlock_wrlock(&guarded.lock));pthread_t t;
    CHECK(!pthread_create(&t,0,blocked,0));pthread_join(t,0);__builtin_trap();
#else
    CHECK(!pthread_cond_init(&condition,0));
    errno=47;CHECK(pthread_rwlock_init(0,0)==EINVAL&&errno==47);
    CHECK(!pthread_rwlock_rdlock(&guarded.lock.rwlock));
    CHECK(!pthread_rwlock_rdlock(&guarded.lock.rwlock));
    CHECK(u_rwlock_destroy(&guarded.lock)==EBUSY&&errno==47);
    CHECK(!u_rwlock_rdunlock(&guarded.lock)&&!u_rwlock_rdunlock(&guarded.lock));
    CHECK(u_rwlock_rdunlock(&guarded.lock)==1&&errno==47);
    pthread_t t[8];CHECK(!pthread_create(&t[0],0,reader,0)&&!pthread_create(&t[1],0,reader,0));
    CHECK(!pthread_mutex_lock(&control));wait_for(&readers,2);CHECK(!pthread_mutex_unlock(&control));
    CHECK(!pthread_create(&t[2],0,writer,0));
    CHECK(!pthread_mutex_lock(&control));wait_for(&writer_started,1);CHECK(!pthread_mutex_unlock(&control));
    struct timespec pause={0,20000000};CHECK(!clock_nanosleep(CLOCK_MONOTONIC,0,&pause,0));
    CHECK(!__atomic_load_n(&writer_entered,__ATOMIC_ACQUIRE));
    CHECK(!pthread_mutex_lock(&control));release_readers=1;
    CHECK(!pthread_cond_broadcast(&condition)&&!pthread_mutex_unlock(&control));
    for(unsigned i=0;i<3;++i)CHECK(!pthread_join(t[i],0));
    CHECK(writer_entered);
    mirror=counter^0x5a5a;
    for(unsigned i=0;i<8;++i)CHECK(!pthread_create(&t[i],0,stress,(void *)(std::uintptr_t)i));
    for(unsigned i=0;i<8;++i)CHECK(!pthread_join(t[i],0));
    CHECK(counter==128);
    CHECK(!u_rwlock_destroy(&guarded.lock));
    CHECK(guarded.before==UINT64_C(0xa5a5a5a5a5a5a5a5)&&guarded.after==guarded.before);
    CHECK(!pthread_cond_destroy(&condition)&&!pthread_mutex_destroy(&control));
    struct timespec before,after;CHECK(!clock_gettime(CLOCK_REALTIME,&before));
    struct {std::uint64_t before;time_t value;std::uint64_t after;} output={123,7,456};
    time_t result=time(&output.value);CHECK(!clock_gettime(CLOCK_REALTIME,&after));
    CHECK(result==output.value&&result>=before.tv_sec&&result<=after.tv_sec);
    CHECK(output.before==123&&output.after==456&&time(0)>=result);
    fail_clock=1;output.value=7;errno=0;
    CHECK(time(&output.value)==-1&&errno==EIO&&output.value==7&&output.before==123&&output.after==456);
    fail_clock=0;CHECK(time(0)>=result);
    puts("native Mesa rwlock/shared readers/exclusive writers/time/C++ ABI PASS");return 87;
#endif
}
