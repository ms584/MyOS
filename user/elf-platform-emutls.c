/* Native pthread/VM contract integration with unmodified compiler-rt emutls.
 * This does not execute libcobalt.so or claim C++ codegen compatibility. */
#include <stddef.h>
#include <stdint.h>
#include <pthread.h>
#include "../third_party/compiler-rt/emutls.c"

extern int *__errno_location(void);
extern int sem_init(void *,int,unsigned),sem_post(void *),sem_wait(void *),sem_destroy(void *);
extern int mincore(void *,size_t,unsigned char *);
struct timespec { int64_t tv_sec,tv_nsec; };
extern int clock_nanosleep(int,int,const struct timespec *,struct timespec *);

/* Fixture-local byte routines: these are not additional production exports. */
void *memcpy(void *dst,const void *src,size_t size)
{unsigned char *d=dst;const unsigned char *s=src;for(size_t i=0;i<size;++i)d[i]=s[i];return dst;}
void *memset(void *dst,int value,size_t size)
{unsigned char *d=dst;for(size_t i=0;i<size;++i)d[i]=(unsigned char)value;return dst;}
_Noreturn void abort(void){__asm__ volatile("ud2");for(;;){}}

extern void weak_tls_init(void)
    __asm__("_ZTHN2v88internal12trap_handler21g_thread_in_wasm_codeE") __attribute__((weak));
#define OBJECTS 48u
#define WORKERS 3u
#define PAGE 4096u
static __emutls_control controls[OBJECTS];
static uint64_t templates[OBJECTS];
static uint64_t gate[4],ready[4],release[4];
static void *addresses[WORKERS][OBJECTS],*arrays[WORKERS];
static unsigned errors[WORKERS];
static unsigned serial;

static void print(const char *s)
{
    size_t n=0;while(s[n])++n;
    register uint64_t a __asm__("rax")=1,d __asm__("rdi")=(uintptr_t)s,q __asm__("rsi")=n;
    __asm__ volatile("int $0x80":"+a"(a):"D"(d),"S"(q):"rdx","memory","cc");
}
static int pause_tick(void)
{struct timespec delay={0,10000000};return clock_nanosleep(1,0,&delay,0);}
static uint64_t expected(unsigned object,unsigned worker)
{return 0xabc00000ull+((uint64_t)serial<<16)+((uint64_t)worker<<8)+object;}
static int mapped(void *address)
{
    unsigned char resident=0;uintptr_t page=(uintptr_t)address&~(uintptr_t)(PAGE-1);
    int result=mincore((void *)page,PAGE,&resident);
    if(!result)return resident==1?1:-1;
    return *__errno_location()==12?0:-1;
}
static int released(unsigned worker)
{
    if(mapped(arrays[worker])!=0)return 0;
    for(unsigned i=0;i<OBJECTS;++i){
        /* Upstream's saved malloc pointer precedes the aligned TLS value.
         * Copy it while the thread is alive; do not read freed storage. */
        uintptr_t first=(uintptr_t)addresses[worker][i];
        if(mapped((void *)first)!=0)return 0;
    }
    return 1;
}
static void *worker(void *raw)
{
    unsigned who=(unsigned)(uintptr_t)raw;unsigned error=0;
    if(sem_wait(gate))error=1;
    *__errno_location()=200+(int)who;
    if(weak_tls_init)weak_tls_init(); /* The real weak symbol must zero-bind. */
    for(unsigned n=0;n<OBJECTS;++n){
        unsigned i=who&1?OBJECTS-1-n:n;
        uint64_t *p=__emutls_get_address(&controls[i]);
        if(!p||((uintptr_t)p&(controls[i].align-1))||
           *p!=(i&1?templates[i]:0))error=2;
        for(size_t byte=8;byte<controls[i].size;++byte)
            if(((unsigned char *)p)[byte])error=3;
        *p=expected(i,who);
        if(__emutls_get_address(&controls[i])!=p)error=4;
        /* Save a currently live malloc page, not the possibly next-page
         * aligned value, so mincore proves allocation unmapping at exit. */
        addresses[who][i]=((void **)p)[-1];
    }
    emutls_address_array *array=emutls_getspecific();
    if(!array||array->size<OBJECTS||array->skip_destructor_rounds!=EMUTLS_SKIP_DESTRUCTOR_ROUNDS)error=5;
    arrays[who]=array;errors[who]=error;
    if(sem_post(ready)||sem_wait(release))return (void *)6;
    for(unsigned round=0;round<6;++round){
        if(pause_tick())error=7;
        for(unsigned i=0;i<OBJECTS;++i)
            if(*(uint64_t *)__emutls_get_address(&controls[i])!=expected(i,who))error=8;
        if(*__errno_location()!=200+(int)who)error=9;
    }
    errors[who]=error;return (void *)(uintptr_t)error;
}

