#include <stddef.h>
#include <stdint.h>
/* No imported functions and no initialized template bytes. */
_Thread_local uint64_t zero_only[1025] __attribute__((aligned(4096)));
uint64_t _start(void)
{
    uintptr_t actual=(uintptr_t)zero_only;__asm__ volatile("":"+r"(actual));
    if(actual&4095)return 1;
    for(unsigned i=0;i<1025;++i)if(zero_only[i])return 2;
    zero_only[1024]=0xabcdef;if(zero_only[1024]!=0xabcdef)return 3;
    const char text[]="native TLS zero-only/import-free PASS\n";
    register uint64_t a __asm__("rax")=1,d __asm__("rdi")=(uintptr_t)text,s __asm__("rsi")=sizeof(text)-1;
    __asm__ volatile("int $0x80":"+a"(a):"D"(d),"S"(s):"rdx","memory","cc");
    return 59;
}
