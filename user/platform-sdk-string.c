/* Target SDK primitives used by Mesa thread naming; not a complete libc. */
#include <string.h>
void *memcpy(void *destination,const void *source,size_t length)
{
    unsigned char *out=destination;const unsigned char *in=source;
    for(size_t i=0;i<length;++i)out[i]=in[i];
    return destination;
}
size_t strlen(const char *text)
{
    size_t length=0;while(text[length])++length;return length;
}
