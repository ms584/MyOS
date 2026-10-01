#include <stdint.h>
struct timespec { int64_t tv_sec,tv_nsec; };
/* Pinned Cobalt musl opaque mutex capacity/alignment, not host pthread types. */
typedef struct { uint64_t words[10]; } mutex_t;
extern int pthread_mutex_init(void *,const void *),pthread_mutex_destroy(void *);
extern int pthread_mutex_lock(void *),pthread_mutex_unlock(void *);
extern int pthread_mutex_timedlock(void *,const struct timespec *);
extern int pthread_mutexattr_init(void *),pthread_mutexattr_settype(void *,int);
extern int pthread_create(uintptr_t *,const void *,void *(*)(void *),void *);
extern int pthread_join(uintptr_t,void **),clock_gettime(int,struct timespec *);
extern int clock_nanosleep(int,int,const struct timespec *,struct timespec *);
extern int *__errno_location(void);
#ifndef CANCEL_WAIT
#define CANCEL_WAIT 0
#endif
extern _Noreturn void pthread_exit(void *);
static mutex_t mutex;
static unsigned ready,mode;
static void check(int okay){if(!okay)__builtin_trap();}
static void print(const char *message,unsigned length)
{
    register uint64_t a __asm__("rax")=1,d __asm__("rdi")=(uintptr_t)message;
    register uint64_t s __asm__("rsi")=length;
    __asm__ volatile("int $0x80":"+a"(a):"D"(d),"S"(s):"rdx","memory","cc");
}
static uint64_t now(int clock)
{struct timespec t;check(!clock_gettime(clock,&t));return (uint64_t)t.tv_sec*1000000000ull+t.tv_nsec;}
static struct timespec deadline(uint64_t nanos)
{return (struct timespec){(int64_t)(nanos/1000000000ull),(int64_t)(nanos%1000000000ull)};}
static void *worker(void *unused)
{
    (void)unused;struct timespec end=deadline(now(0)+(CANCEL_WAIT?30000000000ull:500000000ull));
    __atomic_store_n(&ready,1,__ATOMIC_RELEASE);
    if(CANCEL_WAIT){const char message[]="native timed mutex cancel ready\n";print(message,sizeof(message)-1);}
    int rc=pthread_mutex_timedlock(&mutex,&end);
    check(rc==(mode?110:0));if(!rc)check(!pthread_mutex_unlock(&mutex));
    return (void *)(uintptr_t)rc;
}
int _start(void)
{
    check(!pthread_mutex_init(&mutex,0));*__errno_location()=123;
    if(CANCEL_WAIT){uintptr_t thread;check(!pthread_mutex_lock(&mutex));
        check(!pthread_create(&thread,0,worker,0));pthread_exit(0);}
    struct timespec bad={0,1000000000},past={-1,0},overflow={18446744073ll,709551616};
    check(!pthread_mutex_timedlock(&mutex,&bad));check(!pthread_mutex_unlock(&mutex));
    check(!pthread_mutex_timedlock(&mutex,0));check(!pthread_mutex_unlock(&mutex));
    check(!pthread_mutex_lock(&mutex));
    check(pthread_mutex_timedlock(&mutex,&bad)==22);
    check(pthread_mutex_timedlock(&mutex,0)==22);
    check(pthread_mutex_timedlock(&mutex,&overflow)==22);
    check(pthread_mutex_timedlock(&mutex,&past)==110);
    struct timespec monotonic=deadline(now(1)+1000000000ull);
    check(pthread_mutex_timedlock(&mutex,&monotonic)==110);
    uint64_t begin=now(1);struct timespec end=deadline(now(0)+100000000ull);
    check(pthread_mutex_timedlock(&mutex,&end)==110);
    uint64_t elapsed=now(1)-begin;check(elapsed>=90000000ull&&elapsed<1000000000ull);
    check(*__errno_location()==123);
    check(clock_nanosleep(1,0,&overflow,0)==22);
    check(clock_nanosleep(1,1,&overflow,0)==22);
    uintptr_t thread;ready=0;mode=0;
    check(!pthread_create(&thread,0,worker,0));
    struct timespec delay={0,10000000};
    while(!__atomic_load_n(&ready,__ATOMIC_ACQUIRE))check(!clock_nanosleep(1,0,&delay,0));
    check(!clock_nanosleep(1,0,&delay,0));check(!pthread_mutex_unlock(&mutex));
    void *value;check(!pthread_join(thread,&value)&&!value);
    check(!pthread_mutex_lock(&mutex));ready=0;mode=1;
    check(!pthread_create(&thread,0,worker,0));
    check(!pthread_join(thread,&value)&&value==(void *)110);
    check(!pthread_mutex_unlock(&mutex));check(!pthread_mutex_destroy(&mutex));
    uint32_t attr;check(!pthread_mutexattr_init(&attr));check(!pthread_mutexattr_settype(&attr,1));
    check(!pthread_mutex_init(&mutex,&attr));check(!pthread_mutex_lock(&mutex));
    check(!pthread_mutex_timedlock(&mutex,&past));
    check(!pthread_mutex_unlock(&mutex));check(!pthread_mutex_unlock(&mutex));
    check(!pthread_mutexattr_settype(&attr,2));check(!pthread_mutex_init(&mutex,&attr));
    check(!pthread_mutex_lock(&mutex));check(pthread_mutex_timedlock(&mutex,&past)==35);
    check(!pthread_mutex_unlock(&mutex));check(!pthread_mutex_destroy(&mutex));
    check(pthread_mutex_timedlock(0,&bad)==22&&pthread_mutex_lock(0)==22);
    const char message[]="native timed mutex realtime/overflow/contention/errno PASS\n";
    print(message,sizeof(message)-1);
    return 0;
}
