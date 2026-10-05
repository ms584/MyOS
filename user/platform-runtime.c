#include <stddef.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include "../include/system-call.h"

#define SYS_VM_MAP 14u
#define SYS_VM_UNMAP 15u
#define SYS_VM_PROTECT 16u
#define SYS_VM_QUERY 22u
#define SYS_VM_ADVISE 23u
#define SYS_ERRNO_LOCATION 24u
#define SYS_VM_MAP_FIXED 25u
#define PROT_RW 3u
#define PAGE_SIZE 4096u
#define HEADER_MAGIC 0x4d594f53414c4c4full

struct allocation { uint64_t magic,base,length,requested; };
#define SLAB_MAGIC 0x4d594f53534c4142ull
#define SLAB_HEADER 64u
struct allocation_slab { struct allocation_slab *next;uint64_t magic,used,slot; };
struct allocator_state {
    uintptr_t cursor;
    struct allocation_slab *heads[5];
    uint64_t reserved[2],lock[10]; /* pinned opaque80-byte native mutex */
};
/* Per-process runtime data; these bounds are the SYS_VM_MAP ABI in process.h.
 * Reserve a unique hint, not memory. Kernel ownership/collision checks remain
 * authoritative and a rejected hint falls back to the original gap search.
 */
#define ALLOCATION_VM_BASE 0x8020000000ull
#define ALLOCATION_VM_END  0x803f000000ull
/* Main private RW/NX TLS page bytes1744..1887, after event state1712..1743.
 * Zero initialization chooses VM_BASE; never put mutable state in RX code.
 */
static struct allocator_state *allocator_state(void)
{return (struct allocator_state *)(uintptr_t)0x803f0106d0ull;}
static uintptr_t *allocation_cursor(void)
{return &allocator_state()->cursor;}
_Static_assert(sizeof(struct allocator_state)==144,"private allocator state bound");
int pthread_mutex_lock(void *);
int pthread_mutex_unlock(void *);
int *__errno_location(void);

static int64_t call3(uint64_t number,uint64_t first,uint64_t second,uint64_t third)
{
    register uint64_t a __asm__("rax")=number;
    register uint64_t d __asm__("rdi")=first;
    register uint64_t s __asm__("rsi")=second;
    register uint64_t x __asm__("rdx")=third;
    __asm__ volatile("int $0x80":"+a"(a):"D"(d),"S"(s),"d"(x):"memory","cc");
    return (int64_t)a;
}
static int power_of_two(size_t value){return value&&!(value&(value-1));}
static uintptr_t allocation_hint(size_t length)
{
    if(length>ALLOCATION_VM_END-ALLOCATION_VM_BASE)return 0;
    uintptr_t *cursor=allocation_cursor();
    uintptr_t previous=__atomic_load_n(cursor,__ATOMIC_RELAXED);
    for(;;){
        uintptr_t address=previous;
        if(address<ALLOCATION_VM_BASE||address>ALLOCATION_VM_END-length)
            address=ALLOCATION_VM_BASE;
        if(__atomic_compare_exchange_n(cursor,&previous,address+length,
                                     1,__ATOMIC_RELAXED,__ATOMIC_RELAXED))return address;
    }
}
static int64_t allocation_map(size_t length)
{
    uintptr_t hint=allocation_hint(length);
    int64_t mapped=call3(SYS_VM_MAP,hint,length,PROT_RW);
    return mapped<0&&hint?call3(SYS_VM_MAP,0,length,PROT_RW):mapped;
}
/* Small objects share pages, without changing the public allocation header.
 * One native mutex protects membership/bitmaps; no allocator call is made by
 * that mutex or while creating a slab. Empty slabs are unmapped immediately.
 */
static void *allocate_small(size_t size)
{
    unsigned index=0;size_t slot=64;
    while(slot<size+sizeof(struct allocation)){slot*=2;++index;}
    struct allocator_state *state=allocator_state();
    if(pthread_mutex_lock(state->lock))return 0;
    struct allocation_slab *slab=state->heads[index];
    unsigned slots=(PAGE_SIZE-SLAB_HEADER)/(unsigned)slot;
    uint64_t available=(UINT64_C(1)<<slots)-1;
    while(slab&&slab->used==available)slab=slab->next;
    if(!slab){
        int64_t mapped=allocation_map(PAGE_SIZE);
        if(mapped<0){(void)pthread_mutex_unlock(state->lock);return 0;}
        slab=(void *)(uintptr_t)mapped;
        *slab=(struct allocation_slab){state->heads[index],SLAB_MAGIC,0,slot};
        state->heads[index]=slab;
    }
    unsigned bit=(unsigned)__builtin_ctzll(available&~slab->used);
    slab->used|=UINT64_C(1)<<bit;
    struct allocation *header=(void *)((uintptr_t)slab+SLAB_HEADER+bit*slot);
    *header=(struct allocation){SLAB_MAGIC,(uintptr_t)slab,slot,size};
    (void)pthread_mutex_unlock(state->lock);
    return header+1;
}
static void *allocate(size_t alignment,size_t size,int exact)
{
    if(!size||!power_of_two(alignment)||alignment<sizeof(void *))return 0;
    if(exact&&size%alignment)return 0;
    if(size>SIZE_MAX-sizeof(struct allocation)-alignment)return 0;
    if(alignment<=16&&size<=1024-sizeof(struct allocation))return allocate_small(size);
    size_t need=size+sizeof(struct allocation)+alignment-1;
    if(need>SIZE_MAX-(PAGE_SIZE-1))return 0;
    size_t length=(need+PAGE_SIZE-1)&~(size_t)(PAGE_SIZE-1);
    int64_t mapped=allocation_map(length);
    if(mapped<0)return 0;
    uintptr_t address=((uintptr_t)mapped+sizeof(struct allocation)+alignment-1)&~(uintptr_t)(alignment-1);
    struct allocation *header=(struct allocation *)address-1;
    *header=(struct allocation){HEADER_MAGIC,(uint64_t)mapped,length,size};
    return (void *)address;
}
void *malloc(size_t size){void *p=allocate(16,size,0);if(size&&!p)*__errno_location()=12;return p;}
void free(void *address)
{
    if(!address)return;
    struct allocation *header=(struct allocation *)address-1;
    if(header->magic==SLAB_MAGIC){
        struct allocator_state *state=allocator_state();
        if(pthread_mutex_lock(state->lock))return;
        struct allocation_slab *slab=(void *)(uintptr_t)header->base;
        size_t slot=header->length;unsigned index=0;
        for(size_t n=64;n<slot;n*=2)++index;
        unsigned bit=(unsigned)(((uintptr_t)header-(uintptr_t)slab-SLAB_HEADER)/slot);
        header->magic=0;slab->used&=~(UINT64_C(1)<<bit);
        if(!slab->used){
            struct allocation_slab **link=&state->heads[index];
            while(*link&&*link!=slab)link=&(*link)->next;
            if(*link){*link=slab->next;(void)call3(SYS_VM_UNMAP,(uintptr_t)slab,PAGE_SIZE,0);}
        }
        (void)pthread_mutex_unlock(state->lock);return;
    }
    if(header->magic!=HEADER_MAGIC)return;
    uint64_t base=header->base,length=header->length;header->magic=0;
    (void)call3(SYS_VM_UNMAP,base,length,0);
}
void *calloc(size_t count,size_t size)
{
    if(count&&size>SIZE_MAX/count){*__errno_location()=12;return 0;}
    size_t bytes=count*size;unsigned char *memory=malloc(bytes);
    if(memory)for(size_t i=0;i<bytes;++i)memory[i]=0;
    return memory;
}
size_t malloc_usable_size(void *address)
{
    if(!address)return 0;
    struct allocation *header=(struct allocation *)address-1;
    return header->magic==HEADER_MAGIC||header->magic==SLAB_MAGIC?(size_t)header->requested:0;
}
void *realloc(void *address,size_t size)
{
    if(!address)return malloc(size);
    if(!size){free(address);return 0;}
    size_t old=malloc_usable_size(address);if(!old)return 0;
    void *replacement=malloc(size);if(!replacement)return 0;
    size_t count=old<size?old:size;unsigned char *dst=replacement,*src=address;
    for(size_t i=0;i<count;++i)dst[i]=src[i];
    free(address);return replacement;
}
void *aligned_alloc(size_t alignment,size_t size)
{
    if(!power_of_two(alignment)||alignment<sizeof(void *)||size%alignment){
        *__errno_location()=22;return 0;
    }
    void *p=allocate(alignment,size,1);if(size&&!p)*__errno_location()=12;return p;
}
int posix_memalign(void **result,size_t alignment,size_t size)
{
    if(!result||!power_of_two(alignment)||alignment%sizeof(void *))return 22;
    void *address=allocate(alignment,size,0);if(!address)return 12;
    *result=address;return 0;
}


#define POSIX_PROT_READ 1
#define POSIX_PROT_WRITE 2
#define POSIX_PROT_EXEC 4
#define MAP_SHARED 1
#define MAP_PRIVATE 2
#define MAP_FIXED 0x10
#define MAP_ANONYMOUS 0x20
#define MAP_NORESERVE 0x4000
#define MAP_STACK 0x20000
#define MAP_FIXED_NOREPLACE 0x100000

int *__errno_location(void)
{
    return (int *)(uintptr_t)call3(SYS_ERRNO_LOCATION,0,0,0);
}
static int fail(int64_t error)
{
    *__errno_location()=(int)-error;
    return -1;
}
static int vm_prot(int prot)
{
    if(prot&~7)return -1;
    if((prot&POSIX_PROT_WRITE)&&(prot&POSIX_PROT_EXEC))return -1;
    if(prot&POSIX_PROT_WRITE)prot|=POSIX_PROT_READ;
    if(prot&POSIX_PROT_EXEC)prot|=POSIX_PROT_READ;
    return prot;
}
static int rounded(size_t length,size_t *result)
{
    if(!length||length>SIZE_MAX-(PAGE_SIZE-1))return 0;
    *result=(length+PAGE_SIZE-1)&~(size_t)(PAGE_SIZE-1);
    return 1;
}
void *mmap(void *address,size_t length,int prot,int flags,int fd,int64_t offset)
{
    size_t bytes;int translated=vm_prot(prot);
    int allowed=MAP_SHARED|MAP_PRIVATE|MAP_FIXED|MAP_ANONYMOUS|MAP_NORESERVE|MAP_STACK|MAP_FIXED_NOREPLACE;
    if(translated<0||!rounded(length,&bytes)||(flags&~allowed)||
       ((!!(flags&MAP_PRIVATE))==(!!(flags&MAP_SHARED)))||!(flags&MAP_ANONYMOUS)||
       fd!=-1||offset||((flags&(MAP_FIXED|MAP_FIXED_NOREPLACE))&&
       ((uintptr_t)address&(PAGE_SIZE-1)))){
        *__errno_location()=22;return (void *)(uintptr_t)-1;
    }
    uint64_t hint=(flags&(MAP_FIXED|MAP_FIXED_NOREPLACE))?(uint64_t)(uintptr_t)address:0;
    uint64_t syscall=(flags&MAP_FIXED)?SYS_VM_MAP_FIXED:SYS_VM_MAP;
    int64_t result=call3(syscall,hint,bytes,(unsigned)translated);
    if(result<0){fail(result);return (void *)(uintptr_t)-1;}
    return (void *)(uintptr_t)result;
}
int mprotect(void *address,size_t length,int prot)
{
    size_t bytes;int translated=vm_prot(prot);
    if(translated<0||((uintptr_t)address&(PAGE_SIZE-1))||!rounded(length,&bytes)){
        *__errno_location()=22;return -1;
    }
    int64_t result=call3(SYS_VM_PROTECT,(uintptr_t)address,bytes,(unsigned)translated);
    return result<0?fail(result):0;
}
int munmap(void *address,size_t length)
{
    size_t bytes;
    if(((uintptr_t)address&(PAGE_SIZE-1))||!rounded(length,&bytes)){
        *__errno_location()=22;return -1;
    }
    int64_t result=call3(SYS_VM_UNMAP,(uintptr_t)address,bytes,0);
    return result<0?fail(result):0;
}
int mincore(void *address,size_t length,unsigned char *vector)
{
    size_t bytes;
    if(((uintptr_t)address&(PAGE_SIZE-1))||!rounded(length,&bytes)){
        *__errno_location()=22;return -1;
    }
    int64_t result=call3(SYS_VM_QUERY,(uintptr_t)address,bytes,(uintptr_t)vector);
    return result<0?fail(result):0;
}
int madvise(void *address,size_t length,int advice)
{
    size_t bytes;
    if(((uintptr_t)address&(PAGE_SIZE-1))||!rounded(length,&bytes)){
        *__errno_location()=22;return -1;
    }
    int64_t result=call3(SYS_VM_ADVISE,(uintptr_t)address,bytes,(unsigned)advice);
    return result<0?fail(result):0;
}
int msync(void *address,size_t length,int flags)
{
    size_t bytes;
    if(((uintptr_t)address&(PAGE_SIZE-1))||!rounded(length,&bytes)||
       (flags&~7)||!((flags&1)^(flags&4))){
        *__errno_location()=22;return -1;
    }
    unsigned char byte;int64_t result=0;
    for(size_t offset=0;offset<bytes;offset+=PAGE_SIZE){
        result=call3(SYS_VM_QUERY,(uintptr_t)address+offset,PAGE_SIZE,(uintptr_t)&byte);
        if(result<0)break;
    }
    return result<0?fail(result):0;
}


#define SYS_EXIT 3u
#define SYS_YIELD 4u
#define SYS_TICKS 13u
#define SYS_THREAD_CREATE 17u
#define SYS_THREAD_EXIT 18u
#define SYS_THREAD_REAP 19u
#define SYS_WAIT 20u
#define SYS_WAKE 21u
#define SYS_THREAD_DETACH 26u
#define SYS_CLOCK_NANOS 27u
#define SYS_GETTID 28u
#define SYS_MAIN_STACK_INFO 48u
#define SYS_SIGNAL 49u
#define RUNTIME_TLS_BASE 0x803f010000ull
#define THREAD_STACK_SIZE (1024u*1024u)
struct main_stack_info { uint64_t base,length,guard_base,guard_length; };
#define THREAD_MAGIC 0x4d594f5350544852ull
#define EAGAIN 11
#define EPERM 1
#define EINVAL 22
#define ESRCH 3
#define EBUSY 16
#define EDEADLK 35
#define ETIMEDOUT 110
#define ENOSYS 38
#define EOVERFLOW 75
#define MUSL_SIGSET_WORDS 16u
#define THREAD_KEYS 32u

struct timespec { int64_t tv_sec,tv_nsec; };
struct thread_control {
    uint64_t magic,tid,base,length;
    void *(*start)(void *);
    void *argument,*result;
    uint64_t detached,joined,done;
    uint64_t signal_mask[2];
    char name[16];
};
struct thread_attr { uint64_t magic,stack_size,detached,stack_base; };
struct condition_layout { volatile uint64_t sequence;uint32_t clock,waiters; };

static int64_t call4(uint64_t number,uint64_t first,uint64_t second,uint64_t third,uint64_t fourth)
{
    register uint64_t a __asm__("rax")=number;
    register uint64_t d __asm__("rdi")=first;
    register uint64_t s __asm__("rsi")=second;
    register uint64_t x __asm__("rdx")=third;
    register uint64_t t __asm__("r10")=fourth;
    __asm__ volatile("int $0x80":"+a"(a):"D"(d),"S"(s),"d"(x),"r"(t):"memory","cc");
    return (int64_t)a;
}
static uint64_t self_page(void){return (uint64_t)(uintptr_t)__errno_location()&~4095ull;}
static struct thread_control *self_control(void)
{return *(struct thread_control **)(uintptr_t)(self_page()+8);}
/* One eager ELF TLS module. This TCB is not a glibc/musl pthread object.
 * Compiler-generated TLS accesses use FS; private runtime identity stays in
 * its existing pages. Slot metadata is zero when no ELF TLS is installed. */
