#include "dns-wire.h"
#include <stdbool.h>
/* Kernel provides these primitives; do not inherit host libc fortify wrappers. */
extern void *memcpy(void *,const void *,size_t);
extern int memcmp(const void *,const void *,size_t);
static uint16_t read16(const uint8_t *p){return (uint16_t)((unsigned)p[0]<<8|p[1]);}
static void write16(uint8_t *p,uint16_t n){p[0]=(uint8_t)(n>>8);p[1]=(uint8_t)n;}
static unsigned fold(unsigned c){return c>='A'&&c<='Z'?c+32:c;}
int dns_make_query(const char *host,uint16_t id,uint8_t *out,size_t capacity)
{
    if(!host||!out||capacity<17)return -2;
    size_t n=0;while(n<255&&host[n])++n;
    if(!n||n==255)return -2;
    if(host[n-1]=='.')--n;
    uint8_t query[DNS_QUERY_MAX]={0};size_t at=12,start=0;
    for(size_t i=0;i<=n;++i)if(i==n||host[i]=='.'){
        size_t length=i-start;
        if(!length){if(!n)break;return -2;}
        if(length>63||at+length+1>266)return -2;
        query[at++]=(uint8_t)length;
        for(size_t j=start;j<i;++j)query[at++]=(uint8_t)fold((unsigned char)host[j]);
        start=i+1;
    }
    if(at+5>capacity||at+5>sizeof(query))return -12;
    query[at++]=0;write16(query+at,1);at+=2;write16(query+at,1);at+=2;
    write16(query,id);write16(query+2,0x0100);write16(query+4,1);
    memcpy(out,query,at);return (int)at;
}
/* Expand wire names, retaining label lengths rather than interpreting NULs or
 * dots inside labels. Compression targets must refer backward; hop/output
 * bounds also reject loops through a preceding label and pointer. */
static bool expand(const uint8_t *p,size_t size,size_t *cursor,uint8_t name[255])
{
    size_t at=*cursor,used=0,end=0;unsigned hops=0;
    while(at<size&&++hops<=255){
        unsigned length=p[at++];
        if((length&0xc0)==0xc0){
            if(at>=size)return false;
            size_t target=((length&63u)<<8)|p[at++];
            if(target<12||target>=at-2)return false;
            if(!end)end=at;at=target;continue;
        }
        if(length&0xc0||length>size-at||used+length+1>255)return false;
        name[used++]=(uint8_t)length;
        if(!length){*cursor=end?end:at;return true;}
        for(unsigned i=0;i<length;++i)name[used++]=(uint8_t)fold(p[at++]);
    }
    return false;
}
static size_t name_size(const uint8_t *p)
{size_t at=0;while(at<255){unsigned n=p[at++];if(!n)return at;at+=n;}return 0;}
static bool equal(const uint8_t *a,const uint8_t *b)
{size_t n=name_size(a);return n&&n==name_size(b)&&!memcmp(a,b,n);}
static bool canonical(const uint8_t *name,char out[254])
{
    size_t at=0,used=0;
    while(name[at]){
        unsigned length=name[at++];if(used&&used>=253)return false;
        if(used)out[used++]='.';
        if(length>253-used)return false;
        for(unsigned i=0;i<length;++i){unsigned c=name[at++];
            if(!c||c=='.')return false;out[used++]=(char)c;}
    }
    out[used]=0;return true;
}
int dns_parse_reply(const uint8_t *p,size_t size,const uint8_t *query,size_t qsize,
                    struct dns_answer *answer)
{
    if(!p||!query||!answer||size<12||size>DNS_REPLY_MAX||qsize<17||qsize>DNS_QUERY_MAX)return -4;
    unsigned flags=read16(p+2);
    if(read16(p)!=read16(query)||!(flags&0x8000)||(flags&0x7800)||read16(p+4)!=1)return -4;
    uint8_t expected[255],name[255];size_t q=12,at=12;
    if(!expand(query,qsize,&q,expected)||q+4!=qsize||read16(query+q)!=1||read16(query+q+2)!=1||
       !expand(p,size,&at,name)||!equal(expected,name)||at+4>size||
       read16(p+at)!=1||read16(p+at+2)!=1)return -4;
    at+=4;size_t begin=at;
    if(flags&0x0200)return -3;
    unsigned answers=read16(p+6),records=answers+(unsigned)read16(p+8)+read16(p+10);
    if(records>64)return -12;
    /* Validate all sections before honoring RCODE or an otherwise valid A. */
    for(unsigned i=0;i<records;++i){
        if(!expand(p,size,&at,name)||at+10>size)return -4;
        unsigned type=read16(p+at),klass=read16(p+at+2),length=read16(p+at+8);at+=10;
        if(length>size-at)return -4;
        if(type==5&&klass==1){size_t end=at;
            if(!expand(p,size,&end,name)||end!=at+length)return -4;}
        if(type==1&&klass==1&&length!=4)return -4;
        at+=length;
    }
    if(at!=size)return -4;
    if((flags&15)==3)return -2;
    if((flags&15)==2)return -3;
    if(flags&15)return -4;
    struct dns_answer result={0};
    for(unsigned depth=0;depth<9;++depth){
        bool alias=false;uint8_t next[255];at=begin;
        for(unsigned i=0;i<answers;++i){
            if(!expand(p,size,&at,name))return -4;
            unsigned type=read16(p+at),klass=read16(p+at+2),length=read16(p+at+8);at+=10;
            if(klass==1&&equal(expected,name)){
                if(type==5){size_t end=at;uint8_t target[255];
                    if(!expand(p,size,&end,target)||equal(expected,target))return -4;
                    if(alias&&!equal(next,target))return -4;
                    memcpy(next,target,name_size(target));alias=true;
                }else if(type==1){
                    bool duplicate=false;
                    for(unsigned j=0;j<result.count;++j)
                        if(!memcmp(result.addresses[j],p+at,4))duplicate=true;
                    if(!duplicate){if(result.count==DNS_ADDRESS_LIMIT)return -12;
                        memcpy(result.addresses[result.count++],p+at,4);}
                }
            }
            at+=length;
        }
        if(alias){if(result.count)return -4;if(depth==8)return -4;
            memcpy(expected,next,name_size(next));continue;}
        if(!result.count)return -5;
        if(!canonical(expected,result.canonical))return -4;
        *answer=result;return 0;
    }
    return -4;
}
