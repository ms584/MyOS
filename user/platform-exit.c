/* Private MyOS single-image bootstrap ABI; not a foreign libc exit hook. */
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
extern void __cxa_finalize(void *);
#ifdef MYOS_PAIRED_IMAGE
/* Required by the paired loader; old providers must not silently reuse the
 * incompatible single-image bootstrap finalizer frame. */
const uint64_t MyOSPairedExitAbi=1;
extern void MyOSFinalizeImages(void);
#endif
_Noreturn void _Exit(int status)
{
    register uint64_t a __asm__("rax")=3,d __asm__("rdi")=(uint64_t)(int64_t)status;
    __asm__ volatile("int $0x80":"+a"(a):"D"(d):"rsi","rdx","memory","cc");
    __builtin_trap();
}
_Noreturn void exit(int status)
{
#ifdef MYOS_PAIRED_IMAGE
    MyOSFinalizeImages();
#else
    __cxa_finalize(0);
    volatile uint64_t *frame=(volatile uint64_t *)(uintptr_t)
        *(const uint64_t *)(uintptr_t)(0x803f010000ull+56);
    if(!frame)__builtin_trap(); /* Only supported with the owned ELF bootstrap. */
    /* Stay on the caller's stack, including worker callers. Never resume or
     * overwrite a concurrently sleeping main thread's bootstrap stack. */
    while(frame[0]){
        uint64_t index=frame[0]-1;frame[0]=index;
        void (**array)(void)=(void (**)(void))(uintptr_t)frame[6];
        array[index]();
    }
    void (*fini)(void)=(void (*)(void))(uintptr_t)frame[5];
    frame[5]=0;if(fini)fini();
#endif
    (void)fflush(0);
    _Exit(status); /* Stops every thread; no wait for blocked workers. */
}
