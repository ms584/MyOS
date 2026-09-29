#include <stddef.h>
#include <stdint.h>

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
static void *allocate(size_t alignment,size_t size,int exact)
{
    if(!size||!power_of_two(alignment)||alignment<sizeof(void *))return 0;
    if(exact&&size%alignment)return 0;
    if(size>SIZE_MAX-sizeof(struct allocation)-alignment)return 0;
    size_t need=size+sizeof(struct allocation)+alignment-1;
    if(need>SIZE_MAX-(PAGE_SIZE-1))return 0;
    size_t length=(need+PAGE_SIZE-1)&~(size_t)(PAGE_SIZE-1);
    int64_t mapped=call3(SYS_VM_MAP,0,length,PROT_RW);
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
    if(header->magic!=HEADER_MAGIC)return;
    uint64_t base=header->base,length=header->length;header->magic=0;
    (void)call3(SYS_VM_UNMAP,base,length,0);
}
void *calloc(size_t count,size_t size)
{
    if(count&&size>SIZE_MAX/count){*__errno_location()=12;return 0;}
    return malloc(count*size);
}
size_t malloc_usable_size(void *address)
{
    if(!address)return 0;
    struct allocation *header=(struct allocation *)address-1;
    return header->magic==HEADER_MAGIC?(size_t)header->requested:0;
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
#define RUNTIME_TLS_BASE 0x803f010000ull
#define THREAD_STACK_SIZE (1024u*1024u)
#define MAIN_STACK_BASE 0x8010001000ull
#define MAIN_STACK_SIZE (4u*4096u)
#define THREAD_MAGIC 0x4d594f5350544852ull
#define EAGAIN 11
#define EINVAL 22
#define ESRCH 3
#define EBUSY 16
#define EDEADLK 35
#define ETIMEDOUT 110
#define THREAD_KEYS 32u

struct timespec { int64_t tv_sec,tv_nsec; };
struct thread_control {
    uint64_t magic,tid,base,length;
    void *(*start)(void *);
    void *argument,*result;
    uint64_t detached,done;
    char name[16];
};
struct thread_attr { uint64_t magic,stack_size,detached,stack_base; };

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
static void yield_cpu(void){(void)call3(SYS_YIELD,0,0,0);}
static int wait64(volatile uint64_t *word,uint64_t value,uint64_t deadline)
{return (int)call3(SYS_WAIT,(uintptr_t)word,value,deadline);}
static void wake64(volatile uint64_t *word,unsigned count)
{(void)call3(SYS_WAKE,(uintptr_t)word,count,0);}
static uint64_t deadline_ticks(const struct timespec *time)
{
    if(!time||time->tv_sec<0||time->tv_nsec<0||time->tv_nsec>=1000000000)return UINT64_MAX;
    if((uint64_t)time->tv_sec>UINT64_MAX/100)return UINT64_MAX-1;
    return (uint64_t)time->tv_sec*100+(uint64_t)time->tv_nsec/10000000;
}

static uint32_t *key_bits(void){return (uint32_t *)(uintptr_t)(RUNTIME_TLS_BASE+256);}
static void (**key_destructors(void))(void *)
{return (void (**)(void *))(uintptr_t)(RUNTIME_TLS_BASE+512);}
static void **key_values(void){return (void **)(uintptr_t)(self_page()+64);}
static void run_destructors(void)
{
    for(unsigned pass=0;pass<4;++pass){int any=0;
        for(unsigned key=0;key<THREAD_KEYS;++key){void *value=key_values()[key];
            void (*destructor)(void *)=key_destructors()[key];
            if(value&&destructor){key_values()[key]=0;destructor(value);any=1;}
        }
        if(!any)break;
    }
}
static void thread_entry(void *raw)
{
    struct thread_control *control=raw;
    *(struct thread_control **)(uintptr_t)(self_page()+8)=control;
    for(unsigned i=0;i<THREAD_KEYS;++i)key_values()[i]=0;
    control->result=control->start(control->argument);
    run_destructors();
    __atomic_store_n(&control->done,1,__ATOMIC_RELEASE);wake64(&control->done,~0u);
    (void)call3(SYS_THREAD_EXIT,0,0,0);
    (void)call3(SYS_EXIT,127,0,0);
    for(;;){}
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
    *control=(struct thread_control){THREAD_MAGIC,0,(uint64_t)(uintptr_t)mapping,length,start,argument,0,0,0,{0}};
    int64_t tid=call4(SYS_THREAD_CREATE,(uintptr_t)thread_entry,(uintptr_t)control,
                      (uintptr_t)mapping+length,0);
    if(tid<0){munmap(mapping,length);return (int)-tid;}
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
int pthread_join(uint64_t thread,void **result)
{
    struct thread_control *control=(void *)(uintptr_t)thread;
    if(!control||control->magic!=THREAD_MAGIC)return ESRCH;
    if(thread==pthread_self())return EDEADLK;
    if(__atomic_load_n(&control->detached,__ATOMIC_ACQUIRE))return EINVAL;
    int32_t status;
    for(;;){int rc=(int)call3(SYS_THREAD_REAP,control->tid,(uintptr_t)&status,0);
        if(!rc)break;if(rc!=-EAGAIN)return -rc;
        uint64_t deadline=(uint64_t)call3(SYS_TICKS,0,0,0)+1;
        (void)wait64(&control->done,0,deadline);yield_cpu();
    }
    if(result)*result=control->result;
    uint64_t base=control->base,length=control->length;control->magic=0;
    return munmap((void *)(uintptr_t)base,length)?EINVAL:0;
}
int pthread_detach(uint64_t thread)
{
    struct thread_control *control=(void *)(uintptr_t)thread;
    if(!control||control->magic!=THREAD_MAGIC)return ESRCH;
    uint64_t expected=0;
    if(!__atomic_compare_exchange_n(&control->detached,&expected,1,0,__ATOMIC_ACQ_REL,__ATOMIC_ACQUIRE))
        return EINVAL;
    int64_t rc=call3(SYS_THREAD_DETACH,control->tid,control->base,control->length);
    if(rc<0){__atomic_store_n(&control->detached,0,__ATOMIC_RELEASE);return (int)-rc;}
    return 0;
}
int pthread_getattr_np(uint64_t thread,void *raw)
{
    struct thread_control *control=(void *)(uintptr_t)thread;
    if(!raw)return EINVAL;
    struct thread_attr *a=raw;*a=(struct thread_attr){0x41545452,THREAD_STACK_SIZE,0,0};
    if(thread==RUNTIME_TLS_BASE){a->stack_base=MAIN_STACK_BASE;a->stack_size=MAIN_STACK_SIZE;return 0;}
    if(!control||control->magic!=THREAD_MAGIC)return ESRCH;
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
    struct thread_control *control=(void *)(uintptr_t)thread;
    if(thread==RUNTIME_TLS_BASE)return 0;
    if(!control||control->magic!=THREAD_MAGIC||!name)return ESRCH;
    unsigned i=0;for(;i<15&&name[i];++i)control->name[i]=name[i];
    if(name[i])return 34;control->name[i]=0;return 0;
}
int pthread_getname_np(uint64_t thread,char *name,size_t size)
{
    if(!name||!size)return EINVAL;
    if(thread==RUNTIME_TLS_BASE){const char main_name[]="main";size_t i=0;
        for(;i+1<size&&main_name[i];++i)name[i]=main_name[i];name[i]=0;return 0;}
    struct thread_control *control=(void *)(uintptr_t)thread;
    if(!control||control->magic!=THREAD_MAGIC)return ESRCH;
    size_t i=0;for(;i+1<size&&control->name[i];++i)name[i]=control->name[i];name[i]=0;return 0;
}
int pthread_getschedparam(uint64_t thread,int *policy,void *parameter)
{
    if(!thread||!policy||!parameter)return EINVAL;*policy=0;*(int *)parameter=0;return 0;
}
int pthread_kill(uint64_t thread,int signal)
{
    if(!thread)return ESRCH;if(signal)return EINVAL;return 0;
}
int pthread_sigmask(int how,const void *set,void *old)
{
    (void)how;(void)set;if(old)for(unsigned i=0;i<16;++i)((unsigned char *)old)[i]=0;return 0;
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
        if(!rc||rc==EDEADLK)return rc;(void)wait64(&((struct mutex_layout *)raw)->state,1,0);}
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
{if(!raw)return EINVAL;(void)attr;*(uint64_t *)raw=0;return 0;}
int pthread_cond_destroy(void *raw){return raw?0:EINVAL;}
int pthread_cond_signal(void *raw)
{if(!raw)return EINVAL;__atomic_add_fetch((uint64_t *)raw,1,__ATOMIC_RELEASE);wake64(raw,1);return 0;}
int pthread_cond_broadcast(void *raw)
{if(!raw)return EINVAL;__atomic_add_fetch((uint64_t *)raw,1,__ATOMIC_RELEASE);wake64(raw,~0u);return 0;}
static int cond_wait(void *raw,void *mutex,uint64_t deadline)
{
    if(!raw||!mutex)return EINVAL;uint64_t value=__atomic_load_n((uint64_t *)raw,__ATOMIC_ACQUIRE);
    int rc=pthread_mutex_unlock(mutex);if(rc)return rc;
    rc=wait64(raw,value,deadline);int lock=pthread_mutex_lock(mutex);
    if(lock)return lock;return rc==-ETIMEDOUT?ETIMEDOUT:0;
}
int pthread_cond_wait(void *raw,void *mutex){return cond_wait(raw,mutex,0);}
int pthread_cond_timedwait(void *raw,void *mutex,const struct timespec *time)
{
    uint64_t d=deadline_ticks(time);if(d==UINT64_MAX)return EINVAL;
    if(d<=(uint64_t)call3(SYS_TICKS,0,0,0)){
        int rc=pthread_mutex_unlock(mutex);if(rc)return rc;
        rc=pthread_mutex_lock(mutex);return rc?rc:ETIMEDOUT;
    }
    return cond_wait(raw,mutex,d);
}

int sem_init(void *raw,int shared,unsigned value)
{if(!raw||shared||value>0x7fffffff)return EINVAL;*(uint64_t *)raw=value;return 0;}
int sem_destroy(void *raw){return raw?0:EINVAL;}
int sem_post(void *raw)
{
    if(!raw)return -1;__atomic_add_fetch((uint64_t *)raw,1,__ATOMIC_RELEASE);wake64(raw,1);return 0;
}
static int sem_take(void *raw,uint64_t deadline)
{
    if(!raw){*__errno_location()=EINVAL;return -1;}volatile uint64_t *count=raw;
    for(;;){uint64_t value=__atomic_load_n(count,__ATOMIC_ACQUIRE);
        if(value&&__atomic_compare_exchange_n(count,&value,value-1,0,__ATOMIC_ACQUIRE,__ATOMIC_RELAXED))return 0;
        int rc=wait64(count,0,deadline);if(rc==-ETIMEDOUT){*__errno_location()=ETIMEDOUT;return -1;}
    }
}
int sem_wait(void *raw){return sem_take(raw,0);}
int sem_timedwait(void *raw,const struct timespec *time)
{
    uint64_t d=deadline_ticks(time);
    if(d==UINT64_MAX){*__errno_location()=EINVAL;return -1;}
    if(d<=(uint64_t)call3(SYS_TICKS,0,0,0)){*__errno_location()=ETIMEDOUT;return -1;}
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
int pthread_key_create(unsigned *key,void (*destructor)(void *))
{
    if(!key)return EINVAL;
    for(unsigned i=0;i<THREAD_KEYS;++i){uint32_t bit=1u<<i,old=__atomic_fetch_or(key_bits(),bit,__ATOMIC_ACQ_REL);
        if(!(old&bit)){key_destructors()[i]=destructor;*key=i;return 0;}}
    return EAGAIN;
}
int pthread_key_delete(unsigned key)
{
    if(key>=THREAD_KEYS)return EINVAL;uint32_t bit=1u<<key;
    if(!(__atomic_fetch_and(key_bits(),~bit,__ATOMIC_ACQ_REL)&bit))return EINVAL;
    key_destructors()[key]=0;return 0;
}
int pthread_setspecific(unsigned key,const void *value)
{
    if(key>=THREAD_KEYS||!(__atomic_load_n(key_bits(),__ATOMIC_ACQUIRE)&(1u<<key)))return EINVAL;
    key_values()[key]=(void *)value;return 0;
}
void *pthread_getspecific(unsigned key)
{
    if(key>=THREAD_KEYS||!(__atomic_load_n(key_bits(),__ATOMIC_ACQUIRE)&(1u<<key)))return 0;
    return key_values()[key];
}
