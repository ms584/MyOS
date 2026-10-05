#include <netdb.h>
#include <stdlib.h>
#include <stdbool.h>
#include "dns-call.h"
extern int *__errno_location(void);
/* Resident modules cannot depend on an external libc/string image. */
static void copy_bytes(void *out,const void *in,size_t n)
{unsigned char *d=out;const unsigned char *s=in;while(n--)*d++=*s++;}
static size_t string_size(const char *s)
{size_t n=0;while(s[n])++n;return n;}
static int compare_strings(const char *a,const char *b)
{while(*a&&*a==*b){++a;++b;}return (unsigned char)*a-(unsigned char)*b;}
#define memcpy copy_bytes
#define strlen string_size
#define strcmp compare_strings
struct address {uint16_t family;uint8_t port[2],ip[4],pad[8];};
struct result {struct addrinfo ai;struct address address;};
_Static_assert(sizeof(struct address)==16&&sizeof(struct result)==64,"pinned IPv4 result layout");
static int64_t call(uint64_t op,uint64_t first,uint64_t second)
{
    register uint64_t r __asm__("rax")=MYOS_DNS_CALL;
    __asm__ volatile("int $0x80":"+a"(r):"D"(op),"S"(first),"d"(second):"memory","cc");
    return (int64_t)r;
}
static int64_t wait(volatile uint64_t *sequence,uint64_t previous)
{
    register uint64_t r __asm__("rax")=20;
    __asm__ volatile("int $0x80":"+a"(r):"D"((uintptr_t)sequence),"S"(previous),"d"(UINT64_MAX):"memory","cc");
    return (int64_t)r;
}
static bool numeric(const char *s,uint8_t out[4])
{
    uint32_t parts[4];unsigned count=0;
    for(;;){
        if(count==4||*s<'0'||*s>'9')return false;
        unsigned base=*s=='0'?8:10;
        if(*s=='0'&&(s[1]=='x'||s[1]=='X')){base=16;s+=2;}
        uint32_t value=0;unsigned digits=0;
        for(;;){unsigned c=(unsigned char)*s,digit;
            if(c>='0'&&c<='9')digit=c-'0';
            else if(c>='a'&&c<='f')digit=c-'a'+10;
            else if(c>='A'&&c<='F')digit=c-'A'+10;
            else break;
            if(digit>=base)break;
            if(value>(UINT32_MAX-digit)/base)return false;
            value=value*base+digit;++digits;++s;
        }
        if(!digits)return false;parts[count++]=value;
        if(!*s)break;if(*s++!='.')return false;
    }
    if(parts[count-1]>(UINT32_MAX>>((count-1)*8)))return false;
    uint32_t value=parts[count-1];
    for(unsigned i=0;i+1<count;++i){if(parts[i]>255)return false;value|=parts[i]<<(24-i*8);}
    for(unsigned i=0;i<4;++i)out[i]=(uint8_t)(value>>(24-i*8));
    return true;
}
static int system_error(int64_t error)
{*__errno_location()=(int)-error;return EAI_SYSTEM;}
static int resolve(const char *host,struct dns_answer *answer)
{
    volatile uint64_t sequence=0;struct dns_start_request request={0};
    request.sequence=(uintptr_t)&sequence;
    size_t length=0;while(length<255&&host[length])++length;
    if(!length||length==255)return EAI_NONAME;
    memcpy(request.name,host,length+1);
    int64_t handle=call(DNS_CALL_START,(uintptr_t)&request,0);
    if(handle<0){if(handle==-101||handle==-11)return EAI_AGAIN;
        if(handle==-12)return EAI_MEMORY;if(handle==-22)return EAI_NONAME;
        return system_error(handle);}
    int result;
    for(;;){
        uint64_t previous=__atomic_load_n(&sequence,__ATOMIC_ACQUIRE);
        struct dns_job_status status;
        int64_t rc=call(DNS_CALL_STATUS,(uint32_t)handle,(uintptr_t)&status);
        if(!rc){result=status.error;if(!result)*answer=status.answer;break;}
        if(rc!=-11){result=system_error(rc);break;}
        rc=wait(&sequence,previous);
        if(rc&&rc!=-11){result=system_error(rc);break;}
    }
    (void)call(DNS_CALL_CLOSE,(uint32_t)handle,0);return result;
}
void freeaddrinfo(struct addrinfo *info)
{while(info){struct addrinfo *next=info->ai_next;free(info);info=next;}}
int getaddrinfo(const char *host,const char *service,const struct addrinfo *hints,struct addrinfo **output)
{
    if(!output)return system_error(-14);
    if(!host&&!service)return EAI_NONAME;
    int family=hints?hints->ai_family:0,flags=hints?hints->ai_flags:0;
    int type=hints?hints->ai_socktype:0,protocol=hints?hints->ai_protocol:0;
    if(flags&~(AI_PASSIVE|AI_CANONNAME|AI_NUMERICHOST|AI_V4MAPPED|AI_ALL|AI_ADDRCONFIG|AI_NUMERICSERV))return EAI_BADFLAGS;
    if(family&&family!=2)return EAI_FAMILY;
    if(type&&type!=1&&type!=2)return EAI_SOCKTYPE;
    if(protocol&&protocol!=6&&protocol!=17)return EAI_SERVICE;
    if((type==1&&protocol==17)||(type==2&&protocol==6))return EAI_SERVICE;
    unsigned port=0;bool named=false;
    if(service){if(!*service)return EAI_SERVICE;
        for(const char *s=service;*s;++s){if(*s<'0'||*s>'9'){named=true;break;}
            port=port*10+(unsigned)(*s-'0');if(port>65535)return EAI_SERVICE;}
        if(named){if(flags&AI_NUMERICSERV)return EAI_NONAME;
            if(!strcmp(service,"http"))port=80;
            else if(!strcmp(service,"https"))port=443;
            else if(!strcmp(service,"domain"))port=53;
            else return EAI_SERVICE;
            if(port!=53&&(type==2||protocol==17))return EAI_SERVICE;
            if(port!=53){type=1;protocol=6;}}
    }
    if((flags&AI_ADDRCONFIG)&&call(DNS_CALL_CONFIGURED,0,0)!=1)return EAI_NODATA;
    struct dns_answer answer={0};int rc=0;
    if(!host){answer.count=1;if(!(flags&AI_PASSIVE)){answer.addresses[0][0]=127;answer.addresses[0][3]=1;}}
    else if(numeric(host,answer.addresses[0])||!strcmp(host,"localhost")){
        if(!strcmp(host,"localhost")){answer.addresses[0][0]=127;answer.addresses[0][3]=1;}
        answer.count=1;size_t length=strlen(host);
        if(length>=sizeof(answer.canonical))return EAI_NONAME;
        memcpy(answer.canonical,host,length+1);
    }else{
        if(flags&AI_NUMERICHOST)return EAI_NONAME;
        rc=resolve(host,&answer);if(rc)return rc;
    }
    struct addrinfo *head=0,**tail=&head;
    bool stream=type==1||protocol==6,datagram=type==2||protocol==17;
    if(!type&&!protocol)stream=datagram=true;
    for(unsigned i=0;i<answer.count;++i)for(unsigned kind=1;kind<=2;++kind){
        if((kind==1&&!stream)||(kind==2&&!datagram))continue;
        size_t canonical_size=!head&&(flags&AI_CANONNAME)&&host?strlen(answer.canonical)+1:0;
        struct result *r=calloc(1,sizeof(*r)+canonical_size);
        if(!r){freeaddrinfo(head);return EAI_MEMORY;}
        r->ai.ai_family=2;r->ai.ai_socktype=(int)kind;r->ai.ai_protocol=kind==1?6:17;
        r->ai.ai_addrlen=sizeof(r->address);r->ai.ai_addr=(struct sockaddr *)&r->address;
        r->address.family=2;r->address.port[0]=(uint8_t)(port>>8);r->address.port[1]=(uint8_t)port;
        memcpy(r->address.ip,answer.addresses[i],4);
        if(canonical_size){r->ai.ai_canonname=(char *)(r+1);memcpy(r+1,answer.canonical,canonical_size);}
        *tail=&r->ai;tail=&r->ai.ai_next;
    }
    *output=head;return 0;
}
const char *gai_strerror(int error)
{
    switch(error){
    case 0:return "Success";case EAI_BADFLAGS:return "Invalid flags";
    case EAI_NONAME:return "Name does not resolve";case EAI_AGAIN:return "Try again";
    case EAI_FAIL:return "Non-recoverable failure";case EAI_NODATA:return "Name has no usable address";
    case EAI_FAMILY:return "Unrecognized address family";case EAI_SOCKTYPE:return "Unrecognized socket type";
    case EAI_SERVICE:return "Unrecognized service";case EAI_MEMORY:return "Out of memory";
    case EAI_SYSTEM:return "System error";case EAI_OVERFLOW:return "Result too large";
    default:return "Unknown error";}
}
