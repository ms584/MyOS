#include <stddef.h>
#include <stdint.h>

struct timespec { int64_t tv_sec,tv_nsec; };
extern int clock_gettime(int,struct timespec *),clock_nanosleep(int,int,const struct timespec *,struct timespec *);
extern int pthread_attr_init(void *),pthread_attr_destroy(void *),pthread_attr_setstacksize(void *,size_t);
extern int pthread_attr_getstacksize(const void *,size_t *),pthread_attr_getstack(const void *,void **,size_t *);
extern int pthread_attr_setdetachstate(void *,int);
extern int pthread_create(uint64_t *,const void *,void *(*)(void *),void *),pthread_join(uint64_t,void **);
extern int pthread_detach(uint64_t),pthread_getattr_np(uint64_t,void *);
extern int pthread_mutex_init(void *,const void *),pthread_mutex_lock(void *),pthread_mutex_trylock(void *);
extern int pthread_mutex_unlock(void *),pthread_mutex_destroy(void *);
extern int pthread_cond_init(void *,const void *),pthread_cond_timedwait(void *,void *,const struct timespec *);
extern int pthread_cond_signal(void *),pthread_cond_broadcast(void *),pthread_cond_destroy(void *);
extern int pthread_cond_wait(void *,void *);
extern int sem_init(void *,int,unsigned),sem_wait(void *),sem_post(void *),sem_destroy(void *);
extern int sem_timedwait(void *,const struct timespec *);
extern int pthread_rwlock_init(void *,const void *),pthread_rwlock_rdlock(void *);
extern int pthread_rwlock_wrlock(void *),pthread_rwlock_unlock(void *),pthread_rwlock_destroy(void *);
extern int pthread_once(volatile int *,void (*)(void));
extern int pthread_key_create(void **,void (*)(void *)),pthread_key_delete(void *);
extern int pthread_setspecific(void *,const void *);extern void *pthread_getspecific(void *);
extern uint64_t pthread_self(void);extern int pthread_setname_np(uint64_t,const char *);
extern int pthread_getname_np(uint64_t,char *,size_t),pthread_kill(uint64_t,int);
extern int pthread_getschedparam(uint64_t,int *,void *);
extern int pthread_mutexattr_init(void *),pthread_mutexattr_settype(void *,int),pthread_mutexattr_destroy(void *);
extern int pthread_condattr_init(void *),pthread_condattr_setclock(void *,int),pthread_condattr_destroy(void *);
extern int pthread_sigmask(int,const void *,void *);
extern int *__errno_location(void);

