#include <stddef.h>
#include <stdint.h>
extern void *mmap(void *,size_t,int,int,int,int64_t);
extern int mprotect(void *,size_t,int);
extern int64_t write(int,const void *,size_t);
static int64_t system_call(uint64_t number,uint64_t first,uint64_t second,uint64_t third)
{
    register uint64_t result __asm__("rax")=number;
    __asm__ volatile("int $0x80":"+a"(result):"D"(first),"S"(second),"d"(third):"rcx","r11","memory");
    return (int64_t)result;
}
int _start(void)
{
    const size_t length=280u*1024u*1024u;
    unsigned char *memory=mmap(0,length,3,0x22,-1,0);
    if(memory==(void *)-1)return 2;
    /* More resident pages than can fit below256MiB. Validate kernel copyout
     * on every frame, not just a virtual pointer or synthetic memory count. */
    for(size_t offset=0;offset<length;offset+=4096)
        if(system_call(51,3,(uintptr_t)(memory+offset),8))return 3;
    unsigned char *tail=memory+length-4096;
    const char text[]="upper PMM syscall copy/read/write PASS\n";
    for(size_t i=0;i<sizeof(text);++i)tail[16+i]=(unsigned char)text[i];
    if(system_call(1,(uintptr_t)(tail+16),sizeof(text)-1,0)!=(int64_t)sizeof(text)-1)return 4;
    for(unsigned i=4092;i<4096;++i)tail[i]=0xa5;
    if(system_call(51,3,(uintptr_t)(tail+4092),8)!=-14)return 5;
    for(unsigned i=4092;i<4096;++i)if(tail[i]!=0xa5)return 6;
    if(mprotect(tail,4096,1)||system_call(51,3,(uintptr_t)tail,8)!=-14)return 7;
    if(system_call(1,(uintptr_t)(tail+16),sizeof(text)-1,0)!=(int64_t)sizeof(text)-1)return 8;
    if(mprotect(tail,4096,0)||system_call(1,(uintptr_t)tail,8,0)!=-14||
        system_call(51,3,(uintptr_t)tail,8)!=-14)return 9;
    if(system_call(51,3,0x100000,8)!=-14)return 10;
    const char done[]="upper PMM permissions/preflight/deferred cleanup PASS\n";
    if(write(1,done,sizeof(done)-1)!=(int64_t)sizeof(done)-1)return 11;
    /* Leave mapping for process-stop cleanup: verifies full resident reclaim. */
    return 93;
}
