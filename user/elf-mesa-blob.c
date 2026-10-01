#include "util/blob.h"
#include <string.h>
static void message(const char *text,size_t length)
{
    register uint64_t a __asm__("rax")=1,d __asm__("rdi")=(uintptr_t)text;
    register uint64_t s __asm__("rsi")=length;
    __asm__ volatile("int $0x80":"+a"(a):"D"(d),"S"(s):"rdx","memory","cc");
}
static _Noreturn void failed(const char *expression)
{
    const char prefix[]="Mesa blob check failed: ";message(prefix,sizeof(prefix)-1);
    message(expression,strlen(expression));message("\n",1);__builtin_trap();
}
#define CHECK(x) do { if(!(x))failed(#x); } while(0)
static void *pressure[2048];
static unsigned exhaust(void)
{
    unsigned used=0;
    while(used<128){void *p=malloc(1024*1024);if(!p)break;pressure[used++]=p;}
    CHECK(used>0&&used<128);
    while(used<2048){void *p=malloc(4096);if(!p)break;pressure[used++]=p;}
    while(used<2048){void *p=malloc(1);if(!p)break;pressure[used++]=p;}
    CHECK(used<2048);return used;
}
int _start(void)
{
    unsigned char search[]={0x44,0x22,0x22,0xff,0x55};
    CHECK(memchr(search,0x122,sizeof(search))==search+1);
    CHECK(memchr(search,-1,sizeof(search))==search+3);
    CHECK(!memchr(search,0x55,4)&&!memchr(search,0x22,0));
    struct blob b;struct blob_reader r;
    blob_init(&b);
    CHECK(blob_write_uint8(&b,0x81));
    CHECK(blob_write_uint32(&b,0x12345678));
    CHECK(blob_write_uint64(&b,0xdeadbeef01234567ull));
    CHECK(blob_write_string(&b,"MyOS shader cache"));
    for(unsigned i=0;i<6000;++i)CHECK(blob_write_uint8(&b,(uint8_t)i));
    CHECK(b.allocated>=8192&&!b.out_of_memory);
    blob_reader_init(&r,b.data,b.size);
    CHECK(blob_read_uint8(&r)==0x81);
    CHECK(blob_read_uint32(&r)==0x12345678);
    CHECK(blob_read_uint64(&r)==0xdeadbeef01234567ull);
    char *text=blob_read_string(&r);CHECK(text&&strlen(text)==17);
    for(unsigned i=0;i<6000;++i)CHECK(blob_read_uint8(&r)==(uint8_t)i);
    CHECK(r.current==r.end&&!r.overrun);
    CHECK(!blob_read_string(&r)&&r.overrun);
    size_t size=b.size;void *old=b.data;
    CHECK(blob_reserve_bytes(&b,SIZE_MAX)==-1&&b.out_of_memory);
    CHECK(b.size==size&&b.data==old);
    blob_finish(&b);
    unsigned char fixed[12];memset(fixed,0x55,sizeof(fixed));
    blob_init_fixed(&b,fixed+2,8);
    CHECK(blob_write_uint64(&b,0x12345678));
    CHECK(!blob_write_uint8(&b,1)&&b.out_of_memory&&b.size==8);
    CHECK(fixed[0]==0x55&&fixed[1]==0x55&&fixed[10]==0x55&&fixed[11]==0x55);
    blob_finish(&b);
    unsigned char bad[]={1,2,3};blob_reader_init(&r,bad,sizeof(bad));
    CHECK(!blob_read_string(&r)&&r.overrun);
    blob_init(&b);CHECK(blob_write_uint8(&b,0x76));old=b.data;
    unsigned count=exhaust();
    CHECK(blob_reserve_bytes(&b,4096)==-1&&b.data==old&&b.size==1);
    void *buffer=0;size=0;blob_finish_get_buffer(&b,&buffer,&size);
    CHECK(buffer==old&&size==1&&((unsigned char *)buffer)[0]==0x76);
    free(buffer);for(unsigned i=0;i<count;++i)free(pressure[i]);
    blob_finish(&b);
    const char passed[]="native Mesa blob growth/strings/bounds/OOM PASS\n";
    message(passed,sizeof(passed)-1);
    return 74;
}
