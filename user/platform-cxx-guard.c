/* x64 Itanium guard ABI; private MyOS runtime identity, not a libc TCB.
 * The first byte is initialization-complete. Remaining bits are private.
 * Valid compiler-generated guard storage must be writable and 8-byte aligned.
 */
#include <stdint.h>
#include <stddef.h>
#ifdef MYOS_CXX_GUARD_HOST_TEST
extern uintptr_t guard_test_identity(void);
extern int64_t guard_test_call(uint64_t, uint64_t, uint64_t, uint64_t);
#define identity guard_test_identity
#define call3 guard_test_call
#else
extern uintptr_t pthread_self(void);
#define identity pthread_self
static int64_t call3(uint64_t number,uint64_t first,uint64_t second,uint64_t third)
{
    register uint64_t a __asm__("rax")=number,d __asm__("rdi")=first;
    register uint64_t s __asm__("rsi")=second,x __asm__("rdx")=third;
    __asm__ volatile("int $0x80":"+a"(a):"D"(d),"S"(s),"d"(x):"memory","cc");
    return (int64_t)a;
}
#endif
static uint64_t owned(void)
{
    uintptr_t owner=identity();
    /* Current private pthread handles fit 48 bits; do not silently truncate. */
    if(!owner || owner>>48)__builtin_trap();
    return ((uint64_t)owner<<16)|256;
}
int __cxa_guard_acquire(uint64_t *guard)
{
    uint64_t mine=owned();
    for(;;){
        uint64_t observed=__atomic_load_n(guard,__ATOMIC_ACQUIRE);
        if(observed&255)return 0;
        if(!observed){
            if(__atomic_compare_exchange_n(guard,&observed,mine,0,
                                          __ATOMIC_ACQUIRE,__ATOMIC_RELAXED))return 1;
            continue;
        }
        /* Recursive initialization has undefined C++ behavior. Contain it
         * as a process fault, never deadlock or pretend construction finished. */
        if(observed==mine)__builtin_trap();
        int64_t rc=call3(20,(uintptr_t)guard,observed,0);
        if(rc && rc!=-11)__builtin_trap();
    }
}
static void finish(uint64_t *guard,uint64_t value)
{
    if(__atomic_load_n(guard,__ATOMIC_RELAXED)!=owned())__builtin_trap();
    __atomic_store_n(guard,value,__ATOMIC_RELEASE);
    (void)call3(21,(uintptr_t)guard,UINT32_MAX,0);
}
void __cxa_guard_release(uint64_t *guard){finish(guard,1);}
void __cxa_guard_abort(uint64_t *guard){finish(guard,0);}
