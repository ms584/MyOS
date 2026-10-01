#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include "util/u_debug.h"
static_assert(sizeof(gid_t)==4,"guest identity ABI");
static void message(const char *text,size_t length)
{
    register uint64_t a __asm__("rax")=1,d __asm__("rdi")=(uintptr_t)text;
    register uint64_t s __asm__("rsi")=length;
    __asm__ volatile("int $0x80":"+a"(a):"D"(d),"S"(s):"rdx","memory","cc");
}
#define CHECK(x) do{if(!(x)){const char t[]="environment failed: " #x "\n";message(t,sizeof(t)-1);__builtin_trap();}}while(0)
static int same(const char *a,const char *b)
{if(!a||!b)return 0;while(*a&&*a==*b){++a;++b;}return *a==*b;}
static unsigned ready;
static pthread_mutex_t gate=PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t start;
static int go;
static void *worker(void *raw)
{
    unsigned id=(unsigned)(uintptr_t)raw;
    char key[]="worker0";key[6]=(char)('0'+id);
    CHECK(!pthread_mutex_lock(&gate));++ready;
    while(!go)CHECK(!pthread_cond_wait(&start,&gate));
    CHECK(!pthread_mutex_unlock(&gate));
    for(unsigned i=0;i<40;++i){
        char value[]="000";value[0]=(char)('0'+id);value[1]=(char)('0'+i/10);value[2]=(char)('0'+i%10);
        CHECK(!setenv(key,value,1)&&same(getenv(key),value));
        CHECK(!setenv("shared",(i&1)?"red":"blue",1));
        const char *common=getenv("shared");CHECK(same(common,"red")||same(common,"blue"));
        CHECK(same(getenv("stable"),"untouched"));
        struct timespec pause={0,1000000};CHECK(!clock_nanosleep(CLOCK_MONOTONIC,0,&pause,0));
    }
    CHECK(!unsetenv(key));return raw;
}
int _start(void)
{
    /* The Linux host sets this deliberately; it must never enter the guest. */
    errno=41;CHECK(!getenv("MYOS_ENV_HOST_ONLY")&&!getenv("stable")&&errno==41);
    CHECK(getgid()==0&&getegid()==getgid()&&__normal_user());
    CHECK(setenv("","bad",1)==-1&&errno==EINVAL);
    CHECK(setenv("a=b","bad",1)==-1&&errno==EINVAL);
    CHECK(setenv("a",0,1)==-1&&errno==EINVAL);
    CHECK(unsetenv("a=b")==-1&&errno==EINVAL);
    errno=42;CHECK(!unsetenv("absent")&&errno==42);
    CHECK(!setenv("stable","untouched",1));
    char input[]="first";CHECK(!setenv("copied",input,1));input[0]='X';
    char *old=getenv("copied");CHECK(same(old,"first"));
    CHECK(!setenv("copied","ignored",0)&&getenv("copied")==old);
    CHECK(!setenv("copied","second",1)&&same(old,"first"));
    CHECK(same(secure_getenv("copied"),"second"));
    CHECK(!unsetenv("copied")&&!getenv("copied")&&same(old,"first"));
    CHECK(!setenv("empty","",1)&&getenv("empty")&&!*getenv("empty"));
    CHECK(!unsetenv("empty"));
    CHECK(!pthread_cond_init(&start,0));
    pthread_t threads[8];for(unsigned i=0;i<8;++i)
        CHECK(!pthread_create(&threads[i],0,worker,(void *)(uintptr_t)(i+1)));
    for(;;){CHECK(!pthread_mutex_lock(&gate));if(ready==8){go=1;
        CHECK(!pthread_cond_broadcast(&start));CHECK(!pthread_mutex_unlock(&gate));break;}
        CHECK(!pthread_mutex_unlock(&gate));struct timespec pause={0,1000000};
        CHECK(!clock_nanosleep(CLOCK_MONOTONIC,0,&pause,0));
    }
    for(unsigned i=0;i<8;++i){void *result;CHECK(!pthread_join(threads[i],&result));CHECK(result==(void *)(uintptr_t)(i+1));}
    CHECK(!pthread_cond_destroy(&start)&&!pthread_mutex_destroy(&gate));
    CHECK(!unsetenv("shared")&&!unsetenv("stable"));
    for(unsigned i=0;i<64;++i){char key[]="slot00";key[4]=(char)('0'+i/10);key[5]=(char)('0'+i%10);CHECK(!setenv(key,"v",1));}
    CHECK(setenv("overflow","v",1)==-1&&errno==ENOMEM);
    CHECK(!unsetenv("slot00")&&!setenv("replacement","v",1));
    for(unsigned i=1;i<64;++i){char key[]="slot00";key[4]=(char)('0'+i/10);key[5]=(char)('0'+i%10);CHECK(!unsetenv(key));}
    CHECK(!unsetenv("replacement"));
    CHECK(!setenv("survivor","safe",1));
    char large[4096];memset(large,'a',sizeof(large)-1);large[sizeof(large)-1]=0;
    unsigned changed=0;while(changed<32&&setenv("pressure",large,1)==0)++changed;
    CHECK(changed>0&&changed<32&&errno==ENOMEM);
    char *preserved=getenv("pressure");
    CHECK(setenv("pressure",large,1)==-1&&errno==ENOMEM&&getenv("pressure")==preserved);
    CHECK(same(getenv("survivor"),"safe")&&same(old,"first"));
    const char passed[]="native Mesa environment concurrency/bounds/identity PASS\n";
    message(passed,sizeof(passed)-1);return 78;
}