uint64_t _start(void)
{
    pthread_t threads[WORKERS];void *result;
    /* Nonzero templates are 8 bytes; larger zero controls test memset and
     * a page-sized alignment. A zero-initialized V8-like int is also tested. */
    for(unsigned i=0;i<OBJECTS;++i){templates[i]=0x12340000ull+i;
        controls[i].size=i&1?8:((i%3)==0?513:8);
        controls[i].align=i&1?8:((i%3)==0?4096:64);
        controls[i].value=i&1?&templates[i]:0;}
    if(sem_init(gate,0,0)||sem_init(ready,0,0)||sem_init(release,0,0))return 1;
    /* Simultaneous first access assigns every index once under once/mutex.
     * Alternate join/detach, exceeding simultaneous thread-slot capacity. */
    for(serial=1;serial<=12;++serial){
        for(unsigned who=0;who<WORKERS;++who){errors[who]=99;
            if(pthread_create(&threads[who],0,worker,(void *)(uintptr_t)who))return 2;
            if(serial%2==0&&pthread_detach(threads[who]))return 3;}
        for(unsigned who=0;who<WORKERS;++who)if(sem_post(gate))return 4;
        for(unsigned who=0;who<WORKERS;++who)if(sem_wait(ready))return 70;
        for(unsigned who=0;who<WORKERS;++who)
            if(errors[who])return 71+who*10+errors[who];
        if(emutls_num_object!=OBJECTS)return 6;
        uint64_t seen=0;
        for(unsigned i=0;i<OBJECTS;++i){uintptr_t index=controls[i].object.index;
            if(!index||index>OBJECTS||(seen&(1ull<<(index-1))))return 7;
            seen|=1ull<<(index-1);
            for(unsigned who=0;who<WORKERS;++who)
                for(unsigned other=who+1;other<WORKERS;++other)
                    if(addresses[who][i]==addresses[other][i])return 8;}
        if(pthread_getspecific(emutls_pthread_key))return 9; /* Main has no TLS. */
        for(unsigned who=0;who<WORKERS;++who)if(sem_post(release))return 10;
        if(serial&1){
            for(unsigned who=0;who<WORKERS;++who){
                if(pthread_join(threads[who],&result)||result||errors[who]||!released(who))return 11;}
        }else{
            for(unsigned attempt=0;attempt<300;++attempt){unsigned done=0;
                for(unsigned who=0;who<WORKERS;++who)
                    if(released(who))++done;
                if(done==WORKERS)break;
                if(pause_tick())return 19;
                if(attempt==299)return 12;
            }
            for(unsigned who=0;who<WORKERS;++who)if(errors[who])return 13;
        }
        *__errno_location()=123;
    }
    if(weak_tls_init)return 14;
    __emutls_control v8_like={4,4,{0},0};
    int *state=__emutls_get_address(&v8_like);
    if(!state||*state||((uintptr_t)state&3))return 15;
    *state=1;
    if(__emutls_get_address(&v8_like)!=state||*state!=1||*__errno_location()!=123)return 16;
    /* Explicit main-thread fixture cleanup; this is not libc exit handling. */
    emutls_address_array *main_array=emutls_getspecific();
    void *main_object=((void **)state)[-1];
    if(pthread_setspecific(emutls_pthread_key,0))return 17;
    emutls_shutdown(main_array);free(main_array);
    if(mapped(main_array)!=0||mapped(main_object)!=0||pthread_key_delete(emutls_pthread_key)||
       sem_destroy(gate)||sem_destroy(ready)||sem_destroy(release))return 18;
    print("ELF upstream emutls isolation, alignment, growth and destructor cleanup PASS\n");
    return 43;
}
