#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"jump SDK failed: %s line %d\n",#x,__LINE__);__builtin_trap();}}while(0)
#ifdef __cplusplus
extern "C" {
#endif
int MyOSJumpRegisterProbe(jmp_buf,int);
int _start(void);
#ifdef __cplusplus
}
static_assert(sizeof(jmp_buf)==200&&alignof(jmp_buf)==8,"target musl jump storage");
#else
_Static_assert(sizeof(jmp_buf)==200&&_Alignof(jmp_buf)==8,"target musl jump storage");
#endif
struct guarded {uint64_t before;jmp_buf jump;uint64_t after;};
/* The base case deliberately returns nonlocally, never via ordinary return.
 * Keep actual nested stack frames rather than a tail-call transformation. */
__attribute__((noinline,noreturn,optimize("no-optimize-sibling-calls")))
static void deep(jmp_buf jump,int value,unsigned depth)
{
    volatile size_t length=137+depth*197;
    volatile unsigned char *area=(volatile unsigned char *)alloca(length);
    for(size_t i=0;i<length;++i)area[i]=(unsigned char)(i+depth);
    CHECK(!((uintptr_t)area&15));
    if(depth)deep(jump,value,depth-1);
    else longjmp(jump,value);
    __asm__ volatile(""::"r"(area):"memory");
}
static void exercise(int value)
{
    struct guarded buffer;memset(&buffer,0xa5,sizeof(buffer));errno=EIO;
    int result=setjmp(buffer.jump);
    if(!result)deep(buffer.jump,value,7);
    CHECK(result==(value?value:1)&&errno==EIO);
    CHECK(buffer.before==UINT64_C(0xa5a5a5a5a5a5a5a5)&&buffer.after==buffer.before);
    const unsigned char *tail=(const unsigned char *)buffer.jump+64;
    for(unsigned i=0;i<136;++i)CHECK(tail[i]==0xa5);
    CHECK(MyOSJumpRegisterProbe(buffer.jump,value));
    CHECK(buffer.before==UINT64_C(0xa5a5a5a5a5a5a5a5)&&buffer.after==buffer.before);
    jmp_buf outer,inner;volatile int sequence=0;
    int outer_result=_setjmp(outer);
    if(!outer_result){
        int inner_result=setjmp(inner);
        if(!inner_result){sequence=1;longjmp(inner,7);}
        CHECK(inner_result==7&&sequence==1);sequence=2;_longjmp(outer,-9);
    }
    CHECK(outer_result==-9&&sequence==2);
}
static void *worker(void *raw)
{
    int id=(int)(uintptr_t)raw;
    for(int i=0;i<8;++i)exercise((i&1)?-(id*100+i):id*100+i);
    return 0;
}
int _start(void)
{
    int values[]={0,1,-1,7,-7,2147483647,(-2147483647-1)};
    for(unsigned i=0;i<7;++i)exercise(values[i]);
    pthread_t threads[8];
    for(unsigned i=0;i<8;++i)CHECK(!pthread_create(threads+i,0,worker,(void *)(uintptr_t)i));
    for(unsigned i=0;i<8;++i)CHECK(!pthread_join(threads[i],0));
    puts("native sealed jumps/registers/preemption/nested stacks/8workers PASS");return 89;
}
