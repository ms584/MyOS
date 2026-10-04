/* Real upstream gthread calls; no substitute std::mutex or container classes. */
#include <bits/c++config.h>
#include <bits/gthr.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
/* Test the thread adapter independently of still-incomplete upstream cstdlib.
 * This is the real already-tested abort provider, not a replacement library. */
extern "C" void abort(void) __attribute__((noreturn));
#ifndef _GLIBCXX_HAS_GTHREADS
#error Target configuration has not proved gthread availability
#endif
#if _GLIBCXX_GTHREAD_USE_WEAK
#error MyOS locking must not disappear behind weak-zero bindings
#endif
typedef char gthread_mutex_cond_sizes[(sizeof(__gthread_mutex_t)==80 && sizeof(__gthread_cond_t)==96)?1:-1];
typedef char gthread_once_size[(sizeof(__gthread_once_t)==64)?1:-1];
static __gthread_mutex_t mutex=__GTHREAD_MUTEX_INIT;
static __gthread_recursive_mutex_t recursive;
static __gthread_cond_t cond;
static __gthread_once_t once=__GTHREAD_ONCE_INIT;
static __gthread_key_t key;
static int count,ready,release,once_calls,destructors;
static void message(const char *text) { size_t n=0;while(text[n])++n;write(1,text,n);write(1,"\n",1); }
static void require(bool value) { if(!value) { message("native gthread assertion failed"); abort(); } }
static void initialize() { ++once_calls; }
static void destroy(void *p) { require(p==(void *)1); __atomic_fetch_add(&destructors,1,__ATOMIC_SEQ_CST); }
static long long ms(const timespec &t) { return t.tv_sec*1000LL+t.tv_nsec/1000000; }
static void *worker(void *arg) {
    require(__gthread_once(&once,initialize)==0);
    require(__gthread_setspecific(key,(void *)1)==0);
    require(__gthread_getspecific(key)==(void *)1);
    for(int i=0;i<1000;++i) {
        require(__gthread_mutex_lock(&mutex)==0);
        int old=count;
        if(!(i%31))require(__gthread_yield()==0);
        count=old+1;
        require(__gthread_mutex_unlock(&mutex)==0);
    }
    require(__gthread_mutex_lock(&mutex)==0);
    ++ready; require(__gthread_cond_broadcast(&cond)==0);
    while(!release) require(__gthread_cond_wait(&cond,&mutex)==0);
    require(__gthread_mutex_unlock(&mutex)==0);
    return arg;
}
extern "C" int _start() {
    require(__gthread_active_p());
    __GTHREAD_COND_INIT_FUNCTION(&cond);
    require(__GTHREAD_RECURSIVE_MUTEX_INIT_FUNCTION(&recursive)==0);
    require(__gthread_recursive_mutex_lock(&recursive)==0);
    require(__gthread_recursive_mutex_lock(&recursive)==0);
    require(__gthread_recursive_mutex_trylock(&recursive)==0);
    for(int i=0;i<3;++i)require(__gthread_recursive_mutex_unlock(&recursive)==0);
    require(__gthread_recursive_mutex_destroy(&recursive)==0);
    require(__gthread_mutex_lock(&mutex)==0);
    timespec deadline,before,after;
    require(clock_gettime(CLOCK_REALTIME,&deadline)==0);
    require(clock_gettime(CLOCK_MONOTONIC,&before)==0);
    deadline.tv_nsec+=20000000;
    if(deadline.tv_nsec>=1000000000){++deadline.tv_sec;deadline.tv_nsec-=1000000000;}
    require(__gthread_cond_timedwait(&cond,&mutex,&deadline)==ETIMEDOUT);
    require(clock_gettime(CLOCK_MONOTONIC,&after)==0);
    require(ms(after)-ms(before)>=15);
    require(__gthread_mutex_unlock(&mutex)==0);
    require(__gthread_key_create(&key,destroy)==0);
    __gthread_t threads[8];
    for(unsigned long i=0;i<8;++i)require(__gthread_create(&threads[i],worker,(void *)(i+1))==0);
    require(__gthread_equal(__gthread_self(),__gthread_self()));
    require(__gthread_mutex_lock(&mutex)==0);
    while(ready!=8)require(__gthread_cond_wait(&cond,&mutex)==0);
#ifdef GTHREAD_CANCEL
    message("native upstream gthread blocked workers cancel ready");
    for(;;)require(__gthread_cond_wait(&cond,&mutex)==0);
#else
    require(count==8000 && once_calls==1);
    release=1;require(__gthread_cond_broadcast(&cond)==0);
    require(__gthread_mutex_unlock(&mutex)==0);
    for(unsigned long i=0;i<8;++i) { void *value=0;
        require(__gthread_join(threads[i],&value)==0 && value==(void *)(i+1)); }
    require(destructors==8);
    require(__gthread_key_delete(key)==0);
    require(__gthread_cond_destroy(&cond)==0);
    require(__gthread_mutex_destroy(&mutex)==0);
    message("native upstream gthread strong mutex/cond/once/key/timed wait PASS");
    return 97;
#endif
}