void *__tls_get_addr(const uint64_t index[2])
{
    uint64_t tp=*(const uint64_t *)(uintptr_t)(self_page()+48);
    if(!index||!tp||index[0]!=1){*__errno_location()=EINVAL;return 0;}
    const uint64_t *tcb=(const void *)(uintptr_t)tp;
    if(index[1]>=tcb[3]){*__errno_location()=EINVAL;return 0;}
    return (void *)(uintptr_t)(tcb[2]+index[1]);
}
static void yield_cpu(void){(void)call3(SYS_YIELD,0,0,0);}
static int wait64(volatile uint64_t *word,uint64_t value,uint64_t deadline)
{return (int)call3(SYS_WAIT,(uintptr_t)word,value,deadline);}
static void wake64(volatile uint64_t *word,unsigned count)
{(void)call3(SYS_WAKE,(uintptr_t)word,count,0);}
static __attribute__((noinline)) uint64_t ticks(void)
{return (uint64_t)call3(SYS_TICKS,0,0,0);}
static int clock_nanos(int clock,uint64_t *value)
{
    int64_t result=call3(SYS_CLOCK_NANOS,(unsigned)clock,0,0);
    if(result<0)return (int)-result;*value=(uint64_t)result;return 0;
}
static __attribute__((noinline)) int timespec_nanos(const struct timespec *time,uint64_t *value)
{
    if(!time||time->tv_sec<0||
       time->tv_nsec<0||time->tv_nsec>=1000000000)return EINVAL;
    if((uint64_t)time->tv_sec>(UINT64_MAX-(uint64_t)time->tv_nsec)/1000000000ull)return EINVAL;
    *value=(uint64_t)time->tv_sec*1000000000ull+(uint64_t)time->tv_nsec;return 0;
}
static __attribute__((noinline)) uint64_t duration_deadline(uint64_t duration,uint64_t now)
{
    uint64_t add=duration/10000000ull+(duration%10000000ull!=0);
    return add>UINT64_MAX-1-now?UINT64_MAX-1:now+add;
}
static int deadline_ticks(int clock,const struct timespec *time,uint64_t *deadline)
{
    uint64_t target;int converted=timespec_nanos(time,&target);if(converted)return converted;
    if(!deadline||(clock!=0&&clock!=1))return EINVAL;
    uint64_t now_ns=0;int rc=clock_nanos(clock,&now_ns);if(rc)return rc;
    uint64_t now_ticks=ticks();
    if(target<=now_ns){*deadline=now_ticks;return 0;}
    *deadline=duration_deadline(target-now_ns,now_ticks);return 0;
}
int clock_gettime(int clock,struct timespec *time)
{
    if(!time){*__errno_location()=14;return -1;}
    uint64_t value=0;int rc=clock_nanos(clock,&value);
    if(rc){*__errno_location()=rc;return -1;}
    time->tv_sec=(int64_t)(value/1000000000ull);
    time->tv_nsec=(int64_t)(value%1000000000ull);return 0;
}
int clock_nanosleep(int clock,int flags,const struct timespec *request,struct timespec *remaining)
{
    if((clock!=0&&clock!=1)||(flags!=0&&flags!=1))return EINVAL;
    uint64_t deadline,end_ns=0;
    if(flags){int rc=deadline_ticks(clock,request,&deadline);if(rc)return rc;}
    else {
        uint64_t duration;int rc=timespec_nanos(request,&duration);if(rc)return rc;
        uint64_t now=ticks();
        deadline=duration_deadline(duration,now);
        uint64_t start;rc=clock_nanos(1,&start);if(rc)return rc;
        end_ns=duration>UINT64_MAX-start?UINT64_MAX:start+duration;
    }
    if(deadline<=ticks())return 0;
    volatile uint64_t *scratch=(uint64_t *)(uintptr_t)(self_page()+32);
    uint64_t value=__atomic_load_n(scratch,__ATOMIC_ACQUIRE);
    for(;;){int rc=wait64(scratch,value,deadline);
        if(rc==-ETIMEDOUT)break;if(rc!=-EAGAIN){
            if(rc==-4&&!flags&&remaining){uint64_t now;int error=clock_nanos(1,&now);
                if(error)return error;uint64_t left=now<end_ns?end_ns-now:0;
                *remaining=(struct timespec){(int64_t)(left/1000000000ull),(int64_t)(left%1000000000ull)};}
            return -rc;}
        if(deadline<=ticks())break;
    }
    if(!flags&&remaining)*remaining=(struct timespec){0,0};return 0;
}