static uint64_t mutex_words[5],condition_words[6],semaphore_words[4],rwlock_words[7];
static void *key;
static unsigned counter,destructed,once_count;
static void *full_keys[32],*rearm_key;
static unsigned rearm_count;
static void rearm_destructor(void *value)
{++rearm_count;if(rearm_count<5)(void)pthread_setspecific(rearm_key,value);}
static void *key_worker(void *unused)
{
    (void)unused;
    for(unsigned i=0;i<32;++i)if(pthread_getspecific(full_keys[i]))return (void *)1;
    if(pthread_setspecific(full_keys[0],(void *)0x4567))return (void *)2;
    return 0;
}
static void *rearm_worker(void *unused)
{(void)unused;return (void *)(uintptr_t)pthread_setspecific(rearm_key,(void *)1);}
static volatile int once_control;
static int overflow_errno[2];
static void *mask_worker(void *unused)
{
    (void)unused;uint64_t mask[16];for(unsigned i=0;i<16;++i)mask[i]=0x55;
    if(pthread_sigmask(0,0,mask)||mask[0]!=(1ull<<9)||mask[1]!=(1ull<<62))return (void *)1;
    for(unsigned i=2;i<16;++i)if(mask[i])return (void *)2;
    return 0;
}
static void *overflow_worker(void *raw)
{
    unsigned index=(unsigned)(uintptr_t)raw;int result=sem_post(semaphore_words);
    overflow_errno[index]=*__errno_location();return (void *)(uintptr_t)(result?1:0);
}
static void once_routine(void){++once_count;}
static void destructor(void *value){if(value)++destructed;}
static void *worker(void *raw)
{
    uintptr_t value=(uintptr_t)raw;
    if(pthread_setspecific(key,raw)||pthread_getspecific(key)!=raw)return (void *)1;
    if(pthread_mutex_lock(mutex_words))return (void *)2;
    for(unsigned i=0;i<1000;++i)++counter;
    if(pthread_mutex_unlock(mutex_words))return (void *)3;
    if(pthread_once(&once_control,once_routine))return (void *)4;
    if(sem_post(semaphore_words))return (void *)5;
    return (void *)(value+10);
}
static void *condition_worker(void *unused)
{
    (void)unused;
    if(pthread_mutex_lock(mutex_words)||sem_post(semaphore_words))return (void *)1;
    if(pthread_cond_wait(condition_words,mutex_words)||pthread_mutex_unlock(mutex_words))return (void *)2;
    if(sem_post(semaphore_words))return (void *)3;
    return 0;
}
static void *detached_worker(void *unused)
{
    (void)unused;sem_post(semaphore_words);return 0;
}
static void print(const char *s)
{
    size_t n=0;while(s[n])++n;
    register uint64_t a __asm__("rax")=1,d __asm__("rdi")=(uintptr_t)s,q __asm__("rsi")=n;
    __asm__ volatile("int $0x80":"+a"(a):"D"(d),"S"(q):"rdx","memory","cc");
}
uint64_t _start(void)
{
    uint64_t attr[7]={0},probe_attr[7]={0},threads[2],thread;void *results[2],*stack=0;size_t stack_size=0,probe_size=0;
    uint32_t mutex_attr[2]={0},cond_attr[2]={0};char name[16];int policy=-1,parameter=-1;
    struct timespec realtime,monotonic,before,after,pause={0,20000000};
    uint64_t mask[16]={5,9},old[16],check[16];
    _Static_assert(sizeof(mask)==128,"pinned musl_sigset_t storage");
    if(pthread_attr_init(attr)||pthread_attr_setstacksize(attr,65536)||
       pthread_attr_getstacksize(attr,&stack_size)||stack_size!=65536)return 1;
    if(pthread_mutexattr_init(mutex_attr)||pthread_mutexattr_settype(mutex_attr,1)||
       pthread_mutex_init(mutex_words,mutex_attr))return 2;
    if(pthread_mutex_lock(mutex_words)||pthread_mutex_lock(mutex_words)||
       pthread_mutex_unlock(mutex_words)||pthread_mutex_unlock(mutex_words))return 3;
    if(pthread_condattr_init(cond_attr)||pthread_condattr_setclock(cond_attr,1)||
       pthread_cond_init(condition_words,cond_attr)||sem_init(semaphore_words,0,0)||
       pthread_rwlock_init(rwlock_words,0)||pthread_key_create(&key,destructor))return 4;
    if(pthread_create(&threads[0],attr,worker,(void *)11)||
       pthread_create(&threads[1],attr,worker,(void *)22))return 5;
    if(sem_wait(semaphore_words)||sem_wait(semaphore_words))return 6;
    if(pthread_join(threads[0],&results[0])||pthread_join(threads[1],&results[1]))return 7;
    if(results[0]!=(void *)21||results[1]!=(void *)32||counter!=2000||
       once_count!=1||destructed!=2)return 8;
    if(pthread_rwlock_rdlock(rwlock_words)||pthread_rwlock_unlock(rwlock_words)||
       pthread_rwlock_wrlock(rwlock_words)||pthread_rwlock_unlock(rwlock_words))return 9;
    if(pthread_getattr_np(pthread_self(),probe_attr)||
       pthread_attr_getstack(probe_attr,&stack,&probe_size)||!stack||probe_size!=16384)return 17;
    if(pthread_create(&thread,attr,condition_worker,0)||sem_wait(semaphore_words))return 18;
    if(pthread_mutex_lock(mutex_words)||pthread_cond_signal(condition_words)||
       pthread_mutex_unlock(mutex_words)||sem_wait(semaphore_words)||
       pthread_join(thread,0))return 19;
    if(pthread_create(&thread,attr,detached_worker,0)||pthread_detach(thread)||
       sem_wait(semaphore_words))return 20;
    if(pthread_attr_setdetachstate(attr,1))return 21;
    for(unsigned i=0;i<10;++i)
        if(pthread_create(&thread,attr,detached_worker,0)||sem_wait(semaphore_words))return 22;
    if(pthread_attr_setdetachstate(attr,0))return 23;
    struct timespec expired_sem={0,0};
    if(sem_timedwait(semaphore_words,&expired_sem)!=-1)return 24;
    if(sem_post(semaphore_words)||sem_timedwait(semaphore_words,&expired_sem))return 25;
    if(clock_gettime(0,&realtime)||clock_gettime(1,&monotonic)||
       realtime.tv_sec<1700000000||monotonic.tv_sec<1)return 26;
    if(clock_gettime(1,&before)||clock_nanosleep(1,0,&pause,0)||clock_gettime(1,&after)||
       after.tv_sec<before.tv_sec||
       (after.tv_sec==before.tv_sec&&after.tv_nsec<=before.tv_nsec))return 27;
    if(pthread_sigmask(2,mask,old)||old[0]||old[1]||pthread_sigmask(0,0,check)||
       check[0]!=5||check[1]!=9)return 28;
    if(pthread_sigmask(1,mask,0)||pthread_sigmask(0,0,check)||check[0]||check[1])return 29;
    if(pthread_getschedparam(0x1234,&policy,&parameter)!=3||
       pthread_kill(0x1234,0)!=3||pthread_detach(0x1234)!=3)return 30;
    if(pthread_mutex_lock(mutex_words))return 31;
    struct timespec future=monotonic;future.tv_nsec+=30000000;
    if(future.tv_nsec>=1000000000){++future.tv_sec;future.tv_nsec-=1000000000;}
    if(pthread_cond_timedwait(condition_words,mutex_words,&future)!=110||
       pthread_mutex_unlock(mutex_words))return 32;
    if(pthread_mutex_lock(mutex_words))return 10;
    struct timespec expired={0,0};
    if(pthread_cond_timedwait(condition_words,mutex_words,&expired)!=110||
       pthread_mutex_unlock(mutex_words))return 11;
    if(pthread_cond_signal(condition_words)||pthread_cond_broadcast(condition_words))return 12;
    if(pthread_setname_np(pthread_self(),"main")||
       pthread_getname_np(pthread_self(),name,sizeof(name))||name[0]!='m')return 13;
    if(pthread_kill(pthread_self(),0)||pthread_getschedparam(pthread_self(),&policy,&parameter)||
       policy||parameter||pthread_sigmask(0,0,0))return 14;
    if(pthread_key_delete(key)||pthread_rwlock_destroy(rwlock_words)||
       sem_destroy(semaphore_words)||pthread_cond_destroy(condition_words)||
       pthread_mutex_destroy(mutex_words)||pthread_condattr_destroy(cond_attr)||
       pthread_mutexattr_destroy(mutex_attr)||pthread_attr_destroy(attr))return 15;
    if(pthread_mutex_trylock(mutex_words)||pthread_mutex_unlock(mutex_words))return 16;
    if(sem_init(0,0,0)!=-1||*__errno_location()!=22||
       sem_destroy(0)!=-1||*__errno_location()!=22||sem_post(0)!=-1||*__errno_location()!=22)return 41;
    if(sem_init(semaphore_words,0,0)||sem_init(semaphore_words,1,1)!=-1||*__errno_location()!=38||
       sem_init(semaphore_words,0,0xffffffffu)!=-1||*__errno_location()!=22)return 42;
    if(sem_timedwait(semaphore_words,&expired_sem)!=-1||*__errno_location()!=110)return 43;
    struct timespec invalid_sem={0,-1};
    if(sem_timedwait(semaphore_words,&invalid_sem)!=-1||*__errno_location()!=22||
       sem_wait(0)!=-1||*__errno_location()!=22)return 44;
    if(sem_init(semaphore_words,0,0x7ffffffeu)||
       pthread_create(&threads[0],0,overflow_worker,(void *)0)||
       pthread_create(&threads[1],0,overflow_worker,(void *)1)||
       pthread_join(threads[0],&results[0])||pthread_join(threads[1],&results[1]))return 45;
    if((uintptr_t)results[0]+(uintptr_t)results[1]!=1||
       overflow_errno[(uintptr_t)results[0]?0:1]!=75)return 46;
    if(sem_post(semaphore_words)!=-1||*__errno_location()!=75||sem_wait(semaphore_words)||
       sem_post(semaphore_words)||sem_post(semaphore_words)!=-1||*__errno_location()!=75||
       sem_destroy(semaphore_words))return 47;
    char tiny[4]={'x','x','x','x'};*__errno_location()=123;
    if(pthread_getname_np(pthread_self(),tiny,sizeof(tiny))!=34||tiny[0]!='x'||
       *__errno_location()!=123)return 48;
    if(pthread_setname_np(pthread_self(),"abcdefghijklmnop")!=34||
       pthread_getname_np(pthread_self(),name,sizeof(name))||name[0]!='m'||name[4]!=0||
       *__errno_location()!=123)return 49;
    struct { uint64_t before,mask[16],after; } guarded={0x123456789abcdef0ull,{0},0xfedcba9876543210ull};
    for(unsigned i=0;i<16;++i)guarded.mask[i]=0x5555;
    if(pthread_sigmask(123,0,guarded.mask)||guarded.before!=0x123456789abcdef0ull||
       guarded.after!=0xfedcba9876543210ull)return 50;
    for(unsigned i=0;i<16;++i)if(guarded.mask[i])return 51;
    uint64_t wide[16]={(1ull<<8)|(1ull<<18)|(1ull<<9),(1ull<<62)|(1ull<<63)};
    wide[15]=UINT64_MAX;
    if(pthread_sigmask(2,wide,0)||pthread_create(&thread,0,mask_worker,0)||
       pthread_join(thread,&results[0])||results[0])return 52;
    if(pthread_sigmask(0,0,check)||check[0]!=(1ull<<9)||check[1]!=(1ull<<62))return 53;
    for(unsigned i=2;i<16;++i)if(check[i])return 54;
    if(pthread_sigmask(1,wide,0)||pthread_sigmask(0,0,guarded.mask))return 55;
    for(unsigned i=0;i<16;++i)if(guarded.mask[i])return 56;
    struct { uint64_t before;void *key;uint64_t after; } guarded_key={0x123456789abcdef0ull,(void *)UINTPTR_MAX,0xfedcba9876543210ull};
    * __errno_location()=123;
    if(pthread_key_create(0,0)!=22)return 57;
    if(pthread_key_create(&guarded_key.key,0))return 64;
    if((uintptr_t)guarded_key.key>=32)return 66;
    if(guarded_key.before!=0x123456789abcdef0ull)return 67;
    if(guarded_key.after!=0xfedcba9876543210ull)return 68;
    if(*__errno_location()!=123)return 69;
    void *bogus=(void *)((uintptr_t)guarded_key.key|(1ull<<32));
    if(pthread_setspecific(guarded_key.key,(void *)0x1234)||pthread_setspecific(bogus,(void *)0x5678)!=22||
       pthread_key_delete(bogus)!=22||pthread_getspecific(bogus)||
       pthread_getspecific(guarded_key.key)!=(void *)0x1234||pthread_key_delete(guarded_key.key))return 58;
    for(unsigned i=0;i<32;++i){full_keys[i]=(void *)UINTPTR_MAX;
        if(pthread_key_create(&full_keys[i],0)||(uintptr_t)full_keys[i]>=32||pthread_getspecific(full_keys[i])||
           pthread_setspecific(full_keys[i],(void *)(uintptr_t)(i+1)))return 59;}
    guarded_key.key=(void *)UINTPTR_MAX;
    if(pthread_key_create(&guarded_key.key,0)!=11||guarded_key.key!=(void *)UINTPTR_MAX||
       guarded_key.before!=0x123456789abcdef0ull||guarded_key.after!=0xfedcba9876543210ull)return 60;
    if(pthread_create(&thread,0,key_worker,0)||pthread_join(thread,&results[0])||results[0]||
       pthread_getspecific(full_keys[0])!=(void *)1)return 61;
    for(unsigned i=0;i<32;++i)if(pthread_key_delete(full_keys[i]))return 62;
    if(pthread_key_create(&rearm_key,rearm_destructor)||pthread_getspecific(rearm_key)||
       pthread_create(&thread,0,rearm_worker,0)||pthread_join(thread,&results[0])||results[0]||
       rearm_count!=4||pthread_key_delete(rearm_key)||*__errno_location()!=123)return 63;
    print("ELF pthread ABI, clocks, masks, lifecycle and TLS PASS\n");
    return 35;
}
