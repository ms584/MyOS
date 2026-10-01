#include <inttypes.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <fenv.h>
#include "util/rounding.h"
static void message(const char *text,size_t length)
{
    register uint64_t a __asm__("rax")=1,d __asm__("rdi")=(uintptr_t)text;
    register uint64_t s __asm__("rsi")=length;
    __asm__ volatile("int $0x80":"+a"(a):"D"(d),"S"(s):"rdx","memory","cc");
}
#define CHECK(x) do{if(!(x)){const char t[]="integer SDK failed: " #x "\n";message(t,sizeof(t)-1);__builtin_trap();}}while(0)
int _start(void)
{
    CHECK(sizeof(off_t)==8&&sizeof(ssize_t)==8&&sizeof(pid_t)==4&&sizeof(mode_t)==4);
    CHECK(LONG_MAX==INT64_MAX&&ULONG_MAX==UINT64_MAX);
    CHECK(fesetround(FE_TONEAREST)==0);
    volatile float single[]={1.5f,2.5f,-1.5f,-2.5f,16777216.0f};
    volatile double dual[]={1.5,2.5,-1.5,-2.5,2147483648.0};
    const long expected[]={2,2,-2,-2,16777216L};
    for(unsigned i=0;i<5;++i){
        CHECK(_mesa_lroundevenf(single[i])==expected[i]);
        CHECK(_mesa_i64roundevenf(single[i])==expected[i]);
        CHECK(_mesa_lroundeven(dual[i])==(i==4?2147483648L:expected[i]));
    }
    int fd=openat(AT_FDCWD,"/home/headercheck",O_CREAT|O_TRUNC|O_RDWR|O_CLOEXEC,0600);
    CHECK(fd>=0&&fcntl(fd,F_GETFD)==FD_CLOEXEC);
    CHECK(write(fd,"headers",7)==7&&lseek(fd,0,SEEK_SET)==0);
    int copy=dup(fd);CHECK(copy>=0&&fcntl(copy,F_GETFD)==0);
    char text[8]={0};CHECK(read(copy,text,3)==3&&text[0]=='h'&&text[2]=='a');
    CHECK(lseek(fd,0,SEEK_CUR)==3);
    CHECK(pread(fd,text,7,0)==7&&lseek(fd,0,SEEK_CUR)==3);
    CHECK(fcntl(copy,F_SETFD,(long)FD_CLOEXEC)==0&&fcntl(copy,F_GETFD)==FD_CLOEXEC);
    CHECK(close(fd)==0&&read(copy,text,4)==4&&text[0]=='d');
    CHECK(close(copy)==0&&unlinkat(AT_FDCWD,"/home/headercheck",0)==0);
    CHECK(read(copy,text,1)==-1&&errno==EBADF);
    int pipes[2];CHECK(pipe2(pipes,O_NONBLOCK|O_CLOEXEC)==0);
    CHECK(read(pipes[0],text,1)==-1&&errno==EAGAIN);
    CHECK(write(pipes[1],"p",1)==1&&read(pipes[0],text,1)==1&&text[0]=='p');
    CHECK(close(pipes[1])==0&&read(pipes[0],text,1)==0&&close(pipes[0])==0);
    CHECK(getpid()>0&&getuid()==0&&geteuid()==0);
    const char passed[]="native integer headers/Mesa rounding/descriptors PASS\n";
    message(passed,sizeof(passed)-1);return 77;
}