static uint32_t *key_bits(void){return (uint32_t *)(uintptr_t)(RUNTIME_TLS_BASE+1024);}
static void (**key_destructors(void))(void *)
{return (void (**)(void *))(uintptr_t)(RUNTIME_TLS_BASE+1152);}
static void **key_values(void){return (void **)(uintptr_t)(self_page()+64);}
static char *main_thread_name(void){return (char *)(uintptr_t)(RUNTIME_TLS_BASE+512);}
static void clear_key_values(unsigned key)
{for(unsigned i=0;i<=8;++i)*(void **)(uintptr_t)(RUNTIME_TLS_BASE+i*4096+64+key*8)=0;}
static struct thread_control *thread_control(uint64_t thread)
{
    if(!thread||(thread&4095)||thread<0x8020000000ull||thread>=0x803f000000ull)return 0;
    unsigned char resident;
    if(call3(SYS_VM_QUERY,thread,PAGE_SIZE,(uintptr_t)&resident)<0)return 0;
    struct thread_control *control=(void *)(uintptr_t)thread;
    return control->magic==THREAD_MAGIC?control:0;
}
static void run_destructors(void)
{
    void **values=key_values();void (**destructors)(void *)=key_destructors();
    for(unsigned pass=0;pass<4;++pass){int any=0;
        for(unsigned key=0;key<THREAD_KEYS;++key){void *value=values[key];
            void (*destructor)(void *)=destructors[key];
            if(value&&destructor){values[key]=0;destructor(value);any=1;}
        }
        if(!any)break;
    }
}
static volatile uint64_t *active_workers(void)
{return (volatile uint64_t *)(uintptr_t)(RUNTIME_TLS_BASE+16);}
static void worker_finished(void)
{__atomic_sub_fetch(active_workers(),1,__ATOMIC_RELEASE);wake64(active_workers(),~0u);}
extern _Noreturn void MyOSMainThreadExit(uint64_t);
_Noreturn void pthread_exit(void *result)
{
    struct thread_control *control=self_control();
    if(control)control->result=result;
    run_destructors();
    if(control){
        __atomic_store_n(&control->done,1,__ATOMIC_RELEASE);wake64(&control->done,~0u);
        worker_finished();
        (void)call3(SYS_THREAD_EXIT,0,0,0);
    }else{
        /* Preserve the owned bootstrap/finalizer frame, not the caller's
         * abandoned stack frame. Main termination must not kill workers. */
        uint64_t frame=*(const uint64_t *)(uintptr_t)(RUNTIME_TLS_BASE+56);
        if(!frame)__builtin_trap();
        *(void **)(uintptr_t)(RUNTIME_TLS_BASE+24)=result;
        if(call4(SYS_SIGNAL,7,0,0,0)<0)__builtin_trap();
        volatile uint64_t *flags=(volatile uint64_t *)(uintptr_t)(RUNTIME_TLS_BASE+40);
        __atomic_fetch_or(flags,1,__ATOMIC_RELEASE);wake64(flags,~0u);
        for(;;){uint64_t count=__atomic_load_n(active_workers(),__ATOMIC_ACQUIRE);
            if(!count)break;(void)wait64(active_workers(),count,0);}
        MyOSMainThreadExit(frame);
    }
    __builtin_trap();
}
static void thread_entry(void *raw)
{
    struct thread_control *control=raw;
    *(struct thread_control **)(uintptr_t)(self_page()+8)=control;
    void **values=key_values();for(unsigned i=0;i<THREAD_KEYS;++i)values[i]=0;
    pthread_exit(control->start(control->argument));
}
int pthread_detach(uint64_t thread);
int pthread_attr_init(void *raw)
{
    if(!raw)return EINVAL;struct thread_attr *a=raw;
    *a=(struct thread_attr){0x41545452,THREAD_STACK_SIZE,0,0};return 0;
}
int pthread_attr_destroy(void *raw){if(!raw)return EINVAL;*(uint64_t *)raw=0;return 0;}
int pthread_attr_setstacksize(void *raw,size_t size)
{
    if(!raw||size<16384||size>16u*1024u*1024u)return EINVAL;
    ((struct thread_attr *)raw)->stack_size=(size+4095)&~4095ull;return 0;
}
int pthread_attr_getstacksize(const void *raw,size_t *size)
{if(!raw||!size)return EINVAL;*size=((const struct thread_attr *)raw)->stack_size;return 0;}
int pthread_attr_setdetachstate(void *raw,int state)
{if(!raw||(state!=0&&state!=1))return EINVAL;((struct thread_attr *)raw)->detached=(unsigned)state;return 0;}
int pthread_create(uint64_t *thread,const void *raw,void *(*start)(void *),void *argument)
{
    if(!thread||!start)return EINVAL;
    size_t length=raw?((const struct thread_attr *)raw)->stack_size:THREAD_STACK_SIZE;
    if(length<16384||length>16u*1024u*1024u)return EINVAL;
    length=(length+4095)&~4095ull;
    void *mapping=mmap(0,length,POSIX_PROT_READ|POSIX_PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
    if(mapping==(void *)(uintptr_t)-1)return EAGAIN;
    struct thread_control *control=mapping;
    *control=(struct thread_control){THREAD_MAGIC,0,(uint64_t)(uintptr_t)mapping,length,
        start,argument,0,0,0,0,{0,0},{0}};
    __atomic_add_fetch(active_workers(),1,__ATOMIC_RELAXED);
    int64_t tid=call4(SYS_THREAD_CREATE,(uintptr_t)thread_entry,(uintptr_t)control,
                      (uintptr_t)mapping+length,0);
    if(tid<0){worker_finished();munmap(mapping,length);return (int)-tid;}
    control->tid=(uint64_t)tid;*thread=(uint64_t)(uintptr_t)control;
    if(raw&&((const struct thread_attr *)raw)->detached){
        int rc=pthread_detach(*thread);if(rc)return rc;
    }
    return 0;
}
uint64_t pthread_self(void)
{
    struct thread_control *control=self_control();
    return control?(uint64_t)(uintptr_t)control:RUNTIME_TLS_BASE;
}
int pthread_equal(uint64_t first,uint64_t second){return first==second;}
/* Join and detach must claim a single word: independent flags allow both
 * callers to win and race a stack unmap against the joiner's result read. */
static __attribute__((noinline)) int claim_thread(volatile uint64_t *flags,uint64_t bit)
{
    uint64_t value=__atomic_load_n(flags,__ATOMIC_ACQUIRE);
    for(;;){if(value&6)return EINVAL;
        if(__atomic_compare_exchange_n(flags,&value,value|bit,0,__ATOMIC_ACQ_REL,__ATOMIC_ACQUIRE))return 0;}
}
int pthread_join(uint64_t thread,void **result)
{
    if(thread==pthread_self())return EDEADLK;
    if(thread==RUNTIME_TLS_BASE){
        volatile uint64_t *flags=(volatile uint64_t *)(uintptr_t)(RUNTIME_TLS_BASE+40);
        int rc=claim_thread(flags,2);if(rc)return rc;uint64_t value;
        for(;;){value=__atomic_load_n(flags,__ATOMIC_ACQUIRE);if(value&1)break;
            (void)wait64(flags,value,0);}
        if(result)*result=*(void **)(uintptr_t)(RUNTIME_TLS_BASE+24);return 0;
    }
    struct thread_control *control=thread_control(thread);if(!control)return ESRCH;
    int claimed=claim_thread(&control->joined,2);if(claimed)return claimed;
    int32_t status;
    for(;;){int rc=(int)call3(SYS_THREAD_REAP,control->tid,(uintptr_t)&status,0);
        if(!rc)break;if(rc!=-EAGAIN)return -rc;
        uint64_t deadline=ticks()+1;
        (void)wait64(&control->done,0,deadline);yield_cpu();
    }
    if(result)*result=control->result;
    uint64_t base=control->base,length=control->length;control->magic=0;
    return munmap((void *)(uintptr_t)base,length)?EINVAL:0;
}
int pthread_detach(uint64_t thread)
{
    if(thread==RUNTIME_TLS_BASE)return claim_thread((uint64_t *)(uintptr_t)(RUNTIME_TLS_BASE+40),4);
    struct thread_control *control=thread_control(thread);if(!control)return ESRCH;
    int claimed=claim_thread(&control->joined,4);if(claimed)return claimed;
    __atomic_store_n(&control->detached,1,__ATOMIC_RELEASE);
    int64_t rc=call3(SYS_THREAD_DETACH,control->tid,control->base,control->length);
    if(rc<0){__atomic_store_n(&control->detached,0,__ATOMIC_RELEASE);
        __atomic_store_n(&control->joined,0,__ATOMIC_RELEASE);return (int)-rc;}
    return 0;
}
int pthread_getattr_np(uint64_t thread,void *raw)
{
    if(!raw)return EINVAL;
    struct thread_attr *a=raw;*a=(struct thread_attr){0x41545452,THREAD_STACK_SIZE,0,0};
    if(thread==RUNTIME_TLS_BASE){struct main_stack_info info;
        int64_t rc=call3(SYS_MAIN_STACK_INFO,(uintptr_t)&info,0,0);if(rc<0)return (int)-rc;
        a->stack_base=info.base;a->stack_size=info.length;return 0;}
    struct thread_control *control=thread_control(thread);if(!control)return ESRCH;
    a->stack_base=control->base;a->stack_size=control->length;
    a->detached=control->detached;return 0;
}
int pthread_attr_getstack(const void *raw,void **stack,size_t *size)
{
    if(!raw||!stack||!size)return EINVAL;
    const struct thread_attr *a=raw;*stack=(void *)(uintptr_t)a->stack_base;
    *size=a->stack_size;return 0;
}
int pthread_setname_np(uint64_t thread,const char *name)
{
    if(!name)return EINVAL;char *target;
    if(thread==RUNTIME_TLS_BASE)target=main_thread_name();
    else {struct thread_control *control=thread_control(thread);if(!control)return ESRCH;target=control->name;}
    unsigned length=0;while(length<16&&name[length])++length;
    if(length==16)return 34;
    for(unsigned i=0;i<=length;++i)target[i]=name[i];return 0;
}
int pthread_getname_np(uint64_t thread,char *name,size_t size)
{
    if(!name)return EINVAL;if(size<16)return 34;
    if(thread==RUNTIME_TLS_BASE){const char fallback[]="main";
        const char *source=main_thread_name()[0]?main_thread_name():fallback;size_t i=0;
        for(;i<15&&source[i];++i)name[i]=source[i];name[i]=0;return 0;}
    struct thread_control *control=thread_control(thread);if(!control)return ESRCH;
    size_t i=0;for(;i<15&&control->name[i];++i)name[i]=control->name[i];name[i]=0;return 0;
}
int pthread_getschedparam(uint64_t thread,int *policy,void *parameter)
{
    if(!policy||!parameter)return EINVAL;
    if(thread!=RUNTIME_TLS_BASE&&!thread_control(thread))return ESRCH;
    *policy=0;*(int *)parameter=0;return 0;
}
int pthread_kill(uint64_t thread,int signal)
{
    uint64_t tid;
    if(thread==RUNTIME_TLS_BASE)tid=(uint64_t)call3(2,0,0,0);
    else {struct thread_control *control=thread_control(thread);
        if(!control)return ESRCH;tid=control->tid;}
    return (int)-call4(SYS_SIGNAL,5,tid,(unsigned)signal,0);
}
int pause(void)
{
    int64_t result=call4(SYS_SIGNAL,6,0,0,0);
    *__errno_location()=(int)-result;return -1;
}
int pthread_sigmask(int how,const void *set,void *old)
{
    return (int)-call4(SYS_SIGNAL,1,(unsigned)how,(uintptr_t)set,(uintptr_t)old);
}
typedef void (*signal_handler)(int);
struct runtime_signal_action {
    signal_handler handler;uint64_t mask[16];uint32_t flags,padding;
    void (*restorer)(void);
};
int sigaction(int signal,const void *action,void *old)
{
    int64_t result=call4(SYS_SIGNAL,0,(unsigned)signal,(uintptr_t)action,(uintptr_t)old);
    if(result<0){*__errno_location()=(int)-result;return -1;}return 0;
}
signal_handler signal(int number,signal_handler handler)
{
    struct runtime_signal_action action={handler,{0},0,0,0},old;
    /* Interruptible targeted sends are supported; no SA_RESTART claim. */
    if(sigaction(number,&action,&old))return (signal_handler)(uintptr_t)-1;
    return old.handler;
}
int raise(int signal)
{
    int64_t result=call3(SYS_SIGNAL,2,(unsigned)signal,0);
    if(result<0){*__errno_location()=(int)-result;return -1;}return 0;
}
_Noreturn void abort(void)
{
    (void)raise(6);
    /* A returned/ignored/blocked SIGABRT handler cannot prevent the second,
     * allocation-independent default SIGABRT termination. No fake exit134. */
    (void)call3(SYS_SIGNAL,4,6,0);
    __builtin_unreachable();
}

struct mutex_layout { volatile uint64_t state;uint64_t owner;uint32_t recursion,type; };
int pthread_mutexattr_init(void *a){if(!a)return EINVAL;*(uint32_t *)a=0;return 0;}
int pthread_mutexattr_destroy(void *a){return a?0:EINVAL;}
int pthread_mutexattr_settype(void *a,int type)
{if(!a||type<0||type>2)return EINVAL;*(uint32_t *)a=(uint32_t)type;return 0;}
int pthread_mutex_init(void *raw,const void *attr)
{
    if(!raw)return EINVAL;struct mutex_layout *m=raw;
    *m=(struct mutex_layout){0,0,0,attr?*(const uint32_t *)attr:0};return 0;
}
int pthread_mutex_destroy(void *raw)
{if(!raw)return EINVAL;return __atomic_load_n(&((struct mutex_layout *)raw)->state,__ATOMIC_ACQUIRE)?EBUSY:0;}
int pthread_mutex_trylock(void *raw)
{
    if(!raw)return EINVAL;struct mutex_layout *m=raw;uint64_t self=pthread_self();
    if(m->type==1&&m->owner==self){++m->recursion;return 0;}
    uint64_t zero=0;if(!__atomic_compare_exchange_n(&m->state,&zero,1,0,__ATOMIC_ACQUIRE,__ATOMIC_RELAXED))
        return m->type==2&&m->owner==self?EDEADLK:EBUSY;
    m->owner=self;m->recursion=1;return 0;
}
int pthread_mutex_lock(void *raw)
{
    if(!raw)return EINVAL;for(;;){int rc=pthread_mutex_trylock(raw);
        if(rc!=EBUSY)return rc;(void)wait64(&((struct mutex_layout *)raw)->state,1,0);}
}
int pthread_mutex_timedlock(void *raw,const struct timespec *time)
{
    int rc=pthread_mutex_trylock(raw);if(rc!=EBUSY)return rc;
    if(time&&time->tv_sec<0&&time->tv_nsec>=0&&time->tv_nsec<1000000000)return ETIMEDOUT;
    uint64_t deadline;rc=deadline_ticks(0,time,&deadline);if(rc)return rc;
    for(;;){
        rc=pthread_mutex_trylock(raw);if(rc!=EBUSY)return rc;
        if(deadline<=ticks())return ETIMEDOUT;
        rc=wait64(&((struct mutex_layout *)raw)->state,1,deadline);
        if(rc==-ETIMEDOUT)return ETIMEDOUT;if(rc&&rc!=-EAGAIN&&rc!=-4)return -rc;
    }
}
int pthread_mutex_unlock(void *raw)
{
    if(!raw)return EINVAL;struct mutex_layout *m=raw;
    if(!__atomic_load_n(&m->state,__ATOMIC_ACQUIRE)||m->owner!=pthread_self())return 1;
    if(m->type==1&&m->recursion>1){--m->recursion;return 0;}
    m->owner=0;m->recursion=0;__atomic_store_n(&m->state,0,__ATOMIC_RELEASE);wake64(&m->state,1);return 0;
}

int pthread_condattr_init(void *a){if(!a)return EINVAL;*(uint32_t *)a=0;return 0;}
int pthread_condattr_destroy(void *a){return a?0:EINVAL;}
int pthread_condattr_setclock(void *a,int clock)
{if(!a||(clock!=0&&clock!=1))return EINVAL;*(uint32_t *)a=(uint32_t)clock;return 0;}
int pthread_cond_init(void *raw,const void *attr)
{if(!raw)return EINVAL;struct condition_layout *c=raw;
 *c=(struct condition_layout){0,attr?*(const uint32_t *)attr:0,0};return 0;}
int pthread_cond_destroy(void *raw)
{if(!raw)return EINVAL;return __atomic_load_n(&((struct condition_layout *)raw)->waiters,__ATOMIC_ACQUIRE)?EBUSY:0;}
int pthread_cond_signal(void *raw)
{if(!raw)return EINVAL;struct condition_layout *c=raw;
 __atomic_add_fetch(&c->sequence,1,__ATOMIC_RELEASE);wake64(&c->sequence,1);return 0;}
int pthread_cond_broadcast(void *raw)
{if(!raw)return EINVAL;struct condition_layout *c=raw;
 __atomic_add_fetch(&c->sequence,1,__ATOMIC_RELEASE);wake64(&c->sequence,~0u);return 0;}
static int cond_wait(void *raw,void *mutex,uint64_t deadline)
{
    if(!raw||!mutex)return EINVAL;struct condition_layout *c=raw;
    uint64_t value=__atomic_load_n(&c->sequence,__ATOMIC_ACQUIRE);
    __atomic_add_fetch(&c->waiters,1,__ATOMIC_ACQ_REL);
    int rc=pthread_mutex_unlock(mutex);
    if(rc){__atomic_sub_fetch(&c->waiters,1,__ATOMIC_ACQ_REL);return rc;}
    rc=wait64(&c->sequence,value,deadline);
    __atomic_sub_fetch(&c->waiters,1,__ATOMIC_ACQ_REL);int lock=pthread_mutex_lock(mutex);
    if(lock)return lock;return rc==-ETIMEDOUT?ETIMEDOUT:0;
}
int pthread_cond_wait(void *raw,void *mutex){return cond_wait(raw,mutex,0);}
int pthread_cond_timedwait(void *raw,void *mutex,const struct timespec *time)
{
    if(!raw)return EINVAL;uint64_t d;
    int converted=deadline_ticks(((struct condition_layout *)raw)->clock,time,&d);
    if(converted)return converted;if(d<=ticks()){
        int rc=pthread_mutex_unlock(mutex);if(rc)return rc;
        rc=pthread_mutex_lock(mutex);return rc?rc:ETIMEDOUT;
    }
    return cond_wait(raw,mutex,d);
}

int sem_init(void *raw,int shared,unsigned value)
{
    if(!raw||value>0x7fffffff){*__errno_location()=EINVAL;return -1;}
    if(shared){*__errno_location()=ENOSYS;return -1;} /* No process-shared VM. */
    *(uint64_t *)raw=value;return 0;
}
int sem_destroy(void *raw){if(!raw){*__errno_location()=EINVAL;return -1;}return 0;}
int sem_post(void *raw)
{
    if(!raw){*__errno_location()=EINVAL;return -1;}volatile uint64_t *count=raw;
    uint64_t value=__atomic_load_n(count,__ATOMIC_RELAXED);
    for(;;){
        if(value>=0x7fffffff){*__errno_location()=EOVERFLOW;return -1;}
        if(__atomic_compare_exchange_n(count,&value,value+1,0,__ATOMIC_RELEASE,__ATOMIC_RELAXED))break;
    }
    wake64(raw,1);return 0;
}
static int sem_take(void *raw,uint64_t deadline)
{
    if(!raw){*__errno_location()=EINVAL;return -1;}volatile uint64_t *count=raw;
    for(;;){uint64_t value=__atomic_load_n(count,__ATOMIC_ACQUIRE);
        if(value&&__atomic_compare_exchange_n(count,&value,value-1,0,__ATOMIC_ACQUIRE,__ATOMIC_RELAXED))return 0;
        int rc=wait64(count,0,deadline);if(rc==-ETIMEDOUT){*__errno_location()=ETIMEDOUT;return -1;}
        if(rc&&rc!=-EAGAIN){*__errno_location()=-rc;return -1;}
    }
}
int sem_wait(void *raw){return sem_take(raw,0);}
int sem_timedwait(void *raw,const struct timespec *time)
{
    uint64_t d;int rc=deadline_ticks(0,time,&d);
    if(rc){*__errno_location()=rc;return -1;}
    return sem_take(raw,d);
}

int pthread_rwlock_init(void *raw,const void *attr)
{if(!raw)return EINVAL;(void)attr;*(int64_t *)raw=0;return 0;}
int pthread_rwlock_destroy(void *raw)
{if(!raw)return EINVAL;return __atomic_load_n((int64_t *)raw,__ATOMIC_ACQUIRE)?EBUSY:0;}
int pthread_rwlock_rdlock(void *raw)
{
    if(!raw)return EINVAL;volatile int64_t *state=raw;
    for(;;){int64_t value=__atomic_load_n(state,__ATOMIC_ACQUIRE);
        if(value>=0&&__atomic_compare_exchange_n(state,&value,value+1,0,__ATOMIC_ACQUIRE,__ATOMIC_RELAXED))return 0;
        (void)wait64((uint64_t *)raw,(uint64_t)value,0);
    }
}
int pthread_rwlock_wrlock(void *raw)
{
    if(!raw)return EINVAL;volatile int64_t *state=raw;
    for(;;){int64_t zero=0;if(__atomic_compare_exchange_n(state,&zero,-1,0,__ATOMIC_ACQUIRE,__ATOMIC_RELAXED))return 0;
        (void)wait64((uint64_t *)raw,(uint64_t)zero,0);
    }
}
int pthread_rwlock_unlock(void *raw)
{
    if(!raw)return EINVAL;volatile int64_t *state=raw;int64_t value=__atomic_load_n(state,__ATOMIC_ACQUIRE);
    if(value==-1)__atomic_store_n(state,0,__ATOMIC_RELEASE);
    else if(value>0)__atomic_sub_fetch(state,1,__ATOMIC_RELEASE);else return 1;
    wake64((uint64_t *)raw,~0u);return 0;
}
int pthread_once(volatile int *once,void (*routine)(void))
{
    if(!once||!routine)return EINVAL;int value=__atomic_load_n(once,__ATOMIC_ACQUIRE);
    if(value==2)return 0;int zero=0;
    if(__atomic_compare_exchange_n(once,&zero,1,0,__ATOMIC_ACQUIRE,__ATOMIC_RELAXED)){
        routine();__atomic_store_n(once,2,__ATOMIC_RELEASE);return 0;
    }
    while(__atomic_load_n(once,__ATOMIC_ACQUIRE)!=2)yield_cpu();return 0;
}
/* Pinned musl_pthread_key_t is void *; its opaque value is a bounded slot
 * index. Keep the API pointer-typed and validate the complete opaque value. */
int pthread_key_create(void **key,void (*destructor)(void *))
{
    if(!key)return EINVAL;
    for(unsigned i=0;i<THREAD_KEYS;++i){uint32_t bit=1u<<i,old=__atomic_fetch_or(key_bits(),bit,__ATOMIC_ACQ_REL);
        if(!(old&bit)){clear_key_values(i);key_destructors()[i]=destructor;*key=(void *)(uintptr_t)i;return 0;}}
    return EAGAIN;
}
int pthread_key_delete(void *raw_key)
{
    uintptr_t key=(uintptr_t)raw_key;
    if(key>=THREAD_KEYS)return EINVAL;uint32_t bit=1u<<key;
    if(!(__atomic_fetch_and(key_bits(),~bit,__ATOMIC_ACQ_REL)&bit))return EINVAL;
    key_destructors()[key]=0;clear_key_values(key);return 0;
}
int pthread_setspecific(void *raw_key,const void *value)
{
    uintptr_t key=(uintptr_t)raw_key;
    if(key>=THREAD_KEYS||!(__atomic_load_n(key_bits(),__ATOMIC_ACQUIRE)&(1u<<key)))return EINVAL;
    key_values()[key]=(void *)value;return 0;
}
void *pthread_getspecific(void *raw_key)
{
    uintptr_t key=(uintptr_t)raw_key;
    if(key>=THREAD_KEYS||!(__atomic_load_n(key_bits(),__ATOMIC_ACQUIRE)&(1u<<key)))return 0;
    return key_values()[key];
}


/* Process-wide descriptor objects.  Operations take an object reference before
 * dropping the table lock; close removes the generation and wakes every wait.
 * This is also the lifetime model intended for file and socket backends. */
#define SYS_WRITE 1u
#define SYS_FOPEN 5u
#define SYS_FREAD 32u
#define SYS_FWRITE 33u
#define SYS_FCLOSE 8u
#define SYS_MKDIR 9u
#define SYS_UNLINK 10u
#define SYS_READDIR 11u
#define SYS_STAT 12u
#define SYS_FSEEK 29u
#define SYS_FSTAT 30u
#define SYS_FTRUNCATE 31u
#define SYS_FSYNC 34u
#define SYS_TCP_CREATE 35u
#define SYS_TCP_CONNECT 36u
#define SYS_TCP_STATUS 37u
#define SYS_TCP_SEND 38u
#define SYS_TCP_RECV 39u
#define SYS_TCP_SHUTDOWN 40u
#define SYS_TCP_CLOSE 41u
#define SYS_TCP_BIND 53u
#define SYS_TCP_LISTEN 54u
#define SYS_TCP_ACCEPT 55u
#define FD_BASE 64
#define FD_SLOTS 32u
#define PIPE_CAPACITY 4096u
#define EPOLL_REGS 32u
#define O_NONBLOCK 04000
#define O_APPEND 02000
#define O_CLOEXEC 02000000
#define POLLIN 0x0001u
#define POLLOUT 0x0004u
#define POLLERR 0x0008u
#define POLLHUP 0x0010u
#define POLLNVAL 0x0020u
#define EPOLL_CTL_ADD 1
#define EPOLL_CTL_DEL 2
#define EPOLL_CTL_MOD 3
#define EBADF 9
#define EFAULT 14
#define ENOMEM 12
#define EEXIST 17
#define ENOENT 2
#define ENOSPC 28
#define EPIPE 32
#define ENOTSUP 95

enum descriptor_type { DESCRIPTOR_EVENT=1,DESCRIPTOR_PIPE=2,DESCRIPTOR_EPOLL=3,DESCRIPTOR_FILE=4,DESCRIPTOR_DIRECTORY=5,DESCRIPTOR_SOCKETPAIR=6,DESCRIPTOR_TCP=7 };
struct tcp_address_abi { unsigned char ip[4];uint16_t port,reserved; };
struct tcp_status_abi { uint32_t ready,available,error,state;struct tcp_address_abi local,peer; };
struct sockaddr_in_abi { uint16_t family;unsigned char port[2],ip[4],padding[8]; };
struct epoll_event { uint32_t events;uint64_t data __attribute__((packed)); } __attribute__((packed));
struct pollfd { int32_t fd;uint16_t events,revents; };
struct descriptor_object;
struct epoll_registration {
    struct descriptor_object *object;
    uint32_t slot,generation,events,used,access;
    uint64_t data;
};
struct descriptor_object {
    volatile uint64_t lock,sequence;
    volatile uint32_t references;
    uint32_t type,status_flags;
    /* Open-description options; socketpair endpoints remain independent.
     * Zero means infinite, UINT64_MAX means immediate (negative timeval). */
    uint64_t receive_timeout[2],send_timeout[2];
    union {
        struct { uint64_t counter;uint32_t semaphore,opens; } event;
        struct {
            unsigned char data[PIPE_CAPACITY];
            uint32_t head,count,readers,writers;
        } pipe;
        struct { struct epoll_registration registrations[EPOLL_REGS]; } epoll;
        struct { int32_t kernel_fd; } file;
        struct { uint32_t handle,opens; } tcp;
        struct { char path[128]; } directory;
        struct {
            unsigned char bytes[2][PIPE_CAPACITY];
            uint32_t head[2],count[2],readers[2],writers[2],read_shutdown[2],write_shutdown[2],status_flags[2];
        } socketpair;
    } value;
};
struct descriptor_slot {
    struct descriptor_object *object;
    uint32_t generation,access,flags;
};
struct descriptor_state {
    volatile uint64_t lock,sequence;
    struct descriptor_slot slots[FD_SLOTS];
};
struct descriptor_ref {
    struct descriptor_object *object;
    uint32_t slot,generation,access,flags;
};

static struct descriptor_state **descriptor_state_slot(void)
{return (struct descriptor_state **)(uintptr_t)(RUNTIME_TLS_BASE+1536);}
static void spin_lock(volatile uint64_t *lock)
{while(__atomic_exchange_n(lock,1,__ATOMIC_ACQUIRE))yield_cpu();}
static void spin_unlock(volatile uint64_t *lock)
{__atomic_store_n(lock,0,__ATOMIC_RELEASE);}
static struct descriptor_state *descriptor_state(void)
{
    struct descriptor_state *state=__atomic_load_n(descriptor_state_slot(),__ATOMIC_ACQUIRE);
    if(state)return state;
    struct descriptor_state *created=malloc(sizeof(*created));
    if(!created)return 0;
    struct descriptor_state *empty=0;
    if(!__atomic_compare_exchange_n(descriptor_state_slot(),&empty,created,0,
                                    __ATOMIC_RELEASE,__ATOMIC_ACQUIRE)){
        free(created);return empty;
    }
    return created;
}
static struct descriptor_object *descriptor_object(unsigned type)
{
    struct descriptor_object *object=malloc(sizeof(*object));
    if(!object)return 0;object->type=type;return object;
}
static void object_release(struct descriptor_object *object);
static void object_release(struct descriptor_object *object)
{
    if(__atomic_sub_fetch(&object->references,1,__ATOMIC_ACQ_REL))return;
    if(object->type==DESCRIPTOR_EPOLL)
        for(unsigned i=0;i<EPOLL_REGS;++i)
            if(object->value.epoll.registrations[i].used)
                object_release(object->value.epoll.registrations[i].object);
    if(object->type==DESCRIPTOR_FILE)
        (void)call3(SYS_FCLOSE,(unsigned)object->value.file.kernel_fd,0,0);
    if(object->type==DESCRIPTOR_TCP&&object->value.tcp.handle)
        (void)call3(SYS_TCP_CLOSE,object->value.tcp.handle,0,0);
    free(object);
}
static int descriptor_install_from_locked(struct descriptor_state *state,
                                          struct descriptor_object *object,
                                          unsigned access,unsigned flags,
                                          unsigned first)
{
    for(unsigned i=first;i<FD_SLOTS;++i)if(!state->slots[i].object){
        struct descriptor_slot *slot=&state->slots[i];
        if(!object->references)object->status_flags=flags&(O_APPEND|O_NONBLOCK);
        if(!slot->generation)slot->generation=1;
        slot->object=object;slot->access=access;slot->flags=flags;
        __atomic_add_fetch(&object->references,1,__ATOMIC_RELAXED);
        return FD_BASE+(int)i;
    }
    return -1;
}
static int descriptor_install_locked(struct descriptor_state *state,
                                     struct descriptor_object *object,
                                     unsigned access,unsigned flags)
{return descriptor_install_from_locked(state,object,access,flags,0);}
static int descriptor_acquire(int fd,struct descriptor_ref *reference)
{
    struct descriptor_state *state=descriptor_state();
    if(!state||fd<FD_BASE||fd>=FD_BASE+(int)FD_SLOTS)return 0;
    unsigned index=(unsigned)(fd-FD_BASE);spin_lock(&state->lock);
    struct descriptor_slot *slot=&state->slots[index];
    if(!slot->object){spin_unlock(&state->lock);return 0;}
    __atomic_add_fetch(&slot->object->references,1,__ATOMIC_RELAXED);
    unsigned flags=(slot->flags&~(O_APPEND|O_NONBLOCK))|
        (slot->object->type==DESCRIPTOR_SOCKETPAIR?
         __atomic_load_n(&slot->object->value.socketpair.status_flags[(slot->access>>2)&1],__ATOMIC_ACQUIRE):
         __atomic_load_n(&slot->object->status_flags,__ATOMIC_ACQUIRE));
    *reference=(struct descriptor_ref){slot->object,index,slot->generation,slot->access,flags};
    spin_unlock(&state->lock);return 1;
}
static int descriptor_current(const struct descriptor_ref *reference)
{
    struct descriptor_state *state=descriptor_state();if(!state)return 0;
    int current;spin_lock(&state->lock);
    struct descriptor_slot *slot=&state->slots[reference->slot];
    current=slot->object==reference->object&&slot->generation==reference->generation;
    spin_unlock(&state->lock);return current;
}
static void descriptor_changed(struct descriptor_object *target)
{
    __atomic_add_fetch(&target->sequence,1,__ATOMIC_RELEASE);
    wake64(&target->sequence,~0u);
    struct descriptor_state *state=descriptor_state();if(!state)return;
    __atomic_add_fetch(&state->sequence,1,__ATOMIC_RELEASE);wake64(&state->sequence,~0u);
    spin_lock(&state->lock);
    for(unsigned i=0;i<FD_SLOTS;++i){
        struct descriptor_object *epoll=state->slots[i].object;
        if(!epoll||epoll->type!=DESCRIPTOR_EPOLL)continue;
        spin_lock(&epoll->lock);
        for(unsigned j=0;j<EPOLL_REGS;++j)
            if(epoll->value.epoll.registrations[j].used&&
               epoll->value.epoll.registrations[j].object==target){
                __atomic_add_fetch(&epoll->sequence,1,__ATOMIC_RELEASE);
                wake64(&epoll->sequence,~0u);break;
            }
        spin_unlock(&epoll->lock);
    }
    spin_unlock(&state->lock);
}
static uint32_t object_ready(struct descriptor_object *object,unsigned access)
{
    uint32_t ready=0;spin_lock(&object->lock);
    if(object->type==DESCRIPTOR_EVENT){
        if(object->value.event.counter)ready|=POLLIN;
        if(object->value.event.counter<UINT64_MAX-1)ready|=POLLOUT;
    } else if(object->type==DESCRIPTOR_PIPE){
        if((access&1)&&object->value.pipe.count)ready|=POLLIN;
        if((access&1)&&!object->value.pipe.writers)ready|=POLLHUP;
        if((access&2)&&object->value.pipe.readers&&object->value.pipe.count<PIPE_CAPACITY)ready|=POLLOUT;
        if((access&2)&&!object->value.pipe.readers)ready|=POLLERR|POLLHUP;
    } else if(object->type==DESCRIPTOR_SOCKETPAIR){
        unsigned side=(access>>2)&1,peer=side^1;
        if(object->value.socketpair.count[side]||object->value.socketpair.read_shutdown[side]||
           !object->value.socketpair.writers[peer]||object->value.socketpair.write_shutdown[peer])ready|=POLLIN;
        if(!object->value.socketpair.writers[peer]||object->value.socketpair.write_shutdown[peer])ready|=0x2000;
        /* A write that will fail immediately is also nonblocking-ready. */
        if(object->value.socketpair.write_shutdown[side]||!object->value.socketpair.readers[peer]||
           object->value.socketpair.read_shutdown[peer]||object->value.socketpair.count[peer]<PIPE_CAPACITY)ready|=POLLOUT;
        if((!object->value.socketpair.readers[peer]&&!object->value.socketpair.writers[peer])||
           (object->value.socketpair.read_shutdown[peer]&&object->value.socketpair.write_shutdown[peer])||
           (object->value.socketpair.read_shutdown[side]&&object->value.socketpair.write_shutdown[side]))ready|=POLLHUP;
    } else if(object->type==DESCRIPTOR_FILE||object->type==DESCRIPTOR_DIRECTORY){if(access&1)ready|=POLLIN;if(access&2)ready|=POLLOUT;}
    else if(object->type==DESCRIPTOR_TCP){struct tcp_status_abi status;
        int64_t rc=call3(SYS_TCP_STATUS,object->value.tcp.handle,(uintptr_t)&status,0);
        ready=rc<0?POLLERR|POLLHUP:status.ready;}
    spin_unlock(&object->lock);return ready;
}
static int set_error(int error){*__errno_location()=error;return -1;}

#define MSG_PEEK 2
#define MSG_DONTWAIT 0x40
#define MSG_NOSIGNAL 0x4000
#define ENOTSOCK 88
struct socket_timeval { int64_t seconds,microseconds; };
_Static_assert(sizeof(struct socket_timeval)==16,"x64 socket timeval ABI");
static uint64_t socket_timeout(struct descriptor_object *object,unsigned side,int writing)
{
    spin_lock(&object->lock);
    uint64_t timeout=writing?object->send_timeout[side]:object->receive_timeout[side];
    spin_unlock(&object->lock);return timeout;
}
static uint64_t socket_deadline(uint64_t timeout)
{
    if(!timeout)return 0;uint64_t now=ticks();
    if(timeout==UINT64_MAX)return now;
    return timeout>UINT64_MAX-1-now?UINT64_MAX-1:now+timeout;
}
static int64_t tcp_io_ref(const struct descriptor_ref *reference,void *buffer,size_t length,int flags,int writing)
{
    struct descriptor_object *object=reference->object;int64_t result=-1;
    if(length>INT64_MAX){set_error(EINVAL);goto done;}
    if(!buffer&&length){set_error(EFAULT);goto done;}
    if(flags&~(MSG_DONTWAIT|(writing?MSG_NOSIGNAL:MSG_PEEK))){set_error(ENOTSUP);goto done;}
    uint64_t timeout=socket_timeout(object,0,writing),deadline=socket_deadline(timeout);
    for(;;){
        if(!descriptor_current(reference)){set_error(EBADF);break;}
        spin_lock(&object->lock);uint64_t sequence=__atomic_load_n(&object->sequence,__ATOMIC_ACQUIRE);
        int64_t rc=call4(writing?SYS_TCP_SEND:SYS_TCP_RECV,object->value.tcp.handle,
                        (uintptr_t)buffer,length,writing?0:!!(flags&MSG_PEEK));
        unsigned status=__atomic_load_n(&object->status_flags,__ATOMIC_ACQUIRE);spin_unlock(&object->lock);
        if(rc>=0){result=rc;break;}
        if(rc!=-EAGAIN||(flags&MSG_DONTWAIT)||(status&O_NONBLOCK)){fail(rc);break;}
        if(timeout&&ticks()>=deadline){set_error(EAGAIN);break;}
        int waited=wait64(&object->sequence,sequence,deadline);
        if(waited<0&&waited!=-EAGAIN&&waited!=-ETIMEDOUT){fail(waited);break;}
    }
done:object_release(object);return result;
}
static int64_t socketpair_io_ref(const struct descriptor_ref *reference,void *buffer,size_t length,int flags,int writing)
{
    struct descriptor_ref r=*reference;
    struct descriptor_object *object=r.object;
    if(object->type==DESCRIPTOR_TCP)return tcp_io_ref(reference,buffer,length,flags,writing);
    if(length>INT64_MAX){object_release(object);return set_error(EINVAL);}
    if(!buffer&&length){object_release(object);return set_error(EFAULT);}
    if(flags&~(MSG_DONTWAIT|(writing?MSG_NOSIGNAL:MSG_PEEK))){object_release(object);return set_error(ENOTSUP);}
    if(object->type!=DESCRIPTOR_SOCKETPAIR){object_release(object);return set_error(ENOTSOCK);}
    unsigned side=(r.access>>2)&1,peer=side^1;int64_t result=-1;
    if(!length){result=0;goto done;}
    uint64_t timeout=socket_timeout(object,side,writing),deadline=socket_deadline(timeout);
    for(;;){
        if(!descriptor_current(&r)){set_error(EBADF);break;}
        spin_lock(&object->lock);
        uint64_t sequence=object->sequence;
        if(writing){
            if(object->value.socketpair.write_shutdown[side]||!object->value.socketpair.readers[peer]||
               object->value.socketpair.read_shutdown[peer]){spin_unlock(&object->lock);set_error(EPIPE);break;}
            unsigned space=PIPE_CAPACITY-object->value.socketpair.count[peer];
            if(space){
                size_t count=length<space?length:space;
                unsigned tail=(object->value.socketpair.head[peer]+object->value.socketpair.count[peer])%PIPE_CAPACITY;
                for(size_t i=0;i<count;++i){object->value.socketpair.bytes[peer][tail]=((unsigned char *)buffer)[i];tail=(tail+1)%PIPE_CAPACITY;}
                object->value.socketpair.count[peer]+=(unsigned)count;
                spin_unlock(&object->lock);descriptor_changed(object);result=(int64_t)count;break;
            }
        } else {
            if(object->value.socketpair.read_shutdown[side]){spin_unlock(&object->lock);result=0;break;}
            unsigned available=object->value.socketpair.count[side];
            if(available){
                size_t count=length<available?length:available;unsigned head=object->value.socketpair.head[side];
                for(size_t i=0;i<count;++i){((unsigned char *)buffer)[i]=object->value.socketpair.bytes[side][head];head=(head+1)%PIPE_CAPACITY;}
                if(!(flags&MSG_PEEK)){object->value.socketpair.head[side]=head;object->value.socketpair.count[side]-=(unsigned)count;}
                spin_unlock(&object->lock);if(!(flags&MSG_PEEK))descriptor_changed(object);result=(int64_t)count;break;
            }
            if(!object->value.socketpair.writers[peer]||object->value.socketpair.write_shutdown[peer]){
                spin_unlock(&object->lock);result=0;break;}
        }
        spin_unlock(&object->lock);
        if((flags&MSG_DONTWAIT)||(__atomic_load_n(&object->value.socketpair.status_flags[side],__ATOMIC_ACQUIRE)&O_NONBLOCK)){set_error(EAGAIN);break;}
        if(timeout&&ticks()>=deadline){set_error(EAGAIN);break;}
        int waited=wait64(&object->sequence,sequence,deadline);
        if(waited<0&&waited!=-EAGAIN&&waited!=-ETIMEDOUT){fail(waited);break;}
    }
done:object_release(object);return result;
}
static int64_t socketpair_io(int fd,void *buffer,size_t length,int flags,int writing)
{struct descriptor_ref r={0};if(!descriptor_acquire(fd,&r))return set_error(EBADF);
 return socketpair_io_ref(&r,buffer,length,flags,writing);}
int64_t send(int fd,const void *buffer,size_t length,int flags)
{return socketpair_io(fd,(void *)buffer,length,flags,1);}
int64_t recv(int fd,void *buffer,size_t length,int flags)
{return socketpair_io(fd,buffer,length,flags,0);}
int socket(int domain,int type,int protocol)
{
    if(domain!=2)return set_error(97);
    if((type&~(O_NONBLOCK|O_CLOEXEC))!=1||(protocol&&protocol!=6))return set_error(93);
    struct descriptor_state *state=descriptor_state();if(!state)return set_error(ENOMEM);
    struct descriptor_object *object=descriptor_object(DESCRIPTOR_TCP);if(!object)return set_error(ENOMEM);
    int64_t handle=call3(SYS_TCP_CREATE,(uintptr_t)&object->sequence,(uintptr_t)&state->sequence,0);
    if(handle<0){free(object);return fail(handle);}
    object->value.tcp.handle=(uint32_t)handle;object->value.tcp.opens=1;
    spin_lock(&state->lock);int fd=descriptor_install_locked(state,object,3,(unsigned)type&(O_NONBLOCK|O_CLOEXEC));
    spin_unlock(&state->lock);
    if(fd<0){(void)call3(SYS_TCP_CLOSE,(uint32_t)handle,0,0);free(object);return set_error(24);}return fd;
}
int connect(int fd,const void *raw,uint32_t length)
{
    if(!raw)return set_error(EFAULT);if(length<sizeof(struct sockaddr_in_abi))return set_error(EINVAL);
    const struct sockaddr_in_abi *address=raw;if(address->family!=2)return set_error(97);
    struct tcp_address_abi peer={{0},(uint16_t)(((unsigned)address->port[0]<<8)|address->port[1]),0};
    for(unsigned i=0;i<4;++i)peer.ip[i]=address->ip[i];
    struct descriptor_ref r={0};if(!descriptor_acquire(fd,&r))return set_error(EBADF);
    if(r.object->type!=DESCRIPTOR_TCP){object_release(r.object);return set_error(ENOTSOCK);}
    uint64_t timeout=socket_timeout(r.object,0,1),deadline=socket_deadline(timeout);
    spin_lock(&r.object->lock);int64_t rc=call3(SYS_TCP_CONNECT,r.object->value.tcp.handle,(uintptr_t)&peer,0);
    unsigned flags=r.object->status_flags;spin_unlock(&r.object->lock);
    if(rc==-115&&!(flags&O_NONBLOCK))for(;;){
        if(!descriptor_current(&r)){rc=-EBADF;break;}
        spin_lock(&r.object->lock);uint64_t sequence=__atomic_load_n(&r.object->sequence,__ATOMIC_ACQUIRE);
        /* SO_ERROR consumption by another thread must not hide handshake failure. */
        struct tcp_status_abi status;rc=call3(SYS_TCP_STATUS,r.object->value.tcp.handle,(uintptr_t)&status,2);
        spin_unlock(&r.object->lock);if(rc<0)break;
        if(status.error){rc=-(int64_t)status.error;break;}
        if(status.state==4){rc=0;break;}
        if(timeout&&ticks()>=deadline){rc=-115;break;}
        int waited=wait64(&r.object->sequence,sequence,deadline);
        if(waited<0&&waited!=-EAGAIN&&waited!=-ETIMEDOUT){rc=waited;break;}
    }
    object_release(r.object);return rc<0?fail(rc):0;
}
static int tcp_name(int fd,void *raw,uint32_t *length,int peer)
{
    if(!raw||!length)return set_error(EFAULT);struct descriptor_ref r={0};
    if(!descriptor_acquire(fd,&r))return set_error(EBADF);
    if(r.object->type!=DESCRIPTOR_TCP){object_release(r.object);return set_error(ENOTSOCK);}
    struct tcp_status_abi status;spin_lock(&r.object->lock);
    int64_t rc=call3(SYS_TCP_STATUS,r.object->value.tcp.handle,(uintptr_t)&status,0);spin_unlock(&r.object->lock);
    object_release(r.object);if(rc<0)return fail(rc);if(peer&&status.state!=4)return set_error(107);
    struct tcp_address_abi *address=peer?&status.peer:&status.local;
    struct sockaddr_in_abi output={2,{(unsigned char)(address->port>>8),(unsigned char)address->port},{0},{0}};
    for(unsigned i=0;i<4;++i)output.ip[i]=address->ip[i];
    unsigned count=*length;if(count>sizeof(output))count=sizeof(output);
    for(unsigned i=0;i<count;++i)((unsigned char *)raw)[i]=((unsigned char *)&output)[i];*length=sizeof(output);return 0;
}
int getsockname(int fd,void *address,uint32_t *length){return tcp_name(fd,address,length,0);}
int getpeername(int fd,void *address,uint32_t *length){return tcp_name(fd,address,length,1);}
int getsockopt(int fd,int level,int option,void *value,uint32_t *length)
{
    if(!length||(!value&&*length))return set_error(EFAULT);
    if(*length>INT32_MAX)return set_error(EINVAL);
    struct descriptor_ref r={0};if(!descriptor_acquire(fd,&r))return set_error(EBADF);
    struct descriptor_object *object=r.object;
    if(object->type!=DESCRIPTOR_TCP&&object->type!=DESCRIPTOR_SOCKETPAIR){object_release(object);return set_error(ENOTSOCK);}
    union { int number;struct socket_timeval time; } output={0};
    unsigned size=sizeof(output.number),side=object->type==DESCRIPTOR_SOCKETPAIR?(r.access>>2)&1:0;
    int64_t rc=0;spin_lock(&object->lock);
    if(level!=1)rc=-92;
    else if(option==20||option==21){
        uint64_t timeout=option==20?object->receive_timeout[side]:object->send_timeout[side];
        if(timeout==UINT64_MAX)timeout=0;
        output.time=(struct socket_timeval){(int64_t)(timeout/100),(int64_t)(timeout%100)*10000};size=sizeof(output.time);
    } else if(option==4&&object->type==DESCRIPTOR_TCP){struct tcp_status_abi status;
        rc=call3(SYS_TCP_STATUS,object->value.tcp.handle,(uintptr_t)&status,1);if(!rc)output.number=(int)status.error;}
    else if(option==4)output.number=0;
    else if(option==3)output.number=1;
    else if(option==7)output.number=4096;
    else if(option==8)output.number=object->type==DESCRIPTOR_TCP?8192:PIPE_CAPACITY;
    else rc=-92;
    spin_unlock(&object->lock);object_release(object);if(rc<0)return fail(rc);
    unsigned count=*length;if(count>size)count=size;
    for(unsigned i=0;i<count;++i)((unsigned char *)value)[i]=((unsigned char *)&output)[i];
    *length=count;return 0;
}
int setsockopt(int fd,int level,int option,const void *value,uint32_t length)
{
    struct descriptor_ref r={0};if(!descriptor_acquire(fd,&r))return set_error(EBADF);
    struct descriptor_object *object=r.object;int error=0;
    if(object->type!=DESCRIPTOR_TCP&&object->type!=DESCRIPTOR_SOCKETPAIR)error=ENOTSOCK;
    else if(level!=1||(option!=20&&option!=21))error=92;
    else if(length<sizeof(struct socket_timeval)||length>INT32_MAX)error=EINVAL;
    else if(!value)error=EFAULT;
    if(error){object_release(object);return set_error(error);}
    /* Copy first; failed validation must not partially change the option. */
    struct socket_timeval time;
    for(unsigned i=0;i<sizeof(time);++i)((unsigned char *)&time)[i]=((const unsigned char *)value)[i];
    if(time.microseconds<0||time.microseconds>=1000000){object_release(object);return set_error(33);}
    uint64_t timeout=0;
    if(time.seconds<0)timeout=UINT64_MAX;
    else if(time.seconds<(INT64_MAX/100-1))
        timeout=(uint64_t)time.seconds*100+((uint64_t)time.microseconds+9999)/10000;
    unsigned side=object->type==DESCRIPTOR_SOCKETPAIR?(r.access>>2)&1:0;
    spin_lock(&object->lock);
    if(option==20)object->receive_timeout[side]=timeout;else object->send_timeout[side]=timeout;
    spin_unlock(&object->lock);object_release(object);return 0;
}
int socketpair(int domain,int type,int protocol,int descriptors[2])
{
    if(!descriptors)return set_error(EFAULT);
    if(domain!=1)return set_error(97);
    if((type&~(O_NONBLOCK|O_CLOEXEC))!=1||protocol)return set_error(93);
    struct descriptor_state *state=descriptor_state();if(!state)return set_error(ENOMEM);
    struct descriptor_object *object=descriptor_object(DESCRIPTOR_SOCKETPAIR);if(!object)return set_error(ENOMEM);
    for(unsigned side=0;side<2;++side){
        object->value.socketpair.readers[side]=object->value.socketpair.writers[side]=1;
        object->value.socketpair.status_flags[side]=(unsigned)type&O_NONBLOCK;
    }
    spin_lock(&state->lock);
    unsigned flags=(unsigned)type&(O_NONBLOCK|O_CLOEXEC);
    int first=descriptor_install_locked(state,object,3,flags);
    int second=first<0?-1:descriptor_install_locked(state,object,7,flags);
    if(second<0&&first>=0){struct descriptor_slot *slot=&state->slots[first-FD_BASE];
        slot->object=0;slot->access=slot->flags=0;++slot->generation;object->references=0;}
    spin_unlock(&state->lock);
    if(second<0){free(object);return set_error(24);}
    descriptors[0]=first;descriptors[1]=second;return 0;
}
int shutdown(int fd,int how)
{
    if(how<0||how>2)return set_error(EINVAL);struct descriptor_ref r={0};
    if(!descriptor_acquire(fd,&r))return set_error(EBADF);
    if(r.object->type==DESCRIPTOR_TCP){spin_lock(&r.object->lock);
        int64_t rc=call3(SYS_TCP_SHUTDOWN,r.object->value.tcp.handle,(unsigned)how,0);
        spin_unlock(&r.object->lock);object_release(r.object);return rc<0?fail(rc):0;}
    if(r.object->type!=DESCRIPTOR_SOCKETPAIR){object_release(r.object);return set_error(ENOTSOCK);}
    unsigned side=(r.access>>2)&1;spin_lock(&r.object->lock);
    if(how==0||how==2){r.object->value.socketpair.read_shutdown[side]=1;r.object->value.socketpair.count[side]=0;}
    if(how==1||how==2)r.object->value.socketpair.write_shutdown[side]=1;
    spin_unlock(&r.object->lock);descriptor_changed(r.object);object_release(r.object);return 0;
}

int close(int fd)
{
    if(fd>=3&&fd<3+8){int64_t rc=call3(SYS_FCLOSE,(unsigned)fd,0,0);return rc<0?fail(rc):0;}
    struct descriptor_state *state=descriptor_state();
    if(!state||fd<FD_BASE||fd>=FD_BASE+(int)FD_SLOTS)return set_error(EBADF);
    unsigned index=(unsigned)(fd-FD_BASE);struct descriptor_slot closed;
    spin_lock(&state->lock);struct descriptor_slot *slot=&state->slots[index];
    if(!slot->object){spin_unlock(&state->lock);return set_error(EBADF);}
    closed=*slot;slot->object=0;slot->access=slot->flags=0;++slot->generation;
    for(unsigned i=0;i<FD_SLOTS;++i){
        struct descriptor_object *epoll=state->slots[i].object;
        if(!epoll||epoll->type!=DESCRIPTOR_EPOLL)continue;
        spin_lock(&epoll->lock);
        for(unsigned j=0;j<EPOLL_REGS;++j){struct epoll_registration *r=&epoll->value.epoll.registrations[j];
            if(r->used&&r->slot==index&&r->generation==closed.generation){
                r->used=0;object_release(r->object);__atomic_add_fetch(&epoll->sequence,1,__ATOMIC_RELEASE);
                wake64(&epoll->sequence,~0u);
            }
        }
        spin_unlock(&epoll->lock);
    }
    __atomic_add_fetch(&state->sequence,1,__ATOMIC_RELEASE);wake64(&state->sequence,~0u);
    spin_unlock(&state->lock);
    spin_lock(&closed.object->lock);
    if(closed.object->type==DESCRIPTOR_EVENT&&closed.object->value.event.opens)
        --closed.object->value.event.opens;
    if(closed.object->type==DESCRIPTOR_PIPE){
        if((closed.access&1)&&closed.object->value.pipe.readers)--closed.object->value.pipe.readers;
        if((closed.access&2)&&closed.object->value.pipe.writers)--closed.object->value.pipe.writers;
    }
    if(closed.object->type==DESCRIPTOR_SOCKETPAIR){unsigned side=(closed.access>>2)&1;
        --closed.object->value.socketpair.readers[side];--closed.object->value.socketpair.writers[side];}
    if(closed.object->type==DESCRIPTOR_TCP&&!--closed.object->value.tcp.opens){
        (void)call3(SYS_TCP_CLOSE,closed.object->value.tcp.handle,0,0);closed.object->value.tcp.handle=0;}
    spin_unlock(&closed.object->lock);descriptor_changed(closed.object);object_release(closed.object);return 0;
}
int64_t read(int fd,void *buffer,size_t length)
{
    if(fd>=3&&fd<3+8){int64_t rc=call3(SYS_FREAD,(unsigned)fd,(uintptr_t)buffer,length);
        if(rc<0){fail(rc);return -1;}return rc;}
    if(!buffer&&length){set_error(EFAULT);return -1;}
    struct descriptor_ref reference={0};if(!descriptor_acquire(fd,&reference)||!(reference.access&1)){
        if(reference.object)object_release(reference.object);set_error(EBADF);return -1;}
    struct descriptor_object *object=reference.object;int64_t result=-1;
    if(object->type==DESCRIPTOR_SOCKETPAIR)return socketpair_io_ref(&reference,buffer,length,0,0);
    if(object->type==DESCRIPTOR_TCP)return tcp_io_ref(&reference,buffer,length,0,0);
    if(object->type==DESCRIPTOR_EVENT){
        if(length!=8){set_error(EINVAL);goto done;}
        for(;;){spin_lock(&object->lock);uint64_t value=object->value.event.counter;
            if(value){uint64_t take=object->value.event.semaphore?1:value;
                object->value.event.counter-=take;*(uint64_t *)buffer=take;spin_unlock(&object->lock);
                descriptor_changed(object);result=8;break;}
            uint64_t sequence=object->sequence;spin_unlock(&object->lock);
            if(!descriptor_current(&reference)){set_error(EBADF);break;}
            if(reference.flags&O_NONBLOCK){set_error(EAGAIN);break;}
            int waited=wait64(&object->sequence,sequence,0);
            if(waited&&waited!=-EAGAIN){fail(waited);break;}
        }
    } else if(object->type==DESCRIPTOR_PIPE){
        if(!length){result=0;goto done;}
        for(;;){spin_lock(&object->lock);uint32_t count=object->value.pipe.count;
            if(count){if(count>length)count=(uint32_t)length;
                for(uint32_t i=0;i<count;++i){((unsigned char *)buffer)[i]=object->value.pipe.data[object->value.pipe.head];
                    object->value.pipe.head=(object->value.pipe.head+1)%PIPE_CAPACITY;}
                object->value.pipe.count-=count;spin_unlock(&object->lock);
                descriptor_changed(object);result=count;break;}
            uint32_t writers=object->value.pipe.writers;uint64_t sequence=object->sequence;spin_unlock(&object->lock);
            if(!descriptor_current(&reference)){set_error(EBADF);break;}
            if(!writers){result=0;break;}
            if(reference.flags&O_NONBLOCK){set_error(EAGAIN);break;}
            int waited=wait64(&object->sequence,sequence,0);
            if(waited&&waited!=-EAGAIN){fail(waited);break;}
        }
    } else if(object->type==DESCRIPTOR_FILE){spin_lock(&object->lock);
        result=call3(SYS_FREAD,(unsigned)object->value.file.kernel_fd,(uintptr_t)buffer,length);
        spin_unlock(&object->lock);if(result<0){fail(result);result=-1;}
    } else if(object->type==DESCRIPTOR_DIRECTORY)set_error(21);
    else set_error(EBADF);
done:object_release(object);return result;
}
int64_t write(int fd,const void *buffer,size_t length)
{
    if(fd==1||fd==2){int64_t rc=call3(SYS_WRITE,(uintptr_t)buffer,length,0);
        if(rc<0){fail(rc);return -1;}return rc;}
    if(fd>=3&&fd<3+8){int64_t rc=call3(SYS_FWRITE,(unsigned)fd,(uintptr_t)buffer,length);
        if(rc<0){fail(rc);return -1;}return rc;}
    if(!buffer&&length){set_error(EFAULT);return -1;}
    struct descriptor_ref reference={0};if(!descriptor_acquire(fd,&reference)||!(reference.access&2)){
        if(reference.object)object_release(reference.object);set_error(EBADF);return -1;}
    struct descriptor_object *object=reference.object;int64_t result=-1;
    if(object->type==DESCRIPTOR_SOCKETPAIR)return socketpair_io_ref(&reference,(void *)buffer,length,MSG_NOSIGNAL,1);
    if(object->type==DESCRIPTOR_TCP)return tcp_io_ref(&reference,(void *)buffer,length,MSG_NOSIGNAL,1);
    if(object->type==DESCRIPTOR_EVENT){
        if(length!=8){set_error(EINVAL);goto done;}
        uint64_t add=*(const uint64_t *)buffer;if(add==UINT64_MAX){set_error(EINVAL);goto done;}
        for(;;){spin_lock(&object->lock);uint64_t value=object->value.event.counter;
            if(add<=UINT64_MAX-1-value){object->value.event.counter=value+add;spin_unlock(&object->lock);
                descriptor_changed(object);result=8;break;}
            uint64_t sequence=object->sequence;spin_unlock(&object->lock);
            if(!descriptor_current(&reference)){set_error(EBADF);break;}
            if(reference.flags&O_NONBLOCK){set_error(EAGAIN);break;}
            int waited=wait64(&object->sequence,sequence,0);
            if(waited&&waited!=-EAGAIN){fail(waited);break;}
        }
    } else if(object->type==DESCRIPTOR_PIPE){
        if(!length){result=0;goto done;}
        for(;;){spin_lock(&object->lock);if(!object->value.pipe.readers){spin_unlock(&object->lock);set_error(EPIPE);break;}
            uint32_t space=PIPE_CAPACITY-object->value.pipe.count;
            if(space){uint32_t count=space<length?space:(uint32_t)length;
                uint32_t tail=(object->value.pipe.head+object->value.pipe.count)%PIPE_CAPACITY;
                for(uint32_t i=0;i<count;++i){object->value.pipe.data[tail]=((const unsigned char *)buffer)[i];
                    tail=(tail+1)%PIPE_CAPACITY;}
                object->value.pipe.count+=count;spin_unlock(&object->lock);
                descriptor_changed(object);result=count;break;}
            uint64_t sequence=object->sequence;spin_unlock(&object->lock);
            if(!descriptor_current(&reference)){set_error(EBADF);break;}
            if(reference.flags&O_NONBLOCK){set_error(EAGAIN);break;}
            int waited=wait64(&object->sequence,sequence,0);
            if(waited&&waited!=-EAGAIN){fail(waited);break;}
        }
    } else if(object->type==DESCRIPTOR_FILE){spin_lock(&object->lock);
        result=call4(SYS_FWRITE,(unsigned)object->value.file.kernel_fd,(uintptr_t)buffer,length,
                     __atomic_load_n(&object->status_flags,__ATOMIC_ACQUIRE)&O_APPEND);
        spin_unlock(&object->lock);if(result<0){fail(result);result=-1;}
    } else if(object->type==DESCRIPTOR_DIRECTORY)set_error(21);
    else set_error(EBADF);
done:object_release(object);return result;
}
int eventfd(unsigned initval,int flags)
{
    if(flags&~(1|O_NONBLOCK|O_CLOEXEC))return set_error(EINVAL);
    struct descriptor_state *state=descriptor_state();if(!state)return set_error(ENOMEM);
    struct descriptor_object *object=descriptor_object(DESCRIPTOR_EVENT);if(!object)return set_error(ENOMEM);
    object->value.event.counter=initval;object->value.event.semaphore=!!(flags&1);object->value.event.opens=1;
    spin_lock(&state->lock);int fd=descriptor_install_locked(state,object,3,(unsigned)flags);
    spin_unlock(&state->lock);if(fd<0){free(object);return set_error(ENOSPC);}return fd;
}
int pipe2(int fildes[2],int flags)
{
    if(!fildes)return set_error(EFAULT);
    if(flags&~(O_NONBLOCK|O_CLOEXEC))return set_error(EINVAL);
    struct descriptor_state *state=descriptor_state();if(!state)return set_error(ENOMEM);
    struct descriptor_object *object=descriptor_object(DESCRIPTOR_PIPE);if(!object)return set_error(ENOMEM);
    object->value.pipe.readers=object->value.pipe.writers=1;
    spin_lock(&state->lock);int first=descriptor_install_locked(state,object,1,(unsigned)flags);
    int second=first<0?-1:descriptor_install_locked(state,object,2,(unsigned)flags);
    if(second<0&&first>=0){struct descriptor_slot *slot=&state->slots[first-FD_BASE];
        slot->object=0;++slot->generation;object->references=0;}
    spin_unlock(&state->lock);
    if(first<0||second<0){free(object);return set_error(ENOSPC);}
    fildes[0]=first;fildes[1]=second;return 0;
}
int pipe(int fildes[2]){return pipe2(fildes,0);}

static int poll_deadline(int timeout,uint64_t *deadline)
{
    if(timeout< -1)return 0;if(timeout<0){*deadline=0;return 1;}
    uint64_t now=ticks(),add=((uint64_t)timeout+9)/10;
    *deadline=add>UINT64_MAX-1-now?UINT64_MAX-1:now+add;return 1;
}
int poll(struct pollfd *fds,uint64_t count,int32_t timeout)
{
    if((!fds&&count)||count>1024)return set_error(EFAULT);
    uint64_t deadline;if(!poll_deadline(timeout,&deadline))return set_error(EINVAL);
    struct descriptor_state *state=descriptor_state();if(!state)return set_error(ENOMEM);
    for(;;){int ready=0;uint64_t sequence=__atomic_load_n(&state->sequence,__ATOMIC_ACQUIRE);
        for(uint64_t i=0;i<count;++i){fds[i].revents=0;if(fds[i].fd<0)continue;
            struct descriptor_ref r={0};if(!descriptor_acquire(fds[i].fd,&r)){fds[i].revents=POLLNVAL;++ready;continue;}
            uint32_t events=object_ready(r.object,r.access),always=events&(POLLERR|POLLHUP);
            fds[i].revents=(uint16_t)((events&fds[i].events)|always);if(fds[i].revents)++ready;
            object_release(r.object);
        }
        if(ready||timeout==0)return ready;
        int rc=wait64(&state->sequence,sequence,deadline);if(rc==-ETIMEDOUT)return 0;
        if(rc&&rc!=-EAGAIN)return set_error(-rc);
    }
}
int epoll_create1(int flags)
{
    if(flags&~O_CLOEXEC)return set_error(EINVAL);
    struct descriptor_state *state=descriptor_state();if(!state)return set_error(ENOMEM);
    struct descriptor_object *object=descriptor_object(DESCRIPTOR_EPOLL);if(!object)return set_error(ENOMEM);
    spin_lock(&state->lock);int fd=descriptor_install_locked(state,object,3,(unsigned)flags);spin_unlock(&state->lock);
    if(fd<0){free(object);return set_error(ENOSPC);}return fd;
}
int epoll_ctl(int epfd,int operation,int fd,struct epoll_event *event)
{
    struct descriptor_ref epoll={0},target={0};
    if(!descriptor_acquire(epfd,&epoll)||epoll.object->type!=DESCRIPTOR_EPOLL){
        if(epoll.object)object_release(epoll.object);return set_error(EBADF);}
    if(!descriptor_acquire(fd,&target)||target.object->type==DESCRIPTOR_EPOLL){
        if(target.object)object_release(target.object);object_release(epoll.object);return set_error(EBADF);}
    if(operation!=EPOLL_CTL_DEL&&!event){object_release(target.object);object_release(epoll.object);return set_error(EFAULT);}
    int found=-1,empty=-1,result=0;spin_lock(&epoll.object->lock);
    for(unsigned i=0;i<EPOLL_REGS;++i){struct epoll_registration *r=&epoll.object->value.epoll.registrations[i];
        if(r->used&&r->slot==target.slot&&r->generation==target.generation)found=(int)i;
        if(!r->used&&empty<0)empty=(int)i;}
    if(operation==EPOLL_CTL_ADD){
        if(found>=0)result=EEXIST;else if(empty<0)result=ENOSPC;else {struct epoll_registration *r=&epoll.object->value.epoll.registrations[empty];
            *r=(struct epoll_registration){target.object,target.slot,target.generation,event->events,1,target.access,event->data};
            __atomic_add_fetch(&target.object->references,1,__ATOMIC_RELAXED);}
    } else if(operation==EPOLL_CTL_MOD){
        if(found<0)result=ENOENT;else {struct epoll_registration *r=&epoll.object->value.epoll.registrations[found];r->events=event->events;r->data=event->data;}
    } else if(operation==EPOLL_CTL_DEL){
        if(found<0)result=ENOENT;else {struct epoll_registration *r=&epoll.object->value.epoll.registrations[found];r->used=0;object_release(r->object);}
    } else result=EINVAL;
    __atomic_add_fetch(&epoll.object->sequence,1,__ATOMIC_RELEASE);wake64(&epoll.object->sequence,~0u);
    spin_unlock(&epoll.object->lock);
    struct descriptor_state *state=descriptor_state();__atomic_add_fetch(&state->sequence,1,__ATOMIC_RELEASE);wake64(&state->sequence,~0u);
    object_release(target.object);object_release(epoll.object);
    return result?set_error(result):0;
}
int epoll_wait(int epfd,struct epoll_event *events,int maximum,int timeout)
{
    if(!events||maximum<=0)return set_error(EINVAL);uint64_t deadline;
    if(!poll_deadline(timeout,&deadline))return set_error(EINVAL);
    struct descriptor_ref epoll={0};if(!descriptor_acquire(epfd,&epoll)||epoll.object->type!=DESCRIPTOR_EPOLL){
        if(epoll.object)object_release(epoll.object);return set_error(EBADF);}
    struct descriptor_state *state=descriptor_state();
    for(;;){int count=0;uint64_t sequence=__atomic_load_n(&state->sequence,__ATOMIC_ACQUIRE);
        spin_lock(&epoll.object->lock);
        for(unsigned i=0;i<EPOLL_REGS&&(count<maximum);++i){struct epoll_registration *r=&epoll.object->value.epoll.registrations[i];
            if(!r->used)continue;uint32_t ready=object_ready(r->object,r->access);
            ready=(ready&r->events)|(ready&(POLLERR|POLLHUP));
            if(ready)events[count++]=(struct epoll_event){ready,r->data};
        }
        spin_unlock(&epoll.object->lock);
        if(count||timeout==0){object_release(epoll.object);return count;}
        int rc=wait64(&state->sequence,sequence,deadline);
        if(rc==-ETIMEDOUT){object_release(epoll.object);return 0;}
        if(rc&&rc!=-EAGAIN){object_release(epoll.object);return set_error(-rc);}
        if(!descriptor_current(&epoll)){object_release(epoll.object);return set_error(EBADF);}
    }
}
int ioctl_FIONREAD(int fd,int *available)
{
    if(!available)return set_error(EFAULT);struct descriptor_ref reference={0};
    if(!descriptor_acquire(fd,&reference))return set_error(EBADF);
    int result=0;spin_lock(&reference.object->lock);
    if(reference.object->type==DESCRIPTOR_PIPE)*available=(int)reference.object->value.pipe.count;
    else if(reference.object->type==DESCRIPTOR_SOCKETPAIR)*available=(int)reference.object->value.socketpair.count[(reference.access>>2)&1];
    else if(reference.object->type==DESCRIPTOR_EVENT)*available=reference.object->value.event.counter?8:0;
    else if(reference.object->type==DESCRIPTOR_TCP){struct tcp_status_abi status;
        int64_t rc=call3(SYS_TCP_STATUS,reference.object->value.tcp.handle,(uintptr_t)&status,0);
        if(rc<0)result=(int)-rc;else *available=(int)status.available;}
    else result=EINVAL;
    spin_unlock(&reference.object->lock);object_release(reference.object);
    return result?set_error(result):0;
}

#define MUSL_AT_FDCWD (-100)
#define MUSL_O_CREAT 0100
#define MUSL_O_EXCL 0200
#define MUSL_O_TRUNC 01000
#define MUSL_O_ACCMODE 010000003
#define MUSL_O_PATH 010000000
#define MUSL_O_DIRECTORY 0200000
#define MUSL_F_DUPFD 0
#define MUSL_F_GETFD 1
#define MUSL_F_SETFD 2
#define MUSL_F_GETFL 3
#define MUSL_F_SETFL 4
#define MUSL_F_DUPFD_CLOEXEC 1030
#define MUSL_FD_CLOEXEC 1
#define MUSL_S_IFDIR 0040000
#define MUSL_S_IFREG 0100000
#define ENOTTY 25
#define MUSL_AT_REMOVEDIR 0x200

struct fs_entry_abi { char name[32];uint32_t type,reserved;uint64_t size; };
struct musl_stat_abi {
    int64_t dev,ino,nlink;uint32_t mode,uid,gid,pad;int64_t rdev,size,blksize,blocks;
    struct timespec atime,mtime,ctime;int64_t unused[3];
};
struct iovec_abi { void *base;size_t length; };
struct musl_dirent_abi { uint64_t ino;int64_t offset;uint16_t record_length;unsigned char type;char name[256]; };
struct runtime_directory {
    void *native;struct musl_dirent_abi *entry;
    char path[128];unsigned position;int descriptor;struct musl_dirent_abi storage;
};

static int runtime_path(const char *path,char result[128])
{
    if(!path)return set_error(EFAULT);if(!path[0])return set_error(ENOENT);
    size_t out=0,in=0;if(path[0]!='/')result[out++]='/';
    while(path[in]){if(out==127)return set_error(36);result[out++]=path[in++];}
    result[out]=0;return 0;
}
static int canonical_path(const char *path,char result[128])
{
    size_t out=1,in=path[0]=='/';result[0]='/';
    while(path[in]){
        while(path[in]=='/')++in;if(!path[in])break;
        size_t begin=in;while(path[in]&&path[in]!='/')++in;size_t length=in-begin;
        if(length==1&&path[begin]=='.')continue;
        if(length==2&&path[begin]=='.'&&path[begin+1]=='.'){
            if(out>1){while(out>1&&result[out-1]!='/')--out;if(out>1)--out;}continue;
        }
        if(out>1){if(out==127)return set_error(36);result[out++]='/';}
        if(length>127-out)return set_error(36);
        for(size_t i=0;i<length;++i)result[out++]=path[begin+i];
    }
    result[out]=0;return 0;
}
/* Directory names are immutable. Pin the open description while taking the
 * path snapshot; closing/reusing its descriptor cannot substitute a new base. */
static int runtime_path_at(int directory,const char *path,char result[128])
{
    if(!path)return set_error(EFAULT);if(!path[0])return set_error(ENOENT);
    if(path[0]=='/'||directory==MUSL_AT_FDCWD)return runtime_path(path,result);
    struct descriptor_ref reference={0};
    if(!descriptor_acquire(directory,&reference))return set_error(EBADF);
    if(reference.object->type!=DESCRIPTOR_DIRECTORY){
        object_release(reference.object);return set_error(20);
    }
    char joined[128];size_t used=0;
    while(reference.object->value.directory.path[used]){
        joined[used]=reference.object->value.directory.path[used];++used;
    }
    object_release(reference.object);
    if(used>1){if(used==127)return set_error(36);joined[used++]='/';}
    size_t relative_begin=used;
    for(size_t i=0;path[i];++i){if(used==127)return set_error(36);joined[used++]=path[i];}
    joined[used]=0;
    /* Check each traversed component BEFORE dot/dot-dot normalization: a
     * regular file or missing component cannot be escaped using /.. . */
    for(size_t i=relative_begin;i<used;++i)if(joined[i]=='/'&&i>relative_begin){
        char prefix[128];joined[i]=0;int error=canonical_path(joined,prefix);joined[i]='/';
        if(error)return -1;struct fs_entry_abi entry;
        int64_t status=call3(SYS_STAT,(uintptr_t)prefix,(uintptr_t)&entry,0);
        if(status<0)return fail(status);if(entry.type!=2)return set_error(20);
    }
    return canonical_path(joined,result);
}
static void stat_convert(const struct fs_entry_abi *source,struct musl_stat_abi *target)
{
    for(size_t i=0;i<sizeof(*target);++i)((unsigned char *)target)[i]=0;
    target->dev=1;target->ino=1;target->nlink=1;
    target->mode=(source->type==2?MUSL_S_IFDIR|0777:MUSL_S_IFREG|(source->reserved&1?0444:0666));
    target->size=(int64_t)source->size;target->blksize=4096;
    target->blocks=(int64_t)((source->size+511)/512);
}
int openat(int directory,const char *path,int flags,...)
{
    int access_mode=flags&MUSL_O_ACCMODE;if(access_mode==MUSL_O_PATH)return set_error(ENOTSUP);
    unsigned access=access_mode==0?1:access_mode==1?2:access_mode==2?3:0;
    int allowed=3|MUSL_O_CREAT|MUSL_O_EXCL|MUSL_O_TRUNC|O_APPEND|O_NONBLOCK|O_CLOEXEC|MUSL_O_DIRECTORY;
    if(!access||(flags&~allowed)||((flags&MUSL_O_TRUNC)&&!(access&2)))return set_error(EINVAL);
    char normalized[128];if(runtime_path_at(directory,path,normalized))return -1;
    struct fs_entry_abi entry;int64_t status=call3(SYS_STAT,(uintptr_t)normalized,(uintptr_t)&entry,0);
    if(!status&&entry.type==2){
        if((flags&(MUSL_O_CREAT|MUSL_O_EXCL))==(MUSL_O_CREAT|MUSL_O_EXCL))return set_error(EEXIST);
        if((access&2)||(flags&MUSL_O_TRUNC))return set_error(21);
        struct descriptor_state *state=descriptor_state();struct descriptor_object *object=descriptor_object(DESCRIPTOR_DIRECTORY);
        if(!state||!object){if(object)free(object);return set_error(ENOMEM);}
        size_t i=0;for(;normalized[i]&&i<127;++i)object->value.directory.path[i]=normalized[i];
        object->value.directory.path[i]=0;spin_lock(&state->lock);
        int fd=descriptor_install_locked(state,object,1,(unsigned)flags);spin_unlock(&state->lock);
        if(fd<0){free(object);return set_error(ENOSPC);}return fd;
    }
    if(!status&&(flags&MUSL_O_DIRECTORY))return set_error(20);
    if(status<0&&status!=-ENOENT)return fail(status);
    if((flags&MUSL_O_DIRECTORY)&&status==-ENOENT)return fail(status);
    unsigned myos=access;if(flags&MUSL_O_CREAT)myos|=4;if(flags&MUSL_O_TRUNC)myos|=8;if(flags&MUSL_O_EXCL)myos|=16;
    int64_t kernel=call3(SYS_FOPEN,(uintptr_t)normalized,myos,0);if(kernel<0)return fail(kernel);
    struct descriptor_state *state=descriptor_state();struct descriptor_object *object=descriptor_object(DESCRIPTOR_FILE);
    if(!state||!object){(void)call3(SYS_FCLOSE,(unsigned)kernel,0,0);if(object)free(object);return set_error(ENOMEM);}
    object->value.file.kernel_fd=(int32_t)kernel;spin_lock(&state->lock);
    int fd=descriptor_install_locked(state,object,access,(unsigned)flags);spin_unlock(&state->lock);
    if(fd<0){(void)call3(SYS_FCLOSE,(unsigned)kernel,0,0);free(object);return set_error(ENOSPC);}return fd;
}
int dup(int fd)
{
    struct descriptor_ref reference={0};if(!descriptor_acquire(fd,&reference))return set_error(EBADF);
    struct descriptor_state *state=descriptor_state();spin_lock(&state->lock);
    int result=descriptor_install_locked(state,reference.object,reference.access,reference.flags&~O_CLOEXEC);
    if(result>=0){spin_lock(&reference.object->lock);
        if(reference.object->type==DESCRIPTOR_EVENT)++reference.object->value.event.opens;
        if(reference.object->type==DESCRIPTOR_PIPE){
            if(reference.access&1)++reference.object->value.pipe.readers;
            if(reference.access&2)++reference.object->value.pipe.writers;
        }
        if(reference.object->type==DESCRIPTOR_SOCKETPAIR){unsigned side=(reference.access>>2)&1;
            ++reference.object->value.socketpair.readers[side];++reference.object->value.socketpair.writers[side];}
        if(reference.object->type==DESCRIPTOR_TCP)++reference.object->value.tcp.opens;
        spin_unlock(&reference.object->lock);
    }
    spin_unlock(&state->lock);object_release(reference.object);return result<0?set_error(ENOSPC):result;
}
int fcntl(int fd,int command,...)
{
    long argument=0;if(command==MUSL_F_DUPFD||command==MUSL_F_DUPFD_CLOEXEC||
       command==MUSL_F_SETFD||command==MUSL_F_SETFL){va_list arguments;
        va_start(arguments,command);argument=va_arg(arguments,long);va_end(arguments);}
    if(command==MUSL_F_DUPFD||command==MUSL_F_DUPFD_CLOEXEC){
        if(argument<0||argument>=FD_BASE+(int)FD_SLOTS)return set_error(EINVAL);
        unsigned first=argument<=FD_BASE?0:(unsigned)(argument-FD_BASE);
        struct descriptor_ref reference={0};if(!descriptor_acquire(fd,&reference))return set_error(EBADF);
        struct descriptor_state *state=descriptor_state();spin_lock(&state->lock);
        unsigned flags=reference.flags&~O_CLOEXEC;
        if(command==MUSL_F_DUPFD_CLOEXEC)flags|=O_CLOEXEC;
        int copy=descriptor_install_from_locked(state,reference.object,reference.access,flags,first);
        if(copy>=0){spin_lock(&reference.object->lock);
            if(reference.object->type==DESCRIPTOR_EVENT)++reference.object->value.event.opens;
            if(reference.object->type==DESCRIPTOR_PIPE){
                if(reference.access&1)++reference.object->value.pipe.readers;
                if(reference.access&2)++reference.object->value.pipe.writers;
            }
            if(reference.object->type==DESCRIPTOR_SOCKETPAIR){unsigned side=(reference.access>>2)&1;
                ++reference.object->value.socketpair.readers[side];++reference.object->value.socketpair.writers[side];}
            if(reference.object->type==DESCRIPTOR_TCP)++reference.object->value.tcp.opens;
            spin_unlock(&reference.object->lock);
        }
        spin_unlock(&state->lock);object_release(reference.object);
        return copy<0?set_error(ENOSPC):copy;
    }
    struct descriptor_ref reference={0};if(!descriptor_acquire(fd,&reference))return set_error(EBADF);
    int result=0;
    if(command==MUSL_F_GETFD)result=(reference.flags&O_CLOEXEC)?MUSL_FD_CLOEXEC:0;
    else if(command==MUSL_F_GETFL)result=(reference.access==1?0:reference.access==2?1:2)|(reference.flags&(O_APPEND|O_NONBLOCK));
    else if(command==MUSL_F_SETFD||command==MUSL_F_SETFL){struct descriptor_state *s=descriptor_state();spin_lock(&s->lock);
        struct descriptor_slot *slot=&s->slots[reference.slot];if(slot->object!=reference.object||slot->generation!=reference.generation)result=-EBADF;
        else if(command==MUSL_F_SETFD){if(argument&~MUSL_FD_CLOEXEC)result=-EINVAL;else slot->flags=(slot->flags&~O_CLOEXEC)|(argument?O_CLOEXEC:0);}
        else {if(argument&~(3|O_APPEND|O_NONBLOCK))result=-EINVAL;
            else if(reference.object->type==DESCRIPTOR_SOCKETPAIR){
                if(argument&O_APPEND)result=-EINVAL;
                else __atomic_store_n(&reference.object->value.socketpair.status_flags[(reference.access>>2)&1],(unsigned)argument&O_NONBLOCK,__ATOMIC_RELEASE);
            } else if(reference.object->type==DESCRIPTOR_TCP&&(argument&O_APPEND))result=-EINVAL;
            else __atomic_store_n(&reference.object->status_flags,(unsigned)argument&(O_APPEND|O_NONBLOCK),__ATOMIC_RELEASE);}
        spin_unlock(&s->lock);
    } else result=-EINVAL;
    object_release(reference.object);return result<0?set_error(-result):result;
}
int64_t lseek(int fd,int64_t offset,int whence)
{
    struct descriptor_ref reference={0};if(!descriptor_acquire(fd,&reference)||reference.object->type!=DESCRIPTOR_FILE){if(reference.object)object_release(reference.object);set_error(EBADF);return -1;}
    spin_lock(&reference.object->lock);int64_t result=call3(SYS_FSEEK,(unsigned)reference.object->value.file.kernel_fd,(uint64_t)offset,(unsigned)whence);spin_unlock(&reference.object->lock);
    object_release(reference.object);if(result<0){fail(result);return -1;}return result;
}
static int64_t positioned_io(int fd,void *buffer,size_t length,int64_t offset,int writing)
{
    if(offset<0){set_error(EINVAL);return -1;}struct descriptor_ref reference={0};
    if(!descriptor_acquire(fd,&reference)||reference.object->type!=DESCRIPTOR_FILE||!(reference.access&(writing?2:1))){if(reference.object)object_release(reference.object);set_error(EBADF);return -1;}
    spin_lock(&reference.object->lock);int kernel=reference.object->value.file.kernel_fd;
    int64_t saved=call3(SYS_FSEEK,(unsigned)kernel,0,1),result=saved;
    if(saved>=0){result=call3(SYS_FSEEK,(unsigned)kernel,(uint64_t)offset,0);
        if(result>=0)result=call4(writing?SYS_FWRITE:SYS_FREAD,(unsigned)kernel,(uintptr_t)buffer,length,0);}
    if(saved>=0)(void)call3(SYS_FSEEK,(unsigned)kernel,(uint64_t)saved,0);
    spin_unlock(&reference.object->lock);object_release(reference.object);if(result<0){fail(result);return -1;}return result;
}
int64_t pread(int fd,void *buffer,size_t length,int64_t offset){return positioned_io(fd,buffer,length,offset,0);}
int64_t pwrite(int fd,const void *buffer,size_t length,int64_t offset){return positioned_io(fd,(void *)buffer,length,offset,1);}
static __attribute__((noinline)) int64_t vector_io(int fd,const struct iovec_abi *vectors,int count,int output)
{if((!vectors&&count)||count<0||count>1024){set_error(EINVAL);return -1;}int64_t total=0;
 for(int i=0;i<count;++i){if(vectors[i].length>(size_t)(INT64_MAX-total)){set_error(EINVAL);return total?total:-1;}
  int64_t got=output?write(fd,vectors[i].base,vectors[i].length):read(fd,vectors[i].base,vectors[i].length);
  if(got<0)return total?total:-1;total+=got;if((size_t)got<vectors[i].length)break;}return total;}
int64_t readv(int fd,const struct iovec_abi *vectors,int count)
{return vector_io(fd,vectors,count,0);}
int64_t writev(int fd,const struct iovec_abi *vectors,int count)
{return vector_io(fd,vectors,count,1);}
int ftruncate(int fd,int64_t length)
{if(length<0)return set_error(EINVAL);struct descriptor_ref r={0};if(!descriptor_acquire(fd,&r)||r.object->type!=DESCRIPTOR_FILE){if(r.object)object_release(r.object);return set_error(EBADF);}
 spin_lock(&r.object->lock);int64_t rc=call3(SYS_FTRUNCATE,(unsigned)r.object->value.file.kernel_fd,(uint64_t)length,0);spin_unlock(&r.object->lock);object_release(r.object);return rc<0?fail(rc):0;}
int fsync(int fd){struct descriptor_ref r={0};if(!descriptor_acquire(fd,&r)||r.object->type!=DESCRIPTOR_FILE){if(r.object)object_release(r.object);return set_error(EBADF);}
 int64_t rc=call3(SYS_FSYNC,(unsigned)r.object->value.file.kernel_fd,0,0);object_release(r.object);return rc<0?fail(rc):0;}
int fdatasync(int fd){return fsync(fd);}
int fstat(int fd,struct musl_stat_abi *information)
{if(!information)return set_error(EFAULT);struct descriptor_ref r={0};if(!descriptor_acquire(fd,&r)){return set_error(EBADF);}
 struct fs_entry_abi entry;int64_t rc;
 if(r.object->type==DESCRIPTOR_FILE)rc=call3(SYS_FSTAT,(unsigned)r.object->value.file.kernel_fd,(uintptr_t)&entry,0);
 else if(r.object->type==DESCRIPTOR_DIRECTORY)rc=call3(SYS_STAT,(uintptr_t)r.object->value.directory.path,(uintptr_t)&entry,0);
 else {object_release(r.object);return set_error(EBADF);}
 object_release(r.object);if(rc<0)return fail(rc);stat_convert(&entry,information);return 0;}
int fstatat(int directory,const char *path,struct musl_stat_abi *information,int flags)
{if(flags)return set_error(EINVAL);if(!information)return set_error(EFAULT);char normalized[128];if(runtime_path_at(directory,path,normalized))return -1;
 struct fs_entry_abi entry;int64_t rc=call3(SYS_STAT,(uintptr_t)normalized,(uintptr_t)&entry,0);if(rc<0)return fail(rc);stat_convert(&entry,information);return 0;}
int access(const char *path,int mode)
{if(mode&~7)return set_error(EINVAL);struct musl_stat_abi info;if(fstatat(MUSL_AT_FDCWD,path,&info,0))return -1;
 if((mode&2)&&!(info.mode&0222))return set_error(30);
 if((mode&1)&&!(info.mode&0111))return set_error(13);return 0;}
char *getcwd(char *buffer,size_t size)
{if(!buffer){set_error(EFAULT);return 0;}if(size<2){set_error(34);return 0;}buffer[0]='/';buffer[1]=0;return buffer;}
int mkdir(const char *path,uint32_t mode)
{(void)mode;char normalized[128];if(runtime_path(path,normalized))return -1;int64_t rc=call3(SYS_MKDIR,(uintptr_t)normalized,0,0);return rc<0?fail(rc):0;}
int isatty(int fd){if(fd==1||fd==2)return 1;struct descriptor_ref r={0};
 if(!descriptor_acquire(fd,&r)){set_error(EBADF);return 0;}object_release(r.object);set_error(ENOTTY);return 0;}
long pathconf(const char *path,int name)
{struct musl_stat_abi info;if(fstatat(MUSL_AT_FDCWD,path,&info,0))return -1;
 if(name==3)return 31;if(name==4)return 127;if(name==5)return 4096;return set_error(EINVAL);}
void *fdopendir(int fd)
{
    struct descriptor_ref r={0};if(!descriptor_acquire(fd,&r)||r.object->type!=DESCRIPTOR_DIRECTORY){
        int error=r.object?20:EBADF;if(r.object)object_release(r.object);set_error(error);return 0;}
    struct runtime_directory *directory=malloc(sizeof(*directory));
    if(!directory){object_release(r.object);set_error(ENOMEM);return 0;}
    *directory=(struct runtime_directory){directory,&directory->storage,{0},0,fd,{0}};
    size_t i=0;for(;i<127&&r.object->value.directory.path[i];++i)
        directory->path[i]=r.object->value.directory.path[i];
    directory->path[i]=0;object_release(r.object);
    return directory;
}
void *opendir(const char *path)
{int fd=openat(MUSL_AT_FDCWD,path,MUSL_O_DIRECTORY|O_CLOEXEC,0);if(fd<0)return 0;
 void *directory=fdopendir(fd);if(!directory)(void)close(fd);return directory;}
struct musl_dirent_abi *readdir(void *raw)
{
    struct runtime_directory *directory=raw;
    if(!directory||directory->native!=directory){set_error(EINVAL);return 0;}
    struct system_asset_entry source={0};int64_t rc;
    const char *path=directory->path;
    if(path[0]=='/'&&path[1]=='a'&&path[2]=='s'&&path[3]=='s'&&path[4]=='e'&&
       path[5]=='t'&&path[6]=='s'&&(!path[7]||path[7]=='/')){
        struct system_asset_entry_request request={(uintptr_t)path,directory->position,(uintptr_t)&source};
        rc=call3(MYOS_SYSTEM_CALL,SYSTEM_ASSET_ENTRY,(uintptr_t)&request,0);
    }else{
        struct fs_entry_abi small;rc=call3(SYS_READDIR,(uintptr_t)path,(uintptr_t)&small,directory->position);
        if(rc==1){source.type=small.type;source.size=small.size;
            for(unsigned i=0;i<31&&small.name[i];++i)source.name[i]=small.name[i];}
    }
    if(rc<0){fail(rc);return 0;}if(!rc)return 0;
    struct musl_dirent_abi *entry=&directory->storage;
    for(size_t i=0;i<sizeof(*entry);++i)((unsigned char *)entry)[i]=0;
    entry->ino=directory->position+1;entry->offset=(int64_t)directory->position+1;
    entry->record_length=sizeof(*entry);entry->type=source.type==2?4:8;
    size_t i=0;for(;i<239&&source.name[i];++i)entry->name[i]=source.name[i];
    entry->name[i]=0;++directory->position;return entry;
}
int readdir_r(void *raw,struct musl_dirent_abi *entry,struct musl_dirent_abi **result)
{
    if(!raw||!entry||!result)return EINVAL;*result=0;*__errno_location()=0;
    struct musl_dirent_abi *found=readdir(raw);if(!found)return *__errno_location();
    *entry=*found;*result=entry;return 0;
}
int closedir(void *raw)
{struct runtime_directory *directory=raw;if(!directory||directory->native!=directory)return set_error(EINVAL);
 int rc=close(directory->descriptor);directory->native=0;free(directory);return rc;}
int unlinkat(int directory,const char *path,int flags)
{
    if(directory!=MUSL_AT_FDCWD)return set_error(EBADF);
    if(flags&~MUSL_AT_REMOVEDIR)return set_error(EINVAL);
    char normalized[128];if(runtime_path(path,normalized))return -1;
    struct fs_entry_abi entry;int64_t rc=call3(SYS_STAT,(uintptr_t)normalized,(uintptr_t)&entry,0);
    if(rc<0)return fail(rc);
    if((flags&MUSL_AT_REMOVEDIR)&&entry.type!=2)return set_error(20);
    if(!(flags&MUSL_AT_REMOVEDIR)&&entry.type==2)return set_error(21);
    rc=call3(SYS_UNLINK,(uintptr_t)normalized,0,0);return rc<0?fail(rc):0;
}

/* Starboard storage records use two independently validated file generations.
 * fsync commits the RAMFS snapshot atomically on the persistent v2 disk. */
#define SB_STORAGE_MAGIC UINT64_C(0x4d594f5353544f52)
#define SB_STORAGE_MAX 65504
struct sb_storage_header { uint64_t magic,generation,size;uint32_t checksum,flags; };
struct SbStorageRecordPrivate {
    int fd;
    unsigned slot,tombstone;
    uint64_t generation;
    int64_t size;
    char name[32];
};
static struct SbStorageRecordPrivate **sb_storage_open_location(void)
{return (struct SbStorageRecordPrivate **)(uintptr_t)(RUNTIME_TLS_BASE+1600);}

static uint32_t sb_storage_hash(const unsigned char *data,size_t size)
{
    uint32_t hash=2166136261u;
    for(size_t i=0;i<size;++i)hash=(hash^data[i])*16777619u;
    return hash;
}
static int sb_storage_name(const char *input,char output[32])
{
    if(!input)input="default";
    size_t n=0;
    while(input[n]){
        unsigned char c=(unsigned char)input[n];
        if(n>=31||!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||
                    (c>='0'&&c<='9')||c=='_'||c=='-'||c=='.'))return 0;
        output[n]=(char)c;++n;
    }
    if(!n)return 0;output[n]=0;return 1;
}
static int sb_storage_path(const char *name,unsigned slot,char path[128])
{
    static const char prefix[]="/home/.sb_";
    size_t i=0;for(;prefix[i];++i)path[i]=prefix[i];
    for(size_t j=0;name[j];++j)path[i++]=name[j];
    path[i++]='_';path[i++]=(char)('0'+slot);path[i]=0;return 1;
}
static int sb_storage_read_header(const char *name,unsigned slot,
                                  struct sb_storage_header *header,int *fd_out)
{
    char path[128];sb_storage_path(name,slot,path);
    struct musl_stat_abi info;
    if(fstatat(MUSL_AT_FDCWD,path,&info,0))return 0;
    if(info.size<(int64_t)sizeof(*header)||info.size>(int64_t)(sizeof(*header)+SB_STORAGE_MAX))return 0;
    int fd=openat(MUSL_AT_FDCWD,path,2|O_CLOEXEC,0);
    if(fd<0)return 0;
    if(pread(fd,header,sizeof(*header),0)!=(int64_t)sizeof(*header)||
       header->magic!=SB_STORAGE_MAGIC||!header->generation||header->flags>1||
       header->size>SB_STORAGE_MAX||info.size!=(int64_t)(sizeof(*header)+header->size)||
       (header->flags&&header->size)||
       (header->flags?header->checksum!=sb_storage_hash((const unsigned char *)"",0):0)){
        close(fd);return 0;
    }
    uint32_t hash=2166136261u;unsigned char bytes[512];uint64_t offset=0;
    while(offset<header->size){size_t take=(size_t)(header->size-offset);if(take>sizeof(bytes))take=sizeof(bytes);
        int64_t got=pread(fd,bytes,take,(int64_t)sizeof(*header)+(int64_t)offset);
        if(got!=(int64_t)take){close(fd);return 0;}
        for(size_t i=0;i<take;++i)hash=(hash^bytes[i])*16777619u;offset+=take;
    }
    if(hash!=header->checksum){close(fd);return 0;}
    *fd_out=fd;return 1;
}
static int sb_storage_commit(const char *name,unsigned slot,uint64_t generation,
                             const void *data,size_t size,int tombstone,int *fd_out)
{
    struct sb_storage_header header={SB_STORAGE_MAGIC,generation,size,
        sb_storage_hash((const unsigned char *)data,size),(uint32_t)tombstone};
    char path[128];sb_storage_path(name,slot,path);
    int fd=openat(MUSL_AT_FDCWD,path,MUSL_O_CREAT|MUSL_O_TRUNC|2|O_CLOEXEC,0600);
    if(fd<0)return 0;
    int ok=write(fd,&header,sizeof(header))==(int64_t)sizeof(header);
    size_t offset=0;
    while(ok&&offset<size){int64_t put=write(fd,(const unsigned char *)data+offset,size-offset);
        if(put<=0)ok=0;else offset+=(size_t)put;}
    if(ok&&fsync(fd))ok=0;
    if(!ok){close(fd);(void)unlinkat(MUSL_AT_FDCWD,path,0);return 0;}
    *fd_out=fd;return 1;
}
void *SbStorageOpenRecord(const char *name)
{
    if(*sb_storage_open_location()){set_error(16);return 0;}
    char normalized[32];if(!sb_storage_name(name,normalized)){set_error(EINVAL);return 0;}
    struct sb_storage_header headers[2];int fds[2]={-1,-1};
    int valid[2]={sb_storage_read_header(normalized,0,&headers[0],&fds[0]),
                  sb_storage_read_header(normalized,1,&headers[1],&fds[1])};
    int selected=-1;
    if(valid[0])selected=0;
    if(valid[1]&&(selected<0||headers[1].generation>headers[selected].generation))selected=1;
    if(selected>=0&&fds[1-selected]>=0)close(fds[1-selected]);
    struct SbStorageRecordPrivate *record=malloc(sizeof(*record));
    if(!record){if(fds[0]>=0)close(fds[0]);if(fds[1]>=0)close(fds[1]);set_error(12);return 0;}
    record->fd=selected<0?-1:fds[selected];record->slot=selected<0?1:(unsigned)selected;
    record->generation=selected<0?0:headers[selected].generation;
    record->tombstone=selected<0?0:headers[selected].flags;
    record->size=selected<0||record->tombstone?0:(int64_t)headers[selected].size;
    for(size_t i=0;i<sizeof(record->name);++i)record->name[i]=normalized[i];
    *sb_storage_open_location()=record;return record;
}
bool SbStorageCloseRecord(void *raw)
{
    struct SbStorageRecordPrivate *record=raw;
    if(!record||record!=*sb_storage_open_location()){set_error(EINVAL);return false;}
    int result=record->fd<0?0:close(record->fd);*sb_storage_open_location()=0;free(record);
    return result==0;
}
int64_t SbStorageGetRecordSize(void *raw)
{
    struct SbStorageRecordPrivate *record=raw;
    if(!record||record!=*sb_storage_open_location()){set_error(EINVAL);return -1;}return record->size;
}
int64_t SbStorageReadRecord(void *raw,char *output,int64_t size)
{
    struct SbStorageRecordPrivate *record=raw;
    if(!record||record!=*sb_storage_open_location()){set_error(EINVAL);return -1;}
    if(size<0||(!output&&size)){set_error(EINVAL);return -1;}
    if(!size||!record->size||record->fd<0)return 0;
    size_t count=(uint64_t)size<(uint64_t)record->size?(size_t)size:(size_t)record->size;
    return pread(record->fd,output,count,sizeof(struct sb_storage_header));
}
bool SbStorageWriteRecord(void *raw,const char *data,int64_t size)
{
    struct SbStorageRecordPrivate *record=raw;
    if(!record||record!=*sb_storage_open_location()){set_error(EINVAL);return false;}
    if(size<0||size>SB_STORAGE_MAX||(!data&&size)){set_error(EINVAL);return false;}
    if(record->generation==UINT64_MAX){set_error(EOVERFLOW);return false;}
    unsigned slot=1-record->slot;int fd=-1;
    if(!sb_storage_commit(record->name,slot,record->generation+1,data,(size_t)size,0,&fd))return false;
    if(record->fd>=0)close(record->fd);record->fd=fd;record->slot=slot;
    ++record->generation;record->size=size;record->tombstone=0;return true;
}
bool SbStorageDeleteRecord(const char *name)
{
    char normalized[32];if(!sb_storage_name(name,normalized)){set_error(EINVAL);return false;}
    struct SbStorageRecordPrivate *opened=*sb_storage_open_location();
    if(opened){size_t i=0;while(i<sizeof(opened->name)&&normalized[i]&&normalized[i]==opened->name[i])++i;
        if(i==sizeof(opened->name)||(!normalized[i]&&!opened->name[i])){set_error(16);return false;}}
    struct sb_storage_header headers[2];int fds[2]={-1,-1};
    int valid[2]={sb_storage_read_header(normalized,0,&headers[0],&fds[0]),
                  sb_storage_read_header(normalized,1,&headers[1],&fds[1])};
    int selected=-1;if(valid[0])selected=0;
    if(valid[1]&&(selected<0||headers[1].generation>headers[selected].generation))selected=1;
    if(selected<0||headers[selected].flags){if(fds[0]>=0)close(fds[0]);if(fds[1]>=0)close(fds[1]);return false;}
    uint64_t generation=headers[selected].generation;
    if(generation==UINT64_MAX){close(fds[selected]);if(fds[1-selected]>=0)close(fds[1-selected]);set_error(EOVERFLOW);return false;}
    unsigned target=1-(unsigned)selected;int replacement=-1;
    int ok=sb_storage_commit(normalized,target,generation+1,0,0,1,&replacement);
    close(fds[selected]);if(fds[1-selected]>=0)close(fds[1-selected]);
    if(replacement>=0)close(replacement);return ok!=0;
}
char *realpath(const char *path,char *resolved)
{
    char normalized[128];if(runtime_path(path,normalized))return 0;
    struct fs_entry_abi entry;int64_t rc=call3(SYS_STAT,(uintptr_t)normalized,(uintptr_t)&entry,0);
    if(rc<0){fail(rc);return 0;}char canonical[128];if(canonical_path(normalized,canonical))return 0;
    char *output=resolved?resolved:malloc(128);
    if(!output){set_error(ENOMEM);return 0;}size_t i=0;do{output[i]=canonical[i];}while(canonical[i++]);return output;
}
static volatile uint64_t *temporary_counter(void)
{return (volatile uint64_t *)(uintptr_t)(RUNTIME_TLS_BASE+1544);}
static int temporary_name(char *template)
{
    if(!template)return set_error(EFAULT);size_t length=0;while(template[length])++length;
    if(length<6)return set_error(EINVAL);for(size_t i=length-6;i<length;++i)if(template[i]!='X')return set_error(EINVAL);
    static const char alphabet[]="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    uint64_t value=__atomic_add_fetch(temporary_counter(),1,__ATOMIC_RELAXED)^
                   ((uint64_t)call3(2,0,0,0)<<32)^ticks();
    for(unsigned i=0;i<6;++i){template[length-1-i]=alphabet[value%62];value/=62;}return 0;
}
int mkstemp(char *template)
{
    for(unsigned attempt=0;attempt<100;++attempt){if(temporary_name(template))return -1;
        int fd=openat(MUSL_AT_FDCWD,template,MUSL_O_CREAT|MUSL_O_EXCL|2,0600);
        if(fd>=0)return fd;if(*__errno_location()!=EEXIST)return -1;
        size_t length=0;while(template[length])++length;for(size_t i=length-6;i<length;++i)template[i]='X';
    }
    return set_error(EEXIST);
}
char *mkdtemp(char *template)
{
    for(unsigned attempt=0;attempt<100;++attempt){if(temporary_name(template))return 0;
        if(!mkdir(template,0700))return template;if(*__errno_location()!=EEXIST)return 0;
        size_t length=0;while(template[length])++length;for(size_t i=length-6;i<length;++i)template[i]='X';
    }
    set_error(EEXIST);return 0;
}

