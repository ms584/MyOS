#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <stddef.h>
extern void *mmap(void *,size_t,int,int,int,off_t);
extern int unlinkat(int,const char *,int);
extern void SbLogRaw(const char *);
#define CHECK(x) do{if(!(x)){SbLogRaw("named SHM lifetime failed: " #x "\n");return __LINE__;}}while(0)
int _start(void){
 const char *path="/dev/shm/creator-exit-shm";
#if CASE==0 || CASE==3
 int fd=openat(AT_FDCWD,path,O_CREAT|O_EXCL|O_RDWR,0600);CHECK(fd>=0);
 CHECK(!ftruncate(fd,4096));CHECK(write(fd,"K",1)==1);
#else
 int fd=openat(AT_FDCWD,path,O_RDWR);CHECK(fd>=0);
#endif
#if CASE==1 || CASE==3
 unsigned char *alias=mmap(0,4096,3,1,fd,0);CHECK(alias!=(void *)(uintptr_t)-1);
 CHECK(alias[0]=='K');alias[0]='M';
 /* Deliberately omit munmap: process reap must release its alias reference. */
#endif
#if CASE==2
 char byte=0;CHECK(read(fd,&byte,1)==1&&byte=='M');
#endif
#if CASE==2 || CASE==3
 CHECK(!unlinkat(AT_FDCWD,path,0));
#endif
 CHECK(!close(fd));
 SbLogRaw("named SHM creator/alias/unlink lifetime stage PASS\n");return 88;
}
