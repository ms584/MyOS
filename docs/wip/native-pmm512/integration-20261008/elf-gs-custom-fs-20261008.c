/* No PT_TLS: raw worker FS points to an application-owned non-TCB value. */
#include <stdint.h>
#include <stddef.h>
extern int *__errno_location(void);
extern void *mmap(void *,size_t,int,int,int,int64_t);
extern int munmap(void *,size_t);
extern void SbLogRaw(const char *);
#define CHECK(x) do{if(!(x)){SbLogRaw("GS custom FS failed: " #x "\n");return __LINE__;}}while(0)
static int64_t raw(unsigned number,uint64_t x,uint64_t y,uint64_t z,uint64_t w)
{
 register uint64_t a __asm__("rax")=number,t __asm__("r10")=w;
 __asm__ volatile("int $0x80":"+a"(a):"D"(x),"S"(y),"d"(z),"r"(t):"memory","cc");return (int64_t)a;
}
struct context {uint64_t fs_magic;int sentinel;uintptr_t errno_address;};
static struct context contexts[8];
static uintptr_t main_errno;
static volatile unsigned ready,release_workers;
static _Noreturn void worker_exit(int status)
{(void)raw(18,(unsigned)status,0,0,0);__builtin_trap();}
static _Noreturn void worker(void *argument)
{
 struct context *c=argument;int *location=__errno_location();
 uint64_t fs;__asm__ volatile("mov %%fs:0,%0":"=r"(fs));
 if(fs!=c->fs_magic||location!=(int *)(uintptr_t)raw(24,0,0,0,0)||
    (uintptr_t)location==main_errno)worker_exit(1);
 c->errno_address=(uintptr_t)location;*location=c->sentinel;
 __atomic_add_fetch(&ready,1,__ATOMIC_RELEASE);
 while(!__atomic_load_n(&release_workers,__ATOMIC_ACQUIRE))(void)raw(4,0,0,0,0);
 for(unsigned i=0;i<300;++i){
  (void)raw(4,0,0,0,0);__asm__ volatile("mov %%fs:0,%0":"=r"(fs));
  if(fs!=c->fs_magic||__errno_location()!=location||*location!=c->sentinel||
     location!=(int *)(uintptr_t)raw(24,0,0,0,0))worker_exit(2);
 }
 worker_exit(93);
}
int _start(void)
{
 main_errno=(uintptr_t)__errno_location();*(int *)main_errno=777;
 for(unsigned wave=0;wave<3;++wave){
  void *stacks[8];uint64_t tids[8];ready=release_workers=0;
  for(unsigned i=0;i<8;++i){
   contexts[i]=(struct context){0xabc00000u+wave*16+i,500+(int)i,0};
   stacks[i]=mmap(0,65536,3,0x22,-1,0);CHECK(stacks[i]!=(void *)(uintptr_t)-1);
   int64_t tid=raw(17,(uintptr_t)worker,(uintptr_t)&contexts[i],(uintptr_t)stacks[i]+65536,(uintptr_t)&contexts[i].fs_magic);
   CHECK(tid>0);tids[i]=(uint64_t)tid;
  }
  while(__atomic_load_n(&ready,__ATOMIC_ACQUIRE)!=8)(void)raw(4,0,0,0,0);
  for(unsigned i=0;i<8;++i)for(unsigned j=i+1;j<8;++j)CHECK(contexts[i].errno_address!=contexts[j].errno_address);
  CHECK(__errno_location()==(int *)main_errno&&*(int *)main_errno==777);
  __atomic_store_n(&release_workers,1,__ATOMIC_RELEASE);
  for(unsigned i=0;i<8;++i){
   int status=-1;int64_t rc;
   while((rc=raw(19,tids[i],(uintptr_t)&status,0,0))==-11)(void)raw(4,0,0,0,0);
   CHECK(!rc&&status==93);CHECK(!munmap(stacks[i],65536));
  }
  CHECK(__errno_location()==(int *)main_errno&&*(int *)main_errno==777);
 }
 SbLogRaw("GS errno independent of custom FS/non-TCB/24-worker-reuse PASS\n");return 93;
}
