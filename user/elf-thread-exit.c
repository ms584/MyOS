#include <stdint.h>
extern uintptr_t pthread_self(void);
extern int pthread_create(uintptr_t *,const void *,void *(*)(void *),void *);
extern int pthread_join(uintptr_t,void **),pthread_key_create(void **,void (*)(void *));
extern int pthread_detach(uintptr_t);
#ifndef DETACH_MAIN
#define DETACH_MAIN 0
#endif
extern int pthread_setspecific(void *,const void *);
extern _Noreturn void pthread_exit(void *);
struct timespec {int64_t tv_sec,tv_nsec;};
extern int clock_nanosleep(int,int,const struct timespec *,struct timespec *);
static uintptr_t main_id;
static void *key;
static unsigned destructors,finished;
static void check(int okay){if(!okay)__builtin_trap();}
static void destructor(void *raw)
{
    uintptr_t value=(uintptr_t)raw;
    __atomic_add_fetch(&destructors,1,__ATOMIC_SEQ_CST);
    if(value<100)check(!pthread_setspecific(key,(void *)(value+100)));
}
static void *worker(void *raw)
{
    uintptr_t who=(uintptr_t)raw;check(!pthread_setspecific(key,(void *)(who+2)));
    if(!who){void *result=0;
        if(DETACH_MAIN)check(pthread_join(main_id,&result)==22);
        else check(!pthread_join(main_id,&result)&&result==(void *)0x777);}
    struct timespec delay={0,30000000};check(!clock_nanosleep(1,0,&delay,0));
    __atomic_add_fetch(&finished,1,__ATOMIC_SEQ_CST);
    pthread_exit((void *)(who+10));
}
void finalizer(void)
{
    check(destructors==6&&finished==2);
    const char message[]="native main/worker pthread_exit join/destructors/finalizer PASS\n";
    register uint64_t a __asm__("rax")=1,d __asm__("rdi")=(uintptr_t)message;
    register uint64_t s __asm__("rsi")=sizeof(message)-1;
    __asm__ volatile("int $0x80":"+a"(a):"D"(d),"S"(s):"rdx","memory","cc");
}
int _start(void)
{
    main_id=pthread_self();check(!pthread_key_create(&key,destructor));
    if(DETACH_MAIN){check(!pthread_detach(main_id));check(pthread_detach(main_id)==22);}
    check(!pthread_setspecific(key,(void *)1));
    uintptr_t threads[2];
    for(unsigned i=0;i<2;++i)check(!pthread_create(&threads[i],0,worker,(void *)(uintptr_t)i));
    pthread_exit((void *)0x777);
}
