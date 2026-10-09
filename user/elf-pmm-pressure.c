#include <stddef.h>
#include <stdint.h>
extern void *mmap(void *,size_t,int,int,int,int64_t);
extern int munmap(void *,size_t);
static void message(const char *s){
 size_t n=0;while(s[n])++n;
 register uint64_t a __asm__("rax")=1,d __asm__("rdi")=(uintptr_t)s,v __asm__("rsi")=n;
 __asm__ volatile("int $0x80":"+a"(a):"D"(d),"S"(v):"rdx","memory","cc");
}
int _start(void){
 size_t size=16*1024*1024;
 volatile unsigned char *p=mmap(0,size,3,0x22,-1,0);
 if(p==(void *)-1)__builtin_trap();
 for(size_t i=0;i<size;i+=4096)p[i]=1;
 if(munmap((void *)p,size))__builtin_trap();
 message("native transient 16MiB allocation/free PASS\n");
 return 0;
}
