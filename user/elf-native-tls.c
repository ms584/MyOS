/* Real compiler-generated native TLS, not manual per-thread variables. */
#include <stddef.h>
#include <stdint.h>
#include <pthread.h>
#include "../include/thread-limit.h"
extern int sem_init(void *,int,unsigned),sem_post(void *),sem_wait(void *),sem_destroy(void *);
extern int *__errno_location(void),mincore(void *,size_t,unsigned char *);
extern pthread_t pthread_self(void);
extern void *__tls_get_addr(const uint64_t [2]);
extern void MyOSTlsDesc(void);
struct timespec { int64_t tv_sec,tv_nsec; };
extern int clock_nanosleep(int,int,const struct timespec *,struct timespec *);
static int anchor;
_Thread_local uint64_t initialized=0x12345678;
_Thread_local void *pointer=&anchor;
_Thread_local uint64_t zeros[513] __attribute__((aligned(64)));
_Thread_local uint64_t ie __attribute__((tls_model("initial-exec")))=0x89abcdef;
static uint64_t gate[4],ready[4],release[4],done[4];
static void *key;
static unsigned errors[3],destructors;
static uintptr_t thread_pointers[3],stack_pages[3];
static uintptr_t tp(void){uintptr_t value;__asm__ volatile("mov %%fs:0,%0":"=r"(value));return value;}
static void print(const char *s)
{
    size_t n=0;while(s[n])++n;
    register uint64_t a __asm__("rax")=1,d __asm__("rdi")=(uintptr_t)s,q __asm__("rsi")=n;
    __asm__ volatile("int $0x80":"+a"(a):"D"(d),"S"(q):"rdx","memory","cc");
}
static int pause_tick(void){struct timespec delay={0,10000000};return clock_nanosleep(1,0,&delay,0);}
static int fresh(void)
{
    uintptr_t actual=(uintptr_t)zeros;__asm__ volatile("":"+r"(actual));
    if(!tp()||*(uintptr_t *)tp()!=tp()||initialized!=0x12345678||ie!=0x89abcdef||pointer!=&anchor||
       (actual&63))return 1;
    for(unsigned i=0;i<513;++i)if(zeros[i])return 2;
    return 0;
}
static void destructor(void *raw)
{
    unsigned who=(unsigned)(uintptr_t)raw-1;
    if(initialized!=0xabcd0000+who||ie!=who||zeros[512]!=who+1||*__errno_location()!=200+(int)who)
        errors[who]=20;
    __atomic_add_fetch(&destructors,1,__ATOMIC_SEQ_CST);sem_post(done);
}
static void *worker(void *raw)
{
    unsigned who=(unsigned)(uintptr_t)raw;int rc=fresh();errors[who]=(unsigned)rc;
    thread_pointers[who]=tp();stack_pages[who]=(uintptr_t)pthread_self();
    if(sem_wait(gate))errors[who]=3;
    initialized=0xabcd0000+who;ie=who;zeros[512]=who+1;*__errno_location()=200+(int)who;
    if(pthread_setspecific(key,(void *)(uintptr_t)(who+1)))errors[who]=4;
    sem_post(ready);sem_wait(release);
#if MODE == 1
    if(!who)__asm__ volatile("ud2");
#endif
    for(unsigned n=0;n<4;++n){
        pause_tick();if(initialized!=0xabcd0000+who||ie!=who||zeros[512]!=who+1||pointer!=&anchor||
                         *__errno_location()!=200+(int)who)errors[who]=5;
    }
    return (void *)(uintptr_t)errors[who];
}
void tls_constructor(void)
{
    if(fresh()){print("native TLS constructor FAIL\n");__asm__ volatile("ud2");}
    initialized=0x55;print("native TLS constructor PASS\n");
}
void tls_finalizer(void)
{if(initialized==0x55&&ie==0x89abcdef&&zeros[512]==0)print("native TLS finalizer PASS\n");}
static int descriptor_abi(void)
{
    uint64_t descriptor[2]={(uintptr_t)MyOSTlsDesc,(uint64_t)-123};
    uint64_t a=(uintptr_t)descriptor,d,s,c,x;
    __asm__ volatile("mov $17,%%rdi; mov $19,%%rsi; mov $23,%%rdx; pcmpeqd %%xmm0,%%xmm0;"
                     "call *(%%rax); pmovmskb %%xmm0,%%ecx"
                     :"+a"(a),"=D"(d),"=S"(s),"=d"(x),"=c"(c)::"xmm0","memory","cc");
    return a==(uint64_t)-123&&d==17&&s==19&&x==23&&c==65535;
}
uint64_t _start(void)
{
    if(initialized!=0x55||!descriptor_abi())return 1;
    uint64_t index[2]={2,0};if(__tls_get_addr(index)||*__errno_location()!=22)return 2;
    index[0]=1;index[1]=UINT64_MAX;if(__tls_get_addr(index)||*__errno_location()!=22)return 3;
    if(sem_init(gate,0,0)||sem_init(ready,0,0)||sem_init(release,0,0)||sem_init(done,0,0)||
       pthread_key_create(&key,destructor))return 4;
#if MODE == 2
    print("native TLS NX ready\n");uintptr_t target=(uintptr_t)&zeros[0];
    __asm__ volatile("call *%0"::"r"(target):"memory");
#endif
#if MODE == 3
    print("native TLS guard ready\n");*(volatile char *)(uintptr_t)(0x803f010000ull+(MYOS_WORKER_LIMIT+1ull)*4096)=1;
#endif
    for(unsigned wave=0;wave<12;++wave){pthread_t workers[3];void *result;
        for(unsigned who=0;who<3;++who){errors[who]=99;
            if(pthread_create(&workers[who],0,worker,(void *)(uintptr_t)who))return 5;
            if((wave&1)&&pthread_detach(workers[who]))return 6;
        }
        for(unsigned who=0;who<3;++who)sem_post(gate);
        for(unsigned who=0;who<3;++who)sem_wait(ready);
        for(unsigned who=0;who<3;++who){if(errors[who]||thread_pointers[who]==tp())return 7;
            for(unsigned other=who+1;other<3;++other)if(thread_pointers[who]==thread_pointers[other])return 8;}
#if MODE == 4
        print("native TLS blocked cancel ready\n");sem_wait(done);
#endif
        for(unsigned who=0;who<3;++who)sem_post(release);
        for(unsigned who=0;who<3;++who)sem_wait(done);
        if(!(wave&1)){for(unsigned who=0;who<3;++who)
            if(pthread_join(workers[who],&result)||result||errors[who])return 9;
        }else{
            for(unsigned attempt=0;attempt<200;++attempt){unsigned count=0;unsigned char resident;
                for(unsigned who=0;who<3;++who)if(mincore((void *)stack_pages[who],4096,&resident)==-1&&
                                                    *__errno_location()==12)++count;
                if(count==3)break;if(attempt==199)return 10;pause_tick();}
        }
        if(initialized!=0x55||ie!=0x89abcdef||zeros[512]||pointer!=&anchor)return 11;
        for(unsigned who=0;who<3;++who){const uint64_t *tcb=(const void *)thread_pointers[who];
            if(errors[who]||tcb[0]||tcb[2]||tcb[3])return 12;}
    }
    if(destructors!=36||pthread_key_delete(key)||sem_destroy(gate)||sem_destroy(ready)||
       sem_destroy(release)||sem_destroy(done))return 13;
    print("native ELF TLS template/zero/relocation, FS isolation, slot reuse and destructors PASS\n");
    return 58;
}
