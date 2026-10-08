#include <pthread.h>
#include <stdint.h>
#include <stddef.h>
extern int *__errno_location(void);
extern int sched_yield(void);
extern void SbLogRaw(const char *);
#define CHECK(x) do{if(!(x)){SbLogRaw("GS errno failed: " #x "\n");return __LINE__;}}while(0)
#if WITH_TLS
static _Thread_local uint64_t local=0x1a2b3c4d;
#endif
static volatile unsigned ready,go;
static int *locations[6];
static int *oracle(void)
{
 register uint64_t a __asm__("rax")=24;
 __asm__ volatile("int $0x80":"+a"(a)::"memory","cc");return (int *)(uintptr_t)a;
}
static uint64_t stamp(void)
{uint32_t low,high;__asm__ volatile("lfence;rdtsc":"=a"(low),"=d"(high)::"memory");return ((uint64_t)high<<32)|low;}
static void *worker(void *raw)
{
 unsigned index=(unsigned)(uintptr_t)raw;int *p=__errno_location();
 if(p!=oracle())return (void *)1;
 locations[index]=p;*p=(int)(500+index);
#if WITH_TLS
 if(local!=0x1a2b3c4d)return (void *)2;local=0x9000+index;
#endif
 __atomic_add_fetch(&ready,1,__ATOMIC_RELEASE);
 while(!__atomic_load_n(&go,__ATOMIC_ACQUIRE))sched_yield();
 for(unsigned i=0;i<300;++i){
  sched_yield();if(__errno_location()!=p||oracle()!=p||*p!=(int)(500+index))return (void *)3;
#if WITH_TLS
  if(local!=0x9000+index)return (void *)4;
#endif
 }
 return 0;
}
int _start(void)
{
 int *main_location=__errno_location();CHECK(main_location==oracle());*main_location=12345;
#if WITH_TLS
 CHECK(local==0x1a2b3c4d);local=0xabcdef;
#endif
 for(unsigned round=0;round<8;++round){
  ready=go=0;pthread_t threads[6];
  for(uintptr_t i=0;i<6;++i)CHECK(!pthread_create(&threads[i],0,worker,(void *)i));
  while(__atomic_load_n(&ready,__ATOMIC_ACQUIRE)!=6)sched_yield();
  for(unsigned i=0;i<6;++i){CHECK(locations[i]!=main_location);for(unsigned j=0;j<i;++j)CHECK(locations[i]!=locations[j]);}
  __atomic_store_n(&go,1,__ATOMIC_RELEASE);
  for(unsigned i=0;i<6;++i){void *result;CHECK(!pthread_join(threads[i],&result)&&!result);}
  CHECK(__errno_location()==main_location&&oracle()==main_location&&*main_location==12345);
#if WITH_TLS
  CHECK(local==0xabcdef);
#endif
 }
 uint64_t begin=stamp();for(unsigned i=0;i<25000;++i)CHECK(__errno_location()==main_location);
 uint64_t direct=stamp()-begin;begin=stamp();for(unsigned i=0;i<25000;++i)CHECK(oracle()==main_location);
 uint64_t syscall=stamp()-begin;char text[128];unsigned n=0;const char *label="GS errno raw TSC direct/syscall";
 while(*label)text[n++]=*label++;uint64_t values[]={direct,syscall};
 for(unsigned i=0;i<2;++i){text[n++]=' ';for(int shift=60;shift>=0;shift-=4)text[n++]="0123456789abcdef"[(values[i]>>shift)&15];}
 text[n++]='\n';text[n]=0;SbLogRaw(text);
 SbLogRaw("GS errno oracle/isolation/preemption/48-worker-reuse PASS\n");return 89;
}
