#ifndef MYOS_APP_LIB_H
#define MYOS_APP_LIB_H
#include <stdint.h>
/* Freestanding MyOS ABI; no Linux C runtime or FPU state. */
static inline int64_t myos_call(uint64_t number,uint64_t first,uint64_t second)
{
    register uint64_t a __asm__("rax")=number;
    register uint64_t d __asm__("rdi")=first;
    register uint64_t s __asm__("rsi")=second;
    __asm__ volatile("int $0x80":"+a"(a):"D"(d),"S"(s):"memory","cc");
    return (int64_t)a;
}
static inline int64_t myos_write(const char *s,uint64_t size){return myos_call(1,(uint64_t)s,size);}
static inline uint64_t myos_ticks(void){return (uint64_t)myos_call(13,0,0);}
#define MYOS_HEAP_BASE 0x8000020000ull
#define MYOS_HEAP_BYTES (128u*1024u)
static inline __attribute__((noreturn)) void myos_exit(uint64_t code)
{
    myos_call(3,code,0);
    for(;;)__asm__ volatile("pause");
}
#endif