/* Pinned Cobalt musl system ABI.  Report only capabilities MyOS actually owns. */
#define MUSL_SC_CLK_TCK 2
#define MUSL_SC_OPEN_MAX 4
#define MUSL_SC_VERSION 29
#define MUSL_SC_PAGESIZE 30
#define MUSL_SC_THREADS 67
#define MUSL_SC_THREAD_DESTRUCTOR_ITERATIONS 73
#define MUSL_SC_THREAD_KEYS_MAX 74
#define MUSL_SC_THREAD_STACK_MIN 75
#define MUSL_SC_THREAD_THREADS_MAX 76
#define MUSL_SC_NPROCESSORS_CONF 83
#define MUSL_SC_NPROCESSORS_ONLN 84
#define MUSL_SC_PHYS_PAGES 85
#define MUSL_SC_AVPHYS_PAGES 86
#define MUSL_SC_MONOTONIC_CLOCK 149
#define MUSL_SC_READER_WRITER_LOCKS 153
#define MUSL_SC_TIMEOUTS 164
#define MUSL_AT_PAGESZ 6
#define MUSL_AT_UID 11
#define MUSL_AT_EUID 12
#define MUSL_AT_GID 13
#define MUSL_AT_EGID 14
#define MUSL_AT_CLKTCK 17
#define MUSL_AT_SECURE 23
#define MUSL_RLIMIT_STACK 3
#define MUSL_RLIMIT_NPROC 6
#define MUSL_RLIMIT_NOFILE 7
#define MUSL_PRIO_PROCESS 0
#define MUSL_PR_SET_NAME 15
#define MUSL_PR_GET_NAME 16

