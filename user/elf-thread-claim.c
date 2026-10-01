#include <stdint.h>
struct timespec { int64_t tv_sec,tv_nsec; };
extern int pthread_create(uintptr_t *,const void *,void *(*)(void *),void *);
extern int pthread_join(uintptr_t,void **),pthread_detach(uintptr_t);
extern int clock_nanosleep(int,int,const struct timespec *,struct timespec *);
static uintptr_t target;
static unsigned start,finished,release_target;
static int results[2];
static void check(int okay){if(!okay)__builtin_trap();}
static void delay(unsigned millis)
{struct timespec t={0,(int64_t)millis*1000000};check(!clock_nanosleep(1,0,&t,0));}
static void *target_entry(void *unused)
{(void)unused;while(!__atomic_load_n(&release_target,__ATOMIC_ACQUIRE))delay(10);
 __atomic_store_n(&finished,1,__ATOMIC_RELEASE);return (void *)0x55;}
static void *claim(void *raw)
{
    unsigned who=(unsigned)(uintptr_t)raw;
    while(!__atomic_load_n(&start,__ATOMIC_ACQUIRE))delay(10);
    if(who)results[1]=pthread_detach(target);
    else{void *value=0;results[0]=pthread_join(target,&value);if(!results[0])check(value==(void *)0x55);}
    return 0;
}
int _start(void)
{
    for(unsigned wave=0;wave<12;++wave){
        start=finished=release_target=0;results[0]=results[1]=-1;uintptr_t racers[2];
        check(!pthread_create(&target,0,target_entry,0));
        for(unsigned i=0;i<2;++i)check(!pthread_create(&racers[i],0,claim,(void *)(uintptr_t)i));
        __atomic_store_n(&start,1,__ATOMIC_RELEASE);
        /* Keep the target alive through both claims. A late detach after a
         * successful join would use an expired handle, not test contention. */
        check(!pthread_join(racers[1],0));
        if(results[1]==0)check(!pthread_join(racers[0],0));
        __atomic_store_n(&release_target,1,__ATOMIC_RELEASE);
        if(results[1]!=0)check(!pthread_join(racers[0],0));
        if(!((results[0]==0&&results[1]==22)||(results[0]==22&&results[1]==0))){
            char message[]="claim results: join=00 detach=00\n";
            message[20]='0'+results[0]/10;message[21]='0'+results[0]%10;
            message[30]='0'+results[1]/10;message[31]='0'+results[1]%10;
            register uint64_t a __asm__("rax")=1,d __asm__("rdi")=(uintptr_t)message;
            register uint64_t s __asm__("rsi")=sizeof(message)-1;
            __asm__ volatile("int $0x80":"+a"(a):"D"(d),"S"(s):"rdx","memory","cc");
            __builtin_trap();}
        while(!__atomic_load_n(&finished,__ATOMIC_ACQUIRE))delay(10);
        delay(10);
    }
    const char message[]="native pthread join/detach exclusive ownership PASS\n";
    register uint64_t a __asm__("rax")=1,d __asm__("rdi")=(uintptr_t)message;
    register uint64_t s __asm__("rsi")=sizeof(message)-1;
    __asm__ volatile("int $0x80":"+a"(a):"D"(d),"S"(s):"rdx","memory","cc");
    return 0;
}
