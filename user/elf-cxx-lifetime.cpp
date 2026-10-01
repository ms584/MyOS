#include <new>
#include <pthread.h>
#include <stdint.h>
extern "C" int __cxa_atexit(void (*)(void *),void *,void *);
extern "C" void __cxa_finalize(void *);
extern "C" int atexit(void (*)(void));
static void require(bool value){if(!value)__builtin_trap();}
static unsigned constructed,destroyed,global_destroyed,phase,used;
static unsigned trace[16];
static unsigned char dso_a,dso_b;
static unsigned char dso_threads;
static unsigned thread_mask,ready,active_callbacks,overlap;
static void *pressure[2048];
static unsigned exhaust()
{
    unsigned count=0;
    while(count<128){void *p=::operator new(1024*1024,std::nothrow);if(!p)break;pressure[count++]=p;}
    require(count>0&&count<128);
    while(count<2048){void *p=::operator new(4096,std::nothrow);if(!p)break;pressure[count++]=p;}
    require(count<2048);return count;
}
static void thread_finish(void *raw)
{
    if(__atomic_add_fetch(&active_callbacks,1,__ATOMIC_SEQ_CST)>1)
        __atomic_store_n(&overlap,1,__ATOMIC_RELEASE);
    while(!__atomic_load_n(&overlap,__ATOMIC_ACQUIRE)){
        timespec delay={0,2000000};clock_nanosleep(CLOCK_MONOTONIC,0,&delay,nullptr);}
    unsigned bit=1u<<(unsigned)(uintptr_t)raw;
    require(!(__atomic_fetch_or(&thread_mask,bit,__ATOMIC_SEQ_CST)&bit));
    __atomic_sub_fetch(&active_callbacks,1,__ATOMIC_SEQ_CST);
}
static void *finalizer_worker(void *){__cxa_finalize(&dso_threads);return nullptr;}
static void event(unsigned value){require(used<16);trace[used++]=value;}
static void callback(void *raw)
{
    unsigned value=(unsigned)(uintptr_t)raw;event(value);
    if(value==4)require(!__cxa_atexit(callback,(void *)5,&dso_a));
    if(value==6)__cxa_finalize(&dso_b);
}
struct Item {
    unsigned value=123;
    Item(){__atomic_add_fetch(&constructed,1,__ATOMIC_RELAXED);}
    ~Item(){require(value==123);__atomic_add_fetch(&destroyed,1,__ATOMIC_RELAXED);}
};
struct alignas(4096) Aligned { unsigned value=456; };
struct Global {
    Item *owned;
    Global():owned(new Item){}
    ~Global(){require(phase==1);delete owned;++global_destroyed;}
};
static Global global;
static void plain(){event(9);}
static void *worker(void *identity)
{
    require(!__cxa_atexit(thread_finish,identity,&dso_threads));
#ifdef CXX_CANCEL
    Item *held=new Item;require(held);
    __atomic_add_fetch(&ready,1,__ATOMIC_RELEASE);
    for(;;){timespec wait={1,0};clock_nanosleep(CLOCK_MONOTONIC,0,&wait,nullptr);}
#endif
    for(unsigned i=0;i<12;++i){
        Item *one=new Item;require(one&&((uintptr_t)one&15)==0);delete one;
        Item *many=new Item[3];delete[] many;
        Aligned *aligned=new Aligned;require(((uintptr_t)aligned&4095)==0&&aligned->value==456);delete aligned;
        Aligned *array=new Aligned[2];require(((uintptr_t)array&4095)==0);delete[] array;
    }
    return (void *)88;
}
__attribute__((destructor(101))) static void final_check()
{
    require(phase==1&&global_destroyed==1&&constructed==destroyed);
    require(used==8&&trace[7]==9);
    const char message[]="native C++ allocation/destructors/finalizers PASS\n";
    register uint64_t a __asm__("rax")=1,d __asm__("rdi")=(uintptr_t)message;
    register uint64_t s __asm__("rsi")=sizeof(message)-1;
    __asm__ volatile("int $0x80":"+a"(a):"D"(d),"S"(s):"rdx","memory","cc");
}
extern "C" int _start()
{
    volatile size_t huge=~size_t(0);
#ifdef CXX_OOM
    exhaust();void *result=::operator new(8);require(result);return 1;
#endif
#ifdef CXX_PRESSURE
    unsigned count=exhaust();
    require(__cxa_atexit(callback,(void *)66,&dso_a)!=0&&atexit(plain)!=0);
    for(unsigned i=0;i<count;++i)::operator delete(pressure[i]);
#endif
    require(::operator new(huge,std::nothrow)==nullptr);
    require(::operator new(huge,std::align_val_t(4096),std::nothrow)==nullptr);
    require(::operator new(4,std::align_val_t(3),std::nothrow)==nullptr);
    void *zero=::operator new(0);require(zero);::operator delete(zero);
    zero=::operator new[](0,std::nothrow);require(zero);::operator delete[](zero);
    require(!__cxa_atexit(callback,(void *)1,&dso_a));
    require(!__cxa_atexit(callback,(void *)2,&dso_b));
    require(!__cxa_atexit(callback,(void *)3,&dso_a));
    __cxa_finalize(&dso_a);require(used==2&&trace[0]==3&&trace[1]==1);
    __cxa_finalize(&dso_a);require(used==2);
    require(!__cxa_atexit(callback,(void *)4,&dso_a));
    __cxa_finalize(&dso_a);require(used==4&&trace[2]==4&&trace[3]==5);
    require(!__cxa_atexit(callback,(void *)7,&dso_b));
    require(!__cxa_atexit(callback,(void *)6,&dso_b));
    __cxa_finalize(&dso_b);require(used==7&&trace[4]==6&&trace[5]==7&&trace[6]==2);
    require(__cxa_atexit(nullptr,nullptr,&dso_a)!=0);
    require(atexit(nullptr)!=0&&atexit(plain)==0);
    pthread_t threads[4];
    for(unsigned i=0;i<4;++i)require(!pthread_create(&threads[i],nullptr,worker,(void *)(uintptr_t)(i+1)));
#ifdef CXX_CANCEL
    while(__atomic_load_n(&ready,__ATOMIC_ACQUIRE)!=4){timespec delay={0,2000000};clock_nanosleep(CLOCK_MONOTONIC,0,&delay,nullptr);}
    const char cancel[]="native C++ allocated workers cancel ready\n";
    register uint64_t a __asm__("rax")=1,d __asm__("rdi")=(uintptr_t)cancel;
    register uint64_t s __asm__("rsi")=sizeof(cancel)-1;
    __asm__ volatile("int $0x80":"+a"(a):"D"(d),"S"(s):"rdx","memory","cc");
    pthread_join(threads[0],nullptr);__builtin_trap();
#endif
    for(unsigned i=0;i<4;++i){void *result;require(!pthread_join(threads[i],&result)&&result==(void *)88);}
    for(unsigned i=0;i<2;++i)require(!pthread_create(&threads[i],nullptr,finalizer_worker,nullptr));
    for(unsigned i=0;i<2;++i)require(!pthread_join(threads[i],nullptr));
    require(thread_mask==0x1e&&overlap&&active_callbacks==0);
    phase=1;
    return 73;
}