struct musl_rlimit { uint64_t current,maximum; };
struct musl_cpu_set { uint64_t bits[16]; };
struct musl_utsname { char sysname[257],nodename[257],release[257],version[257],machine[257],domainname[257]; };
const int32_t kSbFileMaxName=31;
const uint32_t kSbFileMaxPath=127;
static const char file_separator[]="/";
const char *const kSbFileSepString=file_separator;
const uint32_t kSbMaxThreads=8;

static void string_copy(char *destination,const char *source,size_t capacity)
{
    if(!capacity)return;size_t i=0;for(;i+1<capacity&&source[i];++i)destination[i]=source[i];
    destination[i]=0;
}
int32_t getpid(void){return (int32_t)call3(2,0,0,0);}
int32_t gettid(void){return (int32_t)call3(SYS_GETTID,0,0,0);}
uint32_t getuid(void){return 0;}
uint32_t geteuid(void){return 0;}
long sysconf(int name)
{
    switch(name){
    case MUSL_SC_CLK_TCK:return 100;
    case MUSL_SC_OPEN_MAX:return 40;
    case MUSL_SC_VERSION:return 200809;
    case MUSL_SC_PAGESIZE:return PAGE_SIZE;
    case MUSL_SC_THREADS:case MUSL_SC_MONOTONIC_CLOCK:
    case MUSL_SC_READER_WRITER_LOCKS:case MUSL_SC_TIMEOUTS:return 200809;
    case MUSL_SC_THREAD_DESTRUCTOR_ITERATIONS:return 4;
    case MUSL_SC_THREAD_KEYS_MAX:return THREAD_KEYS;
    case MUSL_SC_THREAD_STACK_MIN:return 16384;
    case MUSL_SC_THREAD_THREADS_MAX:return 8;
    case MUSL_SC_NPROCESSORS_CONF:case MUSL_SC_NPROCESSORS_ONLN:return 1;
    /* Only boot-map usable RAM managed by MyOS (currently below256MiB).
     * Available pages are an instantaneous unreserved PMM snapshot. */
    case MUSL_SC_PHYS_PAGES:return (long)call3(50,0,0,0);
    case MUSL_SC_AVPHYS_PAGES:return (long)call3(50,1,0,0);
    default:return set_error(EINVAL);
    }
}
unsigned long getauxval(unsigned long type)
{
    switch(type){
    case MUSL_AT_PAGESZ:return PAGE_SIZE;
    case MUSL_AT_UID:case MUSL_AT_EUID:case MUSL_AT_GID:case MUSL_AT_EGID:
    case MUSL_AT_SECURE:return 0;
    case MUSL_AT_CLKTCK:return 100;
    default:*__errno_location()=ENOENT;return 0;
    }
}
int getrlimit(int resource,struct musl_rlimit *limit)
{
    if(!limit)return set_error(EFAULT);
    if(resource==MUSL_RLIMIT_NOFILE)*limit=(struct musl_rlimit){40,40};
    else if(resource==MUSL_RLIMIT_NPROC)*limit=(struct musl_rlimit){8,8};
    else if(resource==MUSL_RLIMIT_STACK){struct main_stack_info info;
        int64_t rc=call3(SYS_MAIN_STACK_INFO,(uintptr_t)&info,0,0);if(rc<0)return set_error((int)-rc);
        *limit=(struct musl_rlimit){info.length,info.length};}
    else return set_error(EINVAL);
    return 0;
}
int sched_getaffinity(int32_t pid,size_t size,struct musl_cpu_set *mask)
{
    if(!mask)return set_error(EFAULT);
    if(pid!=0&&pid!=getpid())return set_error(ESRCH);
    if(size<sizeof(*mask))return set_error(EINVAL);
    for(unsigned i=0;i<16;++i)mask->bits[i]=0;mask->bits[0]=1;return 0;
}
int sched_setscheduler(int32_t pid,int policy,const int *parameter)
{
    if(!parameter)return set_error(EFAULT);
    if(pid!=0&&pid!=getpid())return set_error(ESRCH);
    if(policy||*parameter)return set_error(EINVAL);
    return 0;
}
int sched_yield(void){yield_cpu();return 0;}
int setpriority(int which,uint32_t who,int priority)
{
    if(which!=MUSL_PRIO_PROCESS||(who&&who!=(uint32_t)getpid()))return set_error(EINVAL);
    if(priority)return set_error(EPERM);return 0;
}
int prctl(int operation,...)
{
    va_list arguments;va_start(arguments,operation);void *argument=va_arg(arguments,void *);va_end(arguments);
    if(operation==MUSL_PR_SET_NAME){if(!argument)return set_error(EFAULT);
        char name[16];string_copy(name,argument,sizeof(name));
        int rc=pthread_setname_np(pthread_self(),name);if(rc)return set_error(rc);return 0;}
    if(operation==MUSL_PR_GET_NAME){if(!argument)return set_error(EFAULT);
        int rc=pthread_getname_np(pthread_self(),argument,16);if(rc)return set_error(rc);return 0;}
    return set_error(EINVAL);
}
int uname(struct musl_utsname *name)
{
    if(!name)return set_error(EFAULT);
    for(size_t i=0;i<sizeof(*name);++i)((unsigned char *)name)[i]=0;
    string_copy(name->sysname,"MyOS",257);string_copy(name->nodename,"myos",257);
    string_copy(name->release,"0.33",257);string_copy(name->version,"Phase 33.1",257);
    string_copy(name->machine,"x86_64",257);string_copy(name->domainname,"local",257);return 0;
}

