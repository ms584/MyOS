/* Target SDK primitives used by Mesa thread naming; not a complete libc. */
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
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
void *memmove(void *destination,const void *source,size_t length)
{
    unsigned char *out=destination;const unsigned char *in=source;
    if((uintptr_t)out<(uintptr_t)in){
        for(size_t i=0;i<length;++i)out[i]=in[i];
    }else {
        while(length){--length;out[length]=in[length];}
    }
    return destination;
}
int memcmp(const void *first,const void *second,size_t length)
{
    const unsigned char *a=first,*b=second;
    for(size_t i=0;i<length;++i)if(a[i]!=b[i])return (int)a[i]-(int)b[i];
    return 0;
}
char *strchr(const char *text,int value)
{
    unsigned char byte=(unsigned char)value;
    for(;;++text){
        if((unsigned char)*text==byte)return (char *)text;
        if(!*text)return 0;
    }
}
char *strncpy(char *destination,const char *source,size_t length)
{
    size_t i=0;
    while(i<length&&source[i]){destination[i]=source[i];++i;}
    while(i<length)destination[i++]=0;
    return destination;
}
char *strstr(const char *text,const char *needle)
{
    if(!*needle)return (char *)text;
    for(;*text;++text){
        size_t i=0;while(needle[i]&&text[i]&&needle[i]==text[i])++i;
        if(!needle[i])return (char *)text;
    }
    return 0;
}
char *strdup(const char *text)
{
    size_t length=strlen(text);
    if(length==SIZE_MAX){errno=ENOMEM;return 0;}
    char *copy=malloc(length+1);
    if(copy)memcpy(copy,text,length+1);
    return copy;
}
