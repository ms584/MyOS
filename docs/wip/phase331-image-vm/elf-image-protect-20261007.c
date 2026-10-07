#include <stdint.h>
#include <stddef.h>
extern int mprotect(void *,size_t,int);
extern int *__errno_location(void);
extern void SbLogRaw(const char *);
extern char _end __attribute__((visibility("hidden")));
#ifndef MODE
#define MODE 0
#endif
static volatile unsigned char values[8192] __attribute__((aligned(4096)));
static void *const volatile readonly_pointer __attribute__((section(".data.rel.ro")))=(void *)&values;
static uintptr_t page(const void *p){return (uintptr_t)p&~(uintptr_t)4095;}
#define CHECK(x) do{if(!(x)){SbLogRaw("Image protect FAIL: " #x "\n");return __LINE__;}}while(0)
int _start(void)
{
 values[0]=42;values[4096]=77;
 if(MODE==1){CHECK(!mprotect((void *)values,4096,1));values[0]=9;return 1;}
 if(MODE==2){CHECK(!mprotect((void *)values,4096,0));return values[0];}
 if(MODE==3){values[0]=0xc3;((void (*)(void))(uintptr_t)values)();return 3;}
 CHECK(readonly_pointer==(void *)values);
 CHECK(!mprotect((void *)values,8192,1));CHECK(values[0]==42&&values[4096]==77);
 CHECK(!mprotect((void *)values,8192,3));values[0]=43;
 CHECK(!mprotect((void *)values,8192,0));
 CHECK(!mprotect((void *)values,8192,3));CHECK(values[0]==43);values[4096]=78;
 CHECK(mprotect((void *)values,4096,5)==-1&&*__errno_location()==13);
 values[0]=44;CHECK(values[4096]==78);
 CHECK(mprotect((void *)values,4096,7)==-1&&*__errno_location()==22);
 CHECK(mprotect((void *)page((void *)(uintptr_t)_start),4096,3)==-1&&*__errno_location()==13);
 CHECK(mprotect((void *)page((const void *)&readonly_pointer),4096,3)==-1&&*__errno_location()==13);
 CHECK(readonly_pointer==(void *)values);
 CHECK(mprotect((void *)0x803f000000ull,4096,1)==-1&&*__errno_location()==12);
 CHECK(mprotect((void *)0x8010001000ull,4096,1)==-1&&*__errno_location()==12);
 uintptr_t hole=((uintptr_t)&_end+4095)&~(uintptr_t)4095;
 CHECK(hole>=(uintptr_t)values+sizeof(values));
 CHECK(mprotect((void *)values,hole+4096-(uintptr_t)values,1)==-1&&*__errno_location()==12);
 /* Failed mixed range must not have changed earlier writable pages. */
 values[0]=45;values[4096]=79;CHECK(values[0]==45&&values[4096]==79);
 CHECK(mprotect((void *)((uintptr_t)values+1),4096,1)==-1&&*__errno_location()==22);
 SbLogRaw("native image mprotect rights ceilings/RELRO/W^X/NONE restore/atomic ranges PASS\n");return 95;
}
