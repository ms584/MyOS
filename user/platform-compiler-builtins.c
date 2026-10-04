/* Freestanding GCC x86_64 software popcount ABI; no host libgcc dependency. */
#include <stdint.h>
_Static_assert(sizeof(unsigned long long)==8,"GCC DI argument width");
int __popcountdi2(unsigned long long argument)
{
    uint64_t value=argument;
    value-=((value>>1)&UINT64_C(0x5555555555555555));
    value=(value&UINT64_C(0x3333333333333333))+
          ((value>>2)&UINT64_C(0x3333333333333333));
    value=(value+(value>>4))&UINT64_C(0x0f0f0f0f0f0f0f0f);
    return (int)((value*UINT64_C(0x0101010101010101))>>56);
}
