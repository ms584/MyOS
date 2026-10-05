#include <stdint.h>
#include <stddef.h>
extern void SbLogRaw(const char *);
int _start(void)
{
    const volatile unsigned char *raw=(void *)0x803f000000ull;
    uint64_t hash=14695981039346656037ull;
    for(size_t i=0;i<RAW_SIZE;++i)hash=(hash^raw[i])*1099511628211ull;
    if(hash!=RAW_HASH)__builtin_trap();
    SbLogRaw("mapped runtime exact bytes PASS\n");
#ifdef WRITE_RX
    *(volatile unsigned char *)raw=0;
    __builtin_trap();
#endif
    return 88;
}
