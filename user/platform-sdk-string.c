/* Target SDK primitives used by Mesa thread naming; not a complete libc. */
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <strings.h>
#include <ctype.h>
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
int strncmp(const char *first,const char *second,size_t bound)
{
    const unsigned char *a=(const unsigned char *)first,*b=(const unsigned char *)second;
    for(size_t i=0;i<bound;++i){
        if(a[i]!=b[i])return (int)a[i]-(int)b[i];
        if(!a[i])break;
    }
    return 0;
}
char *strcpy(char *destination,const char *source)
{
    size_t i=0;do{destination[i]=source[i];}while(source[i++]);return destination;
}
char *strcat(char *destination,const char *source)
{strcpy(destination+strlen(destination),source);return destination;}
char *strncat(char *destination,const char *source,size_t bound)
{
    char *out=destination+strlen(destination);size_t i=0;
    while(i<bound&&source[i]){out[i]=source[i];++i;}out[i]=0;return destination;
}
char *strchrnul(const char *text,int value)
{
    unsigned char byte=(unsigned char)value;
    while(*text&&(unsigned char)*text!=byte)++text;
    return (char *)text;
}
char *strrchr(const char *text,int value)
{
    char *last=0;unsigned char byte=(unsigned char)value;
    do{if((unsigned char)*text==byte)last=(char *)text;}while(*text++);
    return last;
}
size_t strspn(const char *text,const char *accept)
{size_t n=0;while(text[n]&&strchr(accept,(unsigned char)text[n]))++n;return n;}
size_t strcspn(const char *text,const char *reject)
{size_t n=0;while(text[n]&&!strchr(reject,(unsigned char)text[n]))++n;return n;}
char *strpbrk(const char *text,const char *accept)
{size_t n=strcspn(text,accept);return text[n]?(char *)text+n:0;}
char *strtok_r(char *text,const char *delimiters,char **saved)
{
    if(!text)text=*saved;
    if(!text)return 0;
    text+=strspn(text,delimiters);
    if(!*text){*saved=text;return 0;}
    char *end=text+strcspn(text,delimiters);
    if(*end)*end++=0;
    *saved=end;return text;
}
/* ISO C strtok owns one shared cursor; concurrent callers use strtok_r. */
char *strtok(char *text,const char *delimiters)
{static char *saved;return strtok_r(text,delimiters,&saved);}
char *strndup(const char *text,size_t bound)
{
    size_t length=strnlen(text,bound);
    if(length==SIZE_MAX){errno=ENOMEM;return 0;}
    char *copy=malloc(length+1);
    if(copy){memcpy(copy,text,length);copy[length]=0;}return copy;
}
int strncasecmp(const char *first,const char *second,size_t bound)
{
    for(size_t i=0;i<bound;++i){
        int a=tolower((unsigned char)first[i]),b=tolower((unsigned char)second[i]);
        if(a!=b)return a-b;
        if(!first[i])break;
    }
    return 0;
}
int strcasecmp(const char *first,const char *second)
{return strncasecmp(first,second,SIZE_MAX);}