/* Optional Starboard features are advertised as unavailable, never as dummy
 * successful devices or DRM sessions. */
void *SbDrmCreateSystem(const char *key,void *context,void *request,void *updated,
                        void *statuses,void *certificate,void *closed)
{(void)key;(void)context;(void)request;(void)updated;(void)statuses;(void)certificate;(void)closed;return 0;}
void SbDrmGenerateSessionUpdateRequest(void *system,int ticket,const char *type,const void *data,int size)
{(void)system;(void)ticket;(void)type;(void)data;(void)size;}
void SbDrmUpdateSession(void *system,int ticket,const void *key,int key_size,const void *session,int session_size)
{(void)system;(void)ticket;(void)key;(void)key_size;(void)session;(void)session_size;}
void SbDrmCloseSession(void *system,const void *session,int size)
{(void)system;(void)session;(void)size;}
bool SbDrmIsServerCertificateUpdatable(void *system){(void)system;return false;}
void SbDrmUpdateServerCertificate(void *system,int ticket,const void *certificate,int size)
{(void)system;(void)ticket;(void)certificate;(void)size;}
const void *SbDrmGetMetrics(void *system,int *size)
{(void)system;if(size)*size=0;return 0;}
void SbDrmDestroySystem(void *system){(void)system;}

int SbMicrophoneGetAvailable(void *information,int count)
{(void)information;return count<0?-1:0;}
void *SbMicrophoneCreate(int id,int frequency,int size)
{(void)id;(void)frequency;(void)size;return 0;}
bool SbMicrophoneOpen(void *microphone){(void)microphone;return false;}
int SbMicrophoneRead(void *microphone,void *data,int size)
{(void)microphone;(void)data;(void)size;return -1;}
void SbMicrophoneDestroy(void *microphone){(void)microphone;}
bool SbSpeechSynthesisIsSupported(void){return false;}
void SbSpeechSynthesisSpeak(const char *text){(void)text;}
void SbSpeechSynthesisCancel(void){}

