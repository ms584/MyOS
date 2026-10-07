#include <sys/statvfs.h>
#include <stddef.h>
#include <stdint.h>
#include <pthread.h>
#include "starboard_layer_posix_statvfs_abi_wrappers.h"
_Static_assert(sizeof(struct musl_statvfs)==sizeof(struct statvfs)&&offsetof(struct musl_statvfs,f_namemax)==offsetof(struct statvfs,f_namemax),"actual pinned ABI wrapper");
extern int mkdir(const char *,unsigned),unlinkat(int,const char *,int),openat(int,const char *,int,...),close(int),mprotect(void *,size_t,int),munmap(void *,size_t),*__errno_location(void);
extern void *mmap(void *,size_t,int,int,int,int64_t);
extern int64_t write(int,const void *,size_t);
static int64_t raw(uint64_t n,uint64_t a,uint64_t b,uint64_t c)
{register uint64_t r __asm__("rax")=n;__asm__ volatile("int $0x80":"+a"(r):"D"(a),"S"(b),"d"(c):"memory","cc");return (int64_t)r;}
static int validate(struct statvfs *s)
{if(s->f_bsize!=4096||s->f_frsize!=4096||s->f_blocks||s->f_bfree||s->f_bavail||s->f_files!=64||s->f_ffree>64||s->f_favail!=s->f_ffree||s->f_fsid!=1||s->f_flag!=ST_NOSUID||s->f_namemax!=31)return 1;for(unsigned i=0;i<6;++i)if(s->__reserved[i])return 1;return 0;}
static void *worker(void *unused)
{(void)unused;for(unsigned i=0;i<64;++i){struct statvfs s;if(statvfs("/home",&s)||validate(&s))return (void *)1;}return 0;}
int _start(void)
{
 struct statvfs initial,now;unsigned char *bytes=(unsigned char *)&now;
 if(statvfs("/home",&initial)||validate(&initial))return 1;
 for(unsigned i=0;i<sizeof(now);++i)bytes[i]=0x5a;
 if(statvfs("/missing",&now)!=-1||*__errno_location()!=2)return 2;
 for(unsigned i=0;i<sizeof(now);++i)if(bytes[i]!=0x5a)return 3;
 if(statvfs("/assets",&now)!=-1||*__errno_location()!=95||statvfs("/",0)!=-1||*__errno_location()!=14)return 4;
 if(raw(58,1,(uintptr_t)&now,0)!=-14||raw(58,(uintptr_t)"/",1,0)!=-14||raw(58,(uintptr_t)"/",(uintptr_t)&now,1)!=-22)return 5;
 if(mkdir("/home/vfs-TEzLw==",0700)||statvfs("/home/vfs-TEzLw==",&now)||now.f_ffree+1!=initial.f_ffree)return 6;
 int fd=openat(-100,"/home/vfs-TEzLw==/Experiment Config",2|0100,0600);if(fd<0)return 7;
 if(statvfs("/home/vfs-TEzLw==",&now)||now.f_ffree+2!=initial.f_ffree)return 8;
 if(unlinkat(-100,"/home/vfs-TEzLw==/Experiment Config",0)||statvfs("/home",&now)||now.f_ffree+2!=initial.f_ffree)return 9;
 if(close(fd)||statvfs("/home",&now)||now.f_ffree+1!=initial.f_ffree)return 10;
 if(unlinkat(-100,"/home/vfs-TEzLw==",0x200)||statvfs("/home",&now)||now.f_ffree!=initial.f_ffree)return 11;
 unsigned char *p=mmap(0,8192,3,0x22,-1,0);if(p==(void *)-1)return 12;
 for(unsigned i=0;i<8192;++i)p[i]=0x6b;
 if(mprotect(p+4096,4096,1)||statvfs("/",(struct statvfs *)(p+4096-56))!=-1||*__errno_location()!=14)return 13;
 for(unsigned i=4096-56;i<4096+56;++i)if(p[i]!=0x6b)return 14;
 if(munmap(p,8192))return 15;
 pthread_t threads[4];for(unsigned i=0;i<4;++i)if(pthread_create(&threads[i],0,worker,0))return 16;
 for(unsigned i=0;i<4;++i){void *result;if(pthread_join(threads[i],&result)||result)return 17;}
 const char message[]="native pinned statvfs counts/lifetime/bounds/whole-output validation/concurrency PASS\n";
 write(1,message,sizeof(message)-1);return 92;
}
