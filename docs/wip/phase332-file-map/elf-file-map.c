#include <stddef.h>
#include <stdint.h>
extern int openat(int,const char *,int,...),close(int),dup(int),mprotect(void *,size_t,int),munmap(void *,size_t),madvise(void *,size_t,int);
extern void *mmap(void *,size_t,int,int,int,int64_t);
extern int64_t lseek(int,int64_t,int),read(int,void *,size_t),write(int,const void *,size_t);
extern int *__errno_location(void);
static int64_t raw(uint64_t n,uint64_t a,uint64_t b,uint64_t c)
{register uint64_t result __asm__("rax")=n;__asm__ volatile("int $0x80":"+a"(result):"D"(a),"S"(b),"d"(c):"memory","cc");return (int64_t)result;}
static unsigned char expected(size_t i){return (unsigned char)((i*37u+i/251u+19u)&255u);}
static void message(const char *s){size_t n=0;while(s[n])++n;write(1,s,n);}
static int bad(void *p,int error){return p==(void *)-1&&*__errno_location()==error;}
uint64_t _start(void)
{
    if(raw(57,1,32,0)!=-14||raw(57,1,31,0)!=-22)return 1;
    int fd=openat(-100,"/assets/mapdata",0),alias=dup(fd);if(fd<0||alias<0)return 2;
    if(lseek(fd,123,0)!=123)return 3;
    int64_t before=raw(50,1,0,0);
    unsigned char *p=mmap(0,12295,1,1,fd,0);if(p==(void *)-1)return 4;
    for(size_t i=0;i<12295;++i)if(p[i]!=expected(i))return 5;
    for(size_t i=12295;i<16384;++i)if(p[i])return 6;
    if(lseek(alias,0,1)!=123||close(fd))return 7;
    int reused=openat(-100,"/etc/motd",0);if(reused!=fd||p[9000]!=expected(9000))return 8;
    if(mprotect(p,16384,3)!=-1||*__errno_location()!=13)return 9;
    if(mprotect(p,16384,0)||mprotect(p,16384,1)||p[9000]!=expected(9000))return 10;
    if(madvise(p,16384,4)!=-1||*__errno_location()!=95||p[0]!=expected(0))return 11;
#if MODE == 1
    message("file map readonly deliberate fault ready\n");*(volatile unsigned char *)p=9;return 99;
#elif MODE == 2
    message("file map cancel ready\n");for(;;)raw(4,0,0,0);
#else
    if(munmap(p+4096,4096)||mprotect(p,16384,1)!=-1||*__errno_location()!=12)return 12;
    if(p[0]!=expected(0)||p[8192]!=expected(8192)||munmap(p,16384))return 13;
    /* Match live descriptions when comparing pages: reused is a newly opened
     * object, unlike the original dup pair sharing one object. */
    if(close(reused)||raw(50,1,0,0)!=before)return 14;
    reused=openat(-100,"/etc/motd",0);if(reused<0)return 35;
    unsigned char *q=mmap(0,1,3,2,alias,4096);if(q==(void *)-1||q[0]!=expected(4096)||q[4095]!=expected(8191))return 15;
    q[0]=99;if(mprotect(q,4096,0)||mprotect(q,4096,3)||q[0]!=99)return 16;
    unsigned char *r=mmap(0,4096,1,1,alias,4096);if(r==(void *)-1||r[0]!=expected(4096))return 17;
    if(lseek(alias,4096,0)!=4096)return 18;
    unsigned char byte=0;
    if(read(alias,&byte,1)!=1||byte!=expected(4096)||q[0]!=99)return 19;
    if(munmap(q,4096)||munmap(r,4096))return 20;
    if(!bad(mmap(0,1,3,1,alias,0),13)||!bad(mmap(0,1,5,2,alias,0),95))return 21;
    if(!bad(mmap(0,1,1,1,alias,1),22)||!bad(mmap(0,1,1,1,alias,-4096),22))return 22;
    if(!bad(mmap(0,16385,1,1,alias,0),95)||!bad(mmap(0,1,1,1,alias,16384),95))return 23;
    if(!bad(mmap(0,1,1,1,reused,0),95)||!bad(mmap(0,1,1,1,-1,0),9))return 24;
    unsigned char *mixed=mmap(0,12288,3,0x22,-1,0);if(mixed==(void *)-1)return 25;
    mixed[0]=55;mixed[8192]=66;
    if(mmap(mixed+4096,4096,1,0x100001,alias,0)!=(void *)-1||*__errno_location()!=17)return 26;
    if(mmap(mixed+4096,4096,1,0x11,alias,0)!=mixed+4096)return 27;
    if(mprotect(mixed,12288,3)!=-1||*__errno_location()!=13)return 28;
    mixed[0]=77;if(mixed[8192]!=66||mixed[4096]!=expected(0))return 29;
    if(madvise(mixed,12288,4)!=-1||*__errno_location()!=95||mixed[0]!=77)return 30;
    if(munmap(mixed+4096,4096))return 31;
    if(mmap(mixed+4096,4096,3,0x100022,-1,0)!=mixed+4096)return 32;
    mixed[4096]=88;if(munmap(mixed,12288))return 33;
    if(close(reused)||close(alias))return 34;
    message("immutable file map bytes/offset/lifetime/rights/EOF/partial/fixed PASS\n");return 90;
#endif
}