#include "../include/elf-lifecycle.h"
/* Stay on the caller's stack. A concurrent exit waits for the first cleanup
 * owner; recursive exit by that same owner resumes already-consumed callbacks.
 * No callback is dispatched on fault/forced cancellation by the kernel. */
void MyOSFinalizeImages(void)
{
    uint64_t address=*(const uint64_t *)(uintptr_t)(0x803f010000ull+56);
    if(address!=ELF_PAIR_CONTROL_ADDRESS){(void)call3(SYS_EXIT,(uint64_t)-8,0,0);__builtin_trap();}
    volatile struct elf_pair_control *control=(void *)(uintptr_t)address;
    if(control->magic!=ELF_PAIR_MAGIC||control->count!=2){
        (void)call3(SYS_EXIT,(uint64_t)-8,0,0);__builtin_trap();}
    uint64_t caller=self_page(),expected=0;
    if(!__atomic_compare_exchange_n(&control->exit_owner,&expected,caller,false,__ATOMIC_ACQ_REL,__ATOMIC_ACQUIRE)&&expected!=caller){
        for(;;)(void)wait64(&control->exit_owner,expected,UINT64_MAX);
    }
    for(unsigned i=0;i<2;++i){volatile struct elf_image_lifecycle *image=&control->images[i];
        if(!image->active)continue;
        uint64_t function=image->cxa;image->cxa=0;
        if(function)((void (*)(void *))(uintptr_t)function)(0);
        while(image->fini_count){uint64_t index=image->fini_count-1;image->fini_count=index;
            ((void (**)(void))(uintptr_t)image->fini_array)[index]();}
        function=image->fini;image->fini=0;
        if(function)((void (*)(void))(uintptr_t)function)();
        image->active=0;
    }
}

#include "platform-message.inc"
#include "platform-server.inc"
#include "platform-interface.inc"
