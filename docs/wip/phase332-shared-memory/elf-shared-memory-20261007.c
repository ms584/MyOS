#include <stddef.h>
#include <stdint.h>
#include <pthread.h>
extern int mkdir(const char *,unsigned),unlinkat(int,const char *,int),openat(int,const char *,int,...),close(int),ftruncate(int,int64_t),mprotect(void *,size_t,int),munmap(void *,size_t),msync(void *,size_t,int),*__errno_location(void);
extern void *mmap(void *,size_t,int,int,int,int64_t);
extern int64_t write(int,const void *,size_t),pread(int,void *,size_t,int64_t),pwrite(int,const void *,size_t,int64_t),lseek(int,int64_t,int);
#define BAD ((void *)(uintptr_t)-1)
static void *worker(void *argument)
{
 char path[]="/dev/shm/worker0";path[15]=(char)('0'+(uintptr_t)argument);
 int fd=openat(-100,path,2|0100|0200,0600);if(fd<0||ftruncate(fd,4096))return (void *)1;
 unsigned char *a=mmap(0,4096,3,1,fd,0),*b=mmap(0,4096,3,1,fd,0);
 if(a==BAD||b==BAD)return (void *)2;
 for(unsigned i=0;i<4096;++i){a[i]=(unsigned char)i;if(b[i]!=(unsigned char)i)return (void *)3;}
 if(unlinkat(-100,path,0)||close(fd)||munmap(a,4096)||munmap(b,4096))return (void *)4;
 return 0;
}
int _start(void)
{
 if(mkdir("/dev/shm",0700)&&*__errno_location()!=17)return 1;
 const char *path="/dev/shm/.org.chromium.Chromium.demo";
 int fd=openat(-100,path,2|0100|0200,0600);if(fd<0||ftruncate(fd,262144))return 2;
 unsigned char *a=mmap(0,262144,3,1,fd,0),*b=mmap(0,262144,3,1,fd,0);
 if(a==BAD||b==BAD)return 3;
 for(unsigned i=0;i<262144;i+=4096){if(a[i]||b[i])return 4;a[i]=(unsigned char)(i/4096+1);if(b[i]!=a[i])return 5;}
 unsigned char byte=0;if(pread(fd,&byte,1,4096)!=1||byte!=2||pwrite(fd,"Z",1,4096)!=1||a[4096]!='Z'||b[4096]!='Z')return 6;
 if(ftruncate(fd,4)!=-1||*__errno_location()!=16)return 7;
 if(ftruncate(fd,1048577)!=-1||*__errno_location()!=27)return 8;
 int ro=openat(-100,path,0,0);if(ro<0)return 9;
 unsigned char *read_only=mmap(0,4096,1,1,ro,0);if(read_only==BAD)return 10;
 if(mprotect(read_only,4096,3)!=-1||*__errno_location()!=13)return 11;
 if(mmap(0,4096,3,1,ro,0)!=BAD||*__errno_location()!=13)return 12;
 unsigned char *private=mmap(0,4096,3,2,ro,0);if(private==BAD)return 13;
 private[0]=0x77;if(a[0]==0x77||b[0]==0x77)return 14;
 if(mmap(0,4096,5,1,fd,0)!=BAD||*__errno_location()!=95)return 15;
 if(mmap(0,4096,1,1,fd,262144)!=BAD||*__errno_location()!=95)return 16;
 if(lseek(fd,7000,0)!=7000)return 17;
 unsigned char *offset=mmap(0,4096,1,1,fd,4096);if(offset==BAD||offset[0]!='Z'||lseek(fd,0,1)!=7000)return 18;
 if(msync(a,262144,4))return 19;
 if(unlinkat(-100,path,0)||close(ro)||close(fd))return 20;
 if(a[4096]!='Z'||b[4096]!='Z'||offset[0]!='Z')return 21;
 if(munmap(a,262144)||b[4096]!='Z'||munmap(b,262144)||munmap(offset,4096)||munmap(read_only,4096)||munmap(private,4096))return 22;
 pthread_t threads[4];for(uintptr_t i=0;i<4;++i)if(pthread_create(&threads[i],0,worker,(void *)i))return 23;
 for(unsigned i=0;i<4;++i){void *result;if(pthread_join(threads[i],&result)||result)return 24;}
 fd=openat(-100,"/dev/shm/deferred-exit",2|0100|0200,0600);if(fd<0||ftruncate(fd,4))return 25;
 unsigned char *tail=mmap(0,4,3,1,fd,0);if(tail==BAD)return 26;tail[0]=0x4a;
 if(close(fd)||unlinkat(-100,"/dev/shm/deferred-exit",0)||tail[0]!=0x4a)return 27;
 /* Intentionally leave this unlinked alias for process-stop reclamation. */
 const char message[]="native shared memory coherence/lifetime/rights/workers PASS\n";
 write(1,message,sizeof(message)-1);return 92;
}
