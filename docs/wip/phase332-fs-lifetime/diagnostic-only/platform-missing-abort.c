#include <stdint.h>
#include <stddef.h>
static void diagnostic_write(const char *s)
{
    size_t n=0;while(s[n]&&n<256)++n;
    register uint64_t a __asm__("rax")=1,extra __asm__("r10")=0;
    __asm__ volatile("int $0x80":"+a"(a):"D"((uintptr_t)s),"S"(n),"d"(UINT64_C(0)),"r"(extra):"memory","cc");
}
_Noreturn void MyOSMissingAbort(const char *name)
{
    diagnostic_write("DIAGNOSTIC ONLY unsupported API called: ");
    diagnostic_write(name);diagnostic_write("\n");
    register uint64_t a __asm__("rax")=3,extra __asm__("r10")=0;
    __asm__ volatile("int $0x80":"+a"(a):"D"(UINT64_C(99)),"S"(UINT64_C(0)),"d"(UINT64_C(0)),"r"(extra):"memory","cc");
    for(;;)__asm__ volatile("pause");
}
