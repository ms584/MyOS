#include <stddef.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include "system-call.h"
extern void *malloc(size_t);
extern void free(void *),SbLogRaw(const char *);
struct dirent {uint64_t ino;int64_t offset;uint16_t record;unsigned char type;char name[256];};
extern void *opendir(const char *);
extern struct dirent *readdir(void *);
extern int closedir(void *);
extern void *mmap(void *,size_t,int,int,int,int64_t);
extern int mprotect(void *,size_t,int),munmap(void *,size_t);
static int64_t raw_entry(uint64_t pointer,uint64_t reserved)
{register uint64_t a __asm__("rax")=MYOS_SYSTEM_CALL;
 __asm__ volatile("int $0x80":"+a"(a):"D"((uint64_t)SYSTEM_ASSET_ENTRY),"S"(pointer),"d"(reserved):"memory","cc");return (int64_t)a;}
static int equal(const char *a,const char *b)
{while(*a&&*a==*b){++a;++b;}return *a==*b;}
static int directory(const char *path,unsigned expected)
{
 void *d=opendir(path);if(!d)return -1;unsigned count=0;struct dirent *e;
 errno=0;while((e=readdir(d))){
  if(equal(e->name,"myos-runtime.mrt"))return -2;
  if(e->type!=4&&e->type!=8)return -3;
  if(++count>expected)return -4;
 }
 int error=errno;if(closedir(d)||error||count!=expected)return -5;return 0;
}
int _start(void)
{
 struct stat info;
 if(fstatat(AT_FDCWD,"/assets/content",&info,0)||!S_ISDIR(info.st_mode))return 1;
 if(fstatat(AT_FDCWD,"/assets/content/fonts",&info,0)||!S_ISDIR(info.st_mode))return 2;
 if(directory("/assets",3)||directory("/assets/content",3)||directory("/assets/content/fonts",2))return 3;
 void *d=opendir("/assets/content/fonts");if(!d)return 4;
 struct dirent *e=readdir(d);if(!e||e->type!=8||!equal(e->name,"font.bin"))return 5;
 const char long_name[]="resource-name-that-exceeds-legacy-directory-metadata-limit-for-browser-fonts.bin";
 e=readdir(d);if(!e||e->type!=8||!equal(e->name,long_name)||readdir(d)||closedir(d))return 18;
 int parent=openat(AT_FDCWD,"/assets/content/fonts",O_DIRECTORY|O_RDONLY,0);if(parent<0)return 26;
 int resource=openat(parent,long_name,O_RDONLY,0);char text[32]={0};
 if(resource<0||read(resource,text,sizeof(text))!=19||!equal(text,"controlled-resource"))return 27;
 if(fstatat(parent,long_name,&info,0)||info.st_size!=19)return 28;
 if(openat(resource,"font.bin",O_RDONLY,0)!=-1||errno!=ENOTDIR)return 29;
 if(openat(-1,"font.bin",O_RDONLY,0)!=-1||errno!=EBADF)return 30;
 int absolute=openat(-1,"/assets/root.txt",O_RDONLY,0);if(absolute<0||close(absolute))return 31;
 int sibling=openat(parent,"../cert.pem",O_RDONLY,0);if(sibling<0||close(sibling))return 32;
 if(openat(parent,"font.bin/../font.bin",O_RDONLY,0)!=-1||errno!=ENOTDIR)return 35;
 if(openat(parent,"absent/../font.bin",O_RDONLY,0)!=-1||errno!=ENOENT)return 36;
 if(close(resource)||close(parent))return 33;
 if(openat(parent,long_name,O_RDONLY,0)!=-1||errno!=EBADF)return 34;
 int fd=openat(AT_FDCWD,"/assets/content/fonts/font.bin",O_RDONLY,0);if(fd<0)return 6;
 if(fstat(fd,&info)||!S_ISREG(info.st_mode)||info.st_size!=65537)return 7;
 unsigned char *data=malloc(32768);if(!data)return 8;
 if(read(fd,data,32768)!=32768)return 9;
 for(unsigned i=0;i<32768;++i)if(data[i]!=(unsigned char)(i*13u+7u))return 10;
 if(lseek(fd,0,SEEK_CUR)!=32768||pread(fd,data,8192,49152)!=8192||lseek(fd,0,SEEK_CUR)!=32768)return 11;
 for(unsigned i=0;i<8192;++i)if(data[i]!=(unsigned char)((49152+i)*13u+7u))return 12;
 if(lseek(fd,65536,SEEK_SET)!=65536||read(fd,data,32768)!=1||data[0]!=7||read(fd,data,1))return 13;
 free(data);if(close(fd))return 14;
 int directory_fd=openat(AT_FDCWD,"/assets/content",O_RDONLY,0);
 char byte;if(directory_fd<0||read(directory_fd,&byte,1)!=-1||errno!=EISDIR||close(directory_fd))return 15;
 if(openat(AT_FDCWD,"/assets/content/cert.pem",O_WRONLY,0)!=-1||errno!=EROFS)return 16;
 if(openat(AT_FDCWD,"/assets/content/no-file",O_RDONLY,0)!=-1||errno!=ENOENT)return 17;
 unsigned char *pages=mmap(0,8192,3,0x22,-1,0);if(pages==(void *)(uintptr_t)-1)return 19;
 for(unsigned i=0;i<128;++i)pages[4096-128+i]='G';
 if(mprotect(pages+4096,4096,1))return 20;
 struct system_asset_entry_request request={(uintptr_t)"/assets/content/fonts",0,(uintptr_t)(pages+4096-128)};
 if(raw_entry((uintptr_t)&request,0)!=-14)return 21;
 for(unsigned i=0;i<128;++i)if(pages[4096-128+i]!='G')return 22;
 if(raw_entry(0,0)!=-14||raw_entry((uintptr_t)&request,1)!=-22)return 23;
 request.position=UINT64_MAX;if(raw_entry((uintptr_t)&request,0)!=-22)return 24;
 if(munmap(pages,8192))return 25;
 SbLogRaw("native v2 hierarchy/hidden runtime/long names/large I/O/readonly/pointer ABI PASS\n");return 90;
}
