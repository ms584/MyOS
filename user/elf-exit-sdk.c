#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <stdint.h>
#include <unistd.h>
#define CHECK(x) do{if(!(x)){puts("exit check failed: " #x);__builtin_trap();}}while(0)
static unsigned step;
static int ends[2];
static unsigned ready;
static void first(void){CHECK(step==2);step=3;}
static void second(void)
{
    CHECK(step==1);step=2;
#ifdef RECURSIVE_EXIT
    exit(78);
#endif
}
static void prepare(void){CHECK(!atexit(first)&&!atexit(second));}
__attribute__((constructor(101))) static void initialize(void)
{
    step=1;
#ifdef CTOR_EXIT
    prepare();exit(77);
#endif
}
__attribute__((destructor(300))) static void last_array(void)
{
#ifdef RETURN_FINI
    extern void __cxa_finalize(void *);__cxa_finalize(0);
#endif
    CHECK(step==3);step=4;
#ifdef LOOP_FINI
    puts("native exit looping finalizer ready");
    for(;;){struct timespec t={1,0};clock_nanosleep(CLOCK_MONOTONIC,0,&t,0);}
#endif
#if defined(FINI_EXIT) || defined(RETURN_FINI)
    exit(78);
#endif
}
__attribute__((destructor(200))) static void earlier_array(void){CHECK(step==4);step=5;}
void image_fini(void){CHECK(step==5);step=6;puts("native exit finalizers PASS");}
static void *blocked(void *arg)
{(void)arg;char byte;__atomic_store_n(&ready,1,__ATOMIC_RELEASE);
 CHECK(read(ends[0],&byte,1)<0);__builtin_trap();}
static void *terminate(void *arg)
{
    (void)arg;
#ifdef WORKER_RAW
    _Exit(-77);
#else
    exit(77);
#endif
}
int _start(void)
{
    CHECK(step==1);prepare();CHECK(!pipe(ends));
    CHECK(malloc(9000));pthread_t sleeper,exiter;
    CHECK(!pthread_create(&sleeper,0,blocked,0));
    struct timespec delay={0,2000000};
    while(!__atomic_load_n(&ready,__ATOMIC_ACQUIRE))clock_nanosleep(CLOCK_MONOTONIC,0,&delay,0);
    CHECK(!clock_nanosleep(CLOCK_MONOTONIC,0,&delay,0));
#if defined(WORKER_EXIT) || defined(WORKER_RAW)
    CHECK(!pthread_create(&exiter,0,terminate,0));pthread_join(exiter,0);
#else
    (void)exiter;(void)terminate;
#ifdef RETURN_FINI
    return 77;
#endif
#ifdef RAW_EXIT
    _Exit(-77);
#else
    exit(77);
#endif
#endif
    __builtin_trap();
}
