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
size_t strnlen(const char *text,size_t bound)
{size_t length=0;while(length<bound&&text[length])++length;return length;}
int strcmp(const char *first,const char *second)
{
    const unsigned char *a=(const unsigned char *)first,*b=(const unsigned char *)second;
    while(*a&&*a==*b){++a;++b;}return (int)*a-(int)*b;
}
void *memset(void *destination,int value,size_t length)
{
    unsigned char *out=destination;
    for(size_t i=0;i<length;++i)out[i]=(unsigned char)value;
    return destination;
}
void *memchr(const void *source,int value,size_t length)
{
    const unsigned char *bytes=source;
    for(size_t i=0;i<length;++i)
        if(bytes[i]==(unsigned char)value)return (void *)(bytes+i);
    return 0;
}
