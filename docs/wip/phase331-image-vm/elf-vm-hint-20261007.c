#include <stdint.h>
#include <stddef.h>
extern void *mmap(void *,size_t,int,int,int,int64_t);
extern int mprotect(void *,size_t,int),munmap(void *,size_t),mincore(void *,size_t,unsigned char *);
extern int *__errno_location(void);
extern void SbLogRaw(const char *);
#define CHECK(x) do{if(!(x)){SbLogRaw("VM hint FAIL: " #x "\n");return __LINE__;}}while(0)
int _start(void)
{
 const uintptr_t near=UINT64_C(0x8050000000);const size_t length=512u*1024u*1024u;
 unsigned char resident[3]={7,7,7};
 void *a=mmap((void *)(near+123),length,0,34,-1,0);
 CHECK(a==(void *)near);CHECK(!mincore(a,12288,resident));
 CHECK(!resident[0]&&!resident[1]&&!resident[2]);
 CHECK(!mprotect(a,4096,3));*(volatile uint64_t *)a=77;
 CHECK(!mincore(a,12288,resident)&&resident[0]&&!resident[1]&&!resident[2]);
 /* A colliding nonfixed hint must not unmap or overwrite its earlier owner. */
 void *b=mmap(a,16384,3,34,-1,0);CHECK(b!=(void *)-1&&b!=a);
 CHECK(*(volatile uint64_t *)a==77);*(volatile uint64_t *)b=88;
 CHECK(mmap(a,length,0,0x100022,-1,0)==(void *)-1&&*__errno_location()==17);
 CHECK(*(volatile uint64_t *)a==77);
 void *c=mmap((void *)UINT64_MAX,4096,3,34,-1,0);CHECK(c!=(void *)-1&&c!=a&&c!=b);
 *(volatile uint64_t *)c=99;
 CHECK(!munmap(c,4096)&&!munmap(b,16384)&&!munmap(a,length));
 CHECK(mmap((void *)near,0,0,34,-1,0)==(void *)-1&&*__errno_location()==22);
 /* Default512MiB reservation cannot fit the low pool; near pool is preferred
  * before the old high pool, keeping this reservation within2GiB of ELF. */
 a=mmap(0,length,0,34,-1,0);CHECK(a!=(void *)-1);
 CHECK((uintptr_t)a>=0x8040000000ull&&(uintptr_t)a+length<=0x8080000000ull);
 CHECK(!munmap(a,length));
 /* Near-image address is not permission to replace loader/runtime mappings. */
 a=mmap((void *)0x8000000000ull,4096,3,34,-1,0);CHECK(a!=(void *)-1&&a!=(void *)0x8000000000ull);
 CHECK(!munmap(a,4096));
 SbLogRaw("native VM hint near512MiB sparse reservation/collision/NOREPLACE/unaligned/invalid/default placement PASS\n");return 96;
}
