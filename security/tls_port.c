#include "os.h"
#include "security.h"
#include <time.h>
#include <stdarg.h>
#include "mbedtls/platform.h"
#include "mbedtls/platform_util.h"

void *calloc(size_t count,size_t width){
    if(width && count>((size_t)-1)/width)return 0;
    size_t bytes=count*width;
    void *p=kmalloc(bytes?bytes:1);
    if(p)memset(p,0,bytes?bytes:1);
    return p;
}
void free(void *p){kfree(p);}
int memcmp(const void *a,const void *b,size_t n){
    const unsigned char *x=a,*y=b;
    for(size_t i=0;i<n;++i)if(x[i]!=y[i])return (int)x[i]-(int)y[i];
    return 0;
}
size_t strlen(const char *s){size_t n=0;while(s[n])++n;return n;}
char *strchr(const char *s,int value){
    for(;;++s){if(*s==(char)value)return (char *)s;if(!*s)return 0;}
}
void *memmove(void *dst,const void *src,size_t size){
    unsigned char *d=dst;const unsigned char *s=src;
    if(d<s)for(size_t i=0;i<size;++i)d[i]=s[i];
    else if(d>s)while(size--)d[size]=s[size];
    return dst;
}
int inet_pton(int family,const char *text,void *out){
    if(family!=2||!text||!out)return family==2?0:-1;
    unsigned char parts[4];
    for(unsigned i=0;i<4;++i){
        unsigned number=0,digits=0;
        while(*text>='0'&&*text<='9'&&digits<3){number=number*10+(*text++-'0');++digits;}
        if(!digits||number>255)return 0;
        parts[i]=(unsigned char)number;
        if(i<3){if(*text++!='.')return 0;}
    }
    if(*text)return 0;
    memcpy(out,parts,4);return 1;
}
static int myos_snprintf(char *out,size_t capacity,const char *format,...){
    va_list args;va_start(args,format);size_t size=0;
    for(const char *p=format;*p;++p){
        if(*p!='%'){
            if(capacity&&size<capacity-1)out[size]=*p;
            ++size;continue;
        }
        ++p;if(!*p)break;
        if(*p=='%'){if(capacity&&size<capacity-1)out[size]='%';++size;continue;}
        if(*p=='c'){
            char c=(char)va_arg(args,int);
            if(capacity&&size<capacity-1)out[size]=c;
            ++size;continue;
        }
        if(*p=='u'||*p=='d'){
            unsigned value=*p=='u'?va_arg(args,unsigned):(unsigned)va_arg(args,int);
            char digits[12];unsigned count=0;
            do{digits[count++]=(char)('0'+value%10);value/=10;}while(value&&count<sizeof(digits));
            while(count){char c=digits[--count];if(capacity&&size<capacity-1)out[size]=c;++size;}
            continue;
        }
        va_end(args);if(capacity)out[size<capacity?size:capacity-1]=0;return -1;
    }
    va_end(args);if(capacity)out[size<capacity?size:capacity-1]=0;
    return (int)size;
}
void tls_port_init(void){(void)mbedtls_platform_set_snprintf(myos_snprintf);}
time_t time(time_t *out){time_t t=(time_t)security_unix_time();if(out)*out=t;return t;}
void mbedtls_platform_zeroize(void *p,size_t n){volatile unsigned char *b=p;while(n--)*b++=0;}
int mbedtls_hardware_poll(void *ctx,unsigned char *out,size_t len,size_t *olen){
    (void)ctx;*olen=0;
    if(!security_random(out,len))return -1;
    *olen=len;return 0;
}

static bool leap(int year){return year%4==0&&(year%100!=0||year%400==0);}
struct tm *mbedtls_platform_gmtime_r(const mbedtls_time_t *value,struct tm *out){
    if(!value||!out||*value<0)return 0;
    uint64_t stamp=(uint64_t)*value,days=stamp/86400u;
    memset(out,0,sizeof(*out));
    out->tm_sec=(int)(stamp%60u);out->tm_min=(int)(stamp/60u%60u);
    out->tm_hour=(int)(stamp/3600u%24u);
    out->tm_wday=(int)((days+4u)%7u);
    int year=1970;
    while(days>=(uint64_t)(leap(year)?366:365))days-=leap(year++)?366u:365u;
    out->tm_year=year-1900;out->tm_yday=(int)days;
    static const int mdays[12]={31,28,31,30,31,30,31,31,30,31,30,31};
    int month=0;
    while(month<11){int span=mdays[month]+(month==1&&leap(year));
        if(days<(uint64_t)span)break;days-=span;++month;}
    out->tm_mon=month;out->tm_mday=(int)days+1;
    return out;
}
