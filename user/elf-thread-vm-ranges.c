#include <stddef.h>
#include <stdint.h>
struct timespec { int64_t tv_sec,tv_nsec; };
extern void *mmap(void *,size_t,int,int,int,int64_t);
extern int munmap(void *,size_t),pthread_create(uintptr_t *,const void *,void *(*)(void *),void *);
extern int pthread_join(uintptr_t,void **),pthread_detach(uintptr_t);
extern int pthread_getattr_np(uintptr_t,void *),pthread_attr_getstacksize(const void *,size_t *);
extern int clock_nanosleep(int,int,const struct timespec *,struct timespec *);
static volatile unsigned completed;
static void message(const char *s){
 size_t n=0;while(s[n])++n;
 register uint64_t a __asm__("rax")=1,d __asm__("rdi")=(uintptr_t)s,v __asm__("rsi")=n;
 __asm__ volatile("int $0x80":"+a"(a):"D"(d),"S"(v):"rdx","memory","cc");
}
static void check(int yes){if(!yes)__builtin_trap();}
static void delay(unsigned ms){struct timespec t={0,(int64_t)ms*1000000};check(!clock_nanosleep(1,0,&t,0));}
static void *worker(void *arg){
 delay(150);__atomic_store_n(&completed,1,__ATOMIC_RELEASE);return arg;
}
static int exercise(uintptr_t start,uintptr_t end){
 uintptr_t thread;completed=0;
 check(!pthread_create(&thread,0,worker,(void *)0x1234));
 check(thread>=start&&thread<end);
 unsigned char attr[80];size_t size;
 int rc=pthread_getattr_np(thread,attr);
 if(rc){message("native expanded pthread getattr rejected\n");return 1;}
 check(!pthread_attr_getstacksize(attr,&size)&&size>=16384);
 void *value=0;rc=pthread_join(thread,&value);
 if(rc){message("native expanded pthread join rejected\n");return 1;}
 check(__atomic_load_n(&completed,__ATOMIC_ACQUIRE)&&value==(void *)0x1234);
 check(pthread_join(thread,0)==3); /* expired mapping cannot be reused */
 completed=0;check(!pthread_create(&thread,0,worker,0));
 check(thread>=start&&thread<end);check(!pthread_detach(thread));
 while(!__atomic_load_n(&completed,__ATOMIC_ACQUIRE))delay(10);
 delay(200);return 0;
}
#ifndef MYOS_TEST_OLD_RANGE_REJECTION
extern int *__errno_location(void);
static void reservation_capacity(void){
 const uintptr_t base=0x8020000000ull,big=base+0x04000000ull;
 const size_t page=4096;const int flags=0x100022;
 check(mmap((void *)big,3*page,0,flags,-1,0)==(void *)big);
 unsigned count=0;
 for(;count<2048;++count){
  uintptr_t address=base+(uintptr_t)count*2*page;
  void *value=mmap((void *)address,page,0,flags,-1,0);
  if(value==(void *)-1){check(*__errno_location()==12);break;}
  check(value==(void *)address);
 }
 check(count>256&&count<=1024); /* formerly failed real YouTube reservation bound */
 /* Full-table splitting fails atomically, preserving middle-page ownership. */
 check(munmap((void *)(big+page),page)==-1&&*__errno_location()==12);
 check(mmap((void *)(big+page),page,0,flags,-1,0)==(void *)-1&&*__errno_location()==17);
 /* Make one metadata slot, split successfully, then merge even at full cap. */
 check(!munmap((void *)base,page));
 check(!munmap((void *)(big+page),page));
 check(mmap((void *)(big+page),page,0,flags,-1,0)==(void *)(big+page));
 check(!munmap((void *)big,3*page));
 /* Free alternating reservations and reuse exactly those disjoint holes. */
 for(unsigned i=2;i<count;i+=2)check(!munmap((void *)(base+(uintptr_t)i*2*page),page));
 for(unsigned i=2;i<count;i+=2){
  uintptr_t address=base+(uintptr_t)i*2*page;
  check(mmap((void *)address,page,0,flags,-1,0)==(void *)address);
 }
 for(unsigned i=1;i<count;++i)check(!munmap((void *)(base+(uintptr_t)i*2*page),page));
 message("native >256 fragmented VM reservations/full-table atomic split/merge/reuse PASS\n");
}
#endif
int _start(void){
#ifndef MYOS_TEST_OLD_RANGE_REJECTION
 reservation_capacity();
#endif
 const uintptr_t low=0x8020000000ull,mid=0x8040000000ull,high=0x8100000000ull,end=0xc100000000ull;
 /* PROT_NONE reserves address space without exhausting physical RAM. */
 void *a=mmap((void *)low,0x1f000000,0,0x100022,-1,0);
 check(a==(void *)low);
 int result=exercise(mid,high);if(result)return result;
 void *b=mmap((void *)mid,0xc0000000ull,0,0x100022,-1,0);
 check(b==(void *)mid);
 result=exercise(high,end);if(result)return result;
 check(pthread_join(mid,0)==3&&pthread_join(low,0)==3);
 check(!munmap(b,0xc0000000ull)&&!munmap(a,0x1f000000));
 message("native expanded pthread join/detach/getattr/guard PASS\n");return 0;
}
