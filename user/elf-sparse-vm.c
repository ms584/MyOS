#include <stddef.h>
#include <stdint.h>
struct timespec {int64_t seconds,nanoseconds;};
struct stat_abi {int64_t dev,ino,nlink;uint32_t mode,uid,gid,pad;int64_t rdev,size,blksize,blocks;struct timespec atime,mtime,ctime;int64_t unused[3];};
struct pollfd {int fd;uint16_t events,revents;};
extern void *mmap(void *,size_t,int,int,int,int64_t);
extern int mprotect(void *,size_t,int),munmap(void *,size_t),mincore(void *,size_t,unsigned char *);
extern int madvise(void *,size_t,int),openat(int,const char *,int,...),dup(int),close(int),fcntl(int,int,...);
extern int fstat(int,struct stat_abi *),fstatat(int,const char *,struct stat_abi *,int),poll(struct pollfd *,size_t,int);
extern int64_t read(int,void *,size_t),write(int,const void *,size_t),lseek(int,int64_t,int);
extern int *__errno_location(void);
static int64_t raw(uint64_t number,uint64_t a,uint64_t b,uint64_t c)
{register uint64_t result __asm__("rax")=number;__asm__ volatile("int $0x80":"+a"(result):"D"(a),"S"(b),"d"(c):"memory","cc");return (int64_t)result;}
static void message(const char *s){size_t n=0;while(s[n])++n;write(1,s,n);}
uint64_t _start(void)
{
    unsigned char bytes[512];int fd=openat(-100,"/dev/urandom",524288|2048);
    if(fd<0||fcntl(fd,1)!=1)return 1;
    struct stat_abi st,path;if(fstat(fd,&st)||fstatat(-100,"/dev/urandom",&path,0)||st.mode!=(0020000|0444)||st.size||st.ino!=path.ino)return 2;
    int alias=dup(fd);if(alias<0||fcntl(alias,1)||close(fd))return 3;
    if(read(alias,0,0)||read(alias,(void *)1,1)!=-1||*__errno_location()!=14)return 4;
    if(write(alias,bytes,1)!=-1||*__errno_location()!=9||lseek(alias,0,0)!=-1||*__errno_location()!=29)return 5;
#ifdef NO_ENTROPY
    if(read(alias,bytes,16)!=-1||*__errno_location()!=38)return 6;
#else
    if(read(alias,bytes,sizeof(bytes))!=256||read(alias,bytes+256,256)!=256)return 6;
    unsigned different=0;for(unsigned i=0;i<256;++i)different|=bytes[i]^bytes[i+256];if(!different)return 7;
#endif
    struct pollfd pfd={alias,1,0};if(poll(&pfd,1,0)!=1||!(pfd.revents&1)||close(alias))return 8;
    if(read(alias,bytes,1)!=-1||*__errno_location()!=9)return 9;
    if(openat(-100,"/dev/urandom",1)!=-1||*__errno_location()!=95)return 10;
    if(openat(-100,"/dev/urandom",0100|0200)!=-1||*__errno_location()!=17)return 11;
    int filled[32];for(unsigned i=0;i<32;++i){filled[i]=openat(-100,"/dev/urandom",0);if(filled[i]<0)return 12;}
    if(openat(-100,"/dev/urandom",0)!=-1||*__errno_location()!=24)return 13;
    for(unsigned i=0;i<32;++i)if(close(filled[i]))return 14;
    const uint64_t length=0x800000000ull,mid=0x400000000ull;
    int64_t before=raw(50,1,0,0);
    unsigned char *base=mmap(0,length,0,0x22,-1,0);if(base==(void *)-1)return 15;
    if(raw(50,1,0,0)!=before)return 16;
    unsigned char residency=9;
    if(mincore(base+mid,4096,&residency)||residency)return 17;
    if(raw(1,(uintptr_t)base,1,0)!=-14)return 18;
    if(mmap(base,length,0,0x100022,-1,0)!=(void *)-1||*__errno_location()!=17)return 19;
    uint64_t offsets[]={0,mid,length-4096};
    for(unsigned i=0;i<3;++i){unsigned char *page=base+offsets[i];if(mprotect(page,4096,3))return 20;
        if(page[0]||page[4095])return 21;
        page[0]=(unsigned char)(40+i);page[4095]=99;
        residency=0;if(mincore(page,4096,&residency)||residency!=1)return 22;}
    if(before-raw(50,1,0,0)>12)return 23;
    if(mprotect(base,length,3)!=-1||*__errno_location()!=12)return 24;
    for(unsigned i=0;i<3;++i)if(base[offsets[i]]!=40+i||base[offsets[i]+4095]!=99)return 25;
    if(mprotect(base,4096,0)||raw(1,(uintptr_t)base,1,0)!=-14)return 26;
#if MODE == 1
    message("sparse reservation deliberate fault ready\n");return *(volatile unsigned char *)base;
#elif MODE == 2
    message("sparse reservation cancel ready\n");for(;;)raw(4,0,0,0);
#else
    if(mprotect(base,4096,3)||base[0]!=40)return 27;
    if(madvise(base,length,4)||base[0]||base[mid]||base[length-4096])return 28;
    if(munmap(base+mid,4096)||mincore(base+mid,4096,&residency)!=-1||*__errno_location()!=12)return 29;
    if(mprotect(base,length,3)!=-1||*__errno_location()!=12)return 30;
    if(mmap(base+mid,4096,0,0x100022,-1,0)!=base+mid)return 31;
    if(mprotect(base+mid,4096,3)||base[mid])return 32;
    base[mid]=73;
    if(mmap(base+mid,4096,3,0x32,-1,0)!=base+mid||base[mid])return 33;
    if(munmap(base,length)||raw(50,1,0,0)!=before)return 34;
    if(munmap(base,length))return 35;
    /* Engine alignment fallback: overreserve64GiB, trim to aligned32GiB. */
    base=mmap(0,2*length,0,0x22,-1,0);if(base==(void *)-1)return 36;
    uintptr_t aligned=((uintptr_t)base+length-1)&~(length-1);
    uint64_t prefix=aligned-(uintptr_t)base,tail=2*length-prefix-length;
    if(prefix&&munmap(base,prefix))return 37;
    if(tail&&munmap((void *)(aligned+length),tail))return 38;
    if(mprotect((void *)aligned,4096,3))return 39;
    *(volatile uint64_t *)aligned=0x12345678;
    if(munmap((void *)aligned,length)||raw(50,1,0,0)!=before)return 40;
    message("sparse32GiB reserve/commit/rollback/residency/partial-unmap/alignment and real entropy descriptor PASS\n");
    return 89;
#endif
}
