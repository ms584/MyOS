#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <wchar.h>
#include <myos/stdio_impl.h>
static void message(const char *text,size_t length)
{
    register uint64_t a __asm__("rax")=1,d __asm__("rdi")=(uintptr_t)text;
    register uint64_t s __asm__("rsi")=length;
    __asm__ volatile("int $0x80":"+a"(a):"D"(d),"S"(s):"rdx","memory","cc");
}
#define CHECK(x) do{if(!(x)){const char t[]="format failed: " #x "\n";message(t,sizeof(t)-1);__builtin_trap();}}while(0)
static char captured[4096];static size_t written;
static size_t capture(FILE *stream,const unsigned char *data,size_t length)
{
    size_t pending=(size_t)(stream->wpos-stream->wbase);
    CHECK(written+pending+length<sizeof(captured));
    memcpy(captured+written,stream->wbase,pending);written+=pending;
    memcpy(captured+written,data,length);written+=length;captured[written]=0;
    stream->wpos=stream->wbase=stream->buf;return length;
}
static int formatted(FILE *stream,const char *format,...)
{va_list args;va_start(args,format);int result=vfprintf(stream,format,args);va_end(args);return result;}
int _start(void)
{
    char text[256];
    CHECK(snprintf(text,sizeof(text),"%ld/%lu",INT64_MIN,UINT64_MAX)==41);
    CHECK(!strcmp(text,"-9223372036854775808/18446744073709551615"));
    CHECK(snprintf(text,sizeof(text),"%08x %.3f %.3Lf %a",0x12,1.25,1.25L,1.5)==sizeof("00000012 1.250 1.250 0x1.8p+0")-1);
    CHECK(!strcmp(text,"00000012 1.250 1.250 0x1.8p+0"));
    CHECK(snprintf(text,sizeof(text),"%g/%g/%g",__builtin_inf(),-__builtin_inf(),__builtin_nan(""))==sizeof("inf/-inf/nan")-1);
    CHECK(!strcmp(text,"inf/-inf/nan"));
    CHECK(snprintf(text,sizeof(text),"%.2Lf",-0.0L)==5&&!strcmp(text,"-0.00"));
    CHECK(snprintf(text,sizeof(text),"%2$d:%1$d",7,9)==3&&!strcmp(text,"9:7"));
    CHECK(snprintf(0,0,"abcdef")==6);
    char guard[5]={'X','X','X','X','X'};
    CHECK(snprintf(guard+1,3,"abcdef")==6);
    CHECK(guard[0]=='X'&&guard[1]=='a'&&guard[2]=='b'&&!guard[3]&&guard[4]=='X');
    CHECK(snprintf(guard+1,1,"abcdef")==6&&!guard[1]);
    errno=EINVAL;CHECK(snprintf(text,sizeof(text),"%m")==16&&!strcmp(text,"Invalid argument"));
    wchar_t wide[]={0xe44,0xe17,0x1f600,0};
    CHECK(snprintf(text,sizeof(text),"%ls",wide)==10);
    CHECK(!strcmp(text,"\xe0\xb9\x84\xe0\xb8\x97\xf0\x9f\x98\x80"));
    wchar_t invalid[]={0xd800,0};CHECK(snprintf(text,sizeof(text),"%ls",invalid)==-1&&errno==EILSEQ);
    char *allocated=0;CHECK(asprintf(&allocated,"%s:%d","allocated",71)==12);
    CHECK(allocated&&!strcmp(allocated,"allocated:71"));free(allocated);
    CHECK(strnlen("abc",0)==0&&strnlen("abc",2)==2);
    CHECK(strcmp("\xff","\x01")>0);
    unsigned char buffer[8];FILE stream={.buf=buffer,.buf_size=sizeof(buffer),.write=capture,.lock=-1,.lbf=-1};
    CHECK(fwrite("0123456789abcdef",4,4,&stream)==4&&written==16);
    CHECK(!strcmp(captured,"0123456789abcdef"));
    CHECK(fwrite("x",SIZE_MAX,2,&stream)==0&&errno==EOVERFLOW&&written==16);
    written=0;stream.buf_size=0;stream.wpos=stream.wbase=stream.wend=0;stream.lock=0;
    CHECK(formatted(&stream,"%s:%d","callback",19)==11);
    CHECK(!strcmp(captured,"callback:19")&&written==11);
    CHECK(!pthread_mutex_destroy(&stream.mutex));
    const char passed[]="native sealed formatter/UTF8/callback/overflow PASS\n";
    message(passed,sizeof(passed)-1);return 79;
}
