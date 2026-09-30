#include <stddef.h>
#include <stdint.h>

#define AT_FDCWD (-100)
#define O_RDWR 2
#define O_CREAT 0100
#define O_EXCL 0200
#define O_TRUNC 01000
#define O_APPEND 02000
#define O_NONBLOCK 04000
#define O_CLOEXEC 02000000
#define O_DIRECTORY 0200000
#define AT_REMOVEDIR 0x200
#define F_DUPFD 0
#define F_GETFD 1
#define F_SETFD 2
#define F_GETFL 3
#define F_SETFL 4
#define F_DUPFD_CLOEXEC 1030
#define FD_CLOEXEC 1

struct timespec { int64_t tv_sec,tv_nsec; };
struct stat_abi {
    int64_t dev,ino,nlink;uint32_t mode,uid,gid,pad;int64_t rdev,size,blksize,blocks;
    struct timespec atime,mtime,ctime;int64_t unused[3];
};
struct iovec_abi { void *base;size_t length; };
struct dirent_abi { uint64_t ino;int64_t offset;uint16_t record_length;unsigned char type;char name[256]; };
struct statvfs_abi {
    unsigned long block_size,fragment_size;uint64_t blocks,free_blocks,available_blocks;
    uint64_t files,free_files,available_files;unsigned long filesystem_id,flags,name_max;int spare[6];
};

extern int *__errno_location(void);
extern int openat(int,const char *,int,...),close(int),dup(int),fcntl(int,int,...);
extern int fstat(int,struct stat_abi *),fstatat(int,const char *,struct stat_abi *,int);
extern int ftruncate(int,int64_t),fsync(int),fdatasync(int),access(const char *,int);
extern int mkdir(const char *,uint32_t),isatty(int);
extern void *opendir(const char *),*fdopendir(int);
extern struct dirent_abi *readdir(void *);
extern int readdir_r(void *,struct dirent_abi *,struct dirent_abi **),closedir(void *);
extern int unlinkat(int,const char *,int),mkstemp(char *);
extern char *mkdtemp(char *),*realpath(const char *,char *);
extern int64_t read(int,void *,size_t),write(int,const void *,size_t);
extern int64_t readv(int,const struct iovec_abi *,int),writev(int,const struct iovec_abi *,int);
extern int64_t lseek(int,int64_t,int),pread(int,void *,size_t,int64_t),pwrite(int,const void *,size_t,int64_t);
extern char *getcwd(char *,size_t);
extern long pathconf(const char *,int);

static void print(const char *text)
{
    size_t length=0;while(text[length])++length;(void)write(1,text,length);
}
static int same(const char *a,const char *b,size_t length)
{
    for(size_t i=0;i<length;++i)if(a[i]!=b[i])return 0;
    return 1;
}
static int equal(const char *a,const char *b)
{while(*a&&*a==*b){++a;++b;}return *a==*b;}
uint64_t _start(void)
{
    if(mkdir("/home/fileabi",0777))return 1;
    int fd=openat(AT_FDCWD,"/home/fileabi/state",O_CREAT|O_RDWR|O_TRUNC,0666);
    if(fd<64)return 2;
    char first[]="shared-",second[]="offset";
    struct iovec_abi out[2]={{first,7},{second,6}};
    if(writev(fd,out,2)!=13)return 3;
    struct stat_abi information;
    if(fstat(fd,&information)||information.size!=13||(information.mode&0170000)!=0100000)return 4;
    if(lseek(fd,0,0)!=0)return 5;
    char left[8]={0},right[7]={0};struct iovec_abi in[2]={{left,7},{right,6}};
    if(readv(fd,in,2)!=13||!same(left,"shared-",7)||!same(right,"offset",6))return 6;
    if(lseek(fd,3,0)!=3)return 7;
    char positioned[7]={0};if(pread(fd,positioned,6,7)!=6||!same(positioned,"offset",6)||lseek(fd,0,1)!=3)return 8;
    if(pwrite(fd,"FILE",4,7)!=4||lseek(fd,0,1)!=3)return 9;
    int copy=dup(fd);if(copy<64||lseek(copy,0,1)!=3||lseek(copy,2,1)!=5||lseek(fd,0,1)!=5)return 10;
    int high=fcntl(fd,F_DUPFD,70);if(high<70)return 11;
    int cloexec=fcntl(fd,F_DUPFD_CLOEXEC,71);if(cloexec<71||fcntl(cloexec,F_GETFD)!=FD_CLOEXEC)return 12;
    if(fcntl(high,F_SETFD,FD_CLOEXEC)||fcntl(high,F_GETFD)!=FD_CLOEXEC)return 13;
    if(fcntl(fd,F_SETFL,O_APPEND|O_NONBLOCK)||(fcntl(fd,F_GETFL)&(O_APPEND|O_NONBLOCK))!=(O_APPEND|O_NONBLOCK))return 14;
    if((fcntl(copy,F_GETFL)&(O_APPEND|O_NONBLOCK))!=(O_APPEND|O_NONBLOCK))return 49;
    if(write(fd,"!",1)!=1||lseek(fd,0,2)!=14)return 15;
    if(ftruncate(fd,20)||fstat(fd,&information)||information.size!=20)return 16;
    char zeros[6]={1,1,1,1,1,1};if(pread(fd,zeros,6,14)!=6)return 17;
    for(unsigned i=0;i<6;++i)if(zeros[i])return 18;
    if(ftruncate(fd,11)||fstatat(AT_FDCWD,"/home/fileabi/state",&information,0)||information.size!=11)return 19;
    if(fsync(fd)!=-1||*__errno_location()!=95||fdatasync(fd)!=-1||*__errno_location()!=95||
       access("/home/fileabi/state",6))return 20;
    char cwd[2];if(!getcwd(cwd,sizeof(cwd))||cwd[0]!='/'||cwd[1])return 21;
    if(pathconf("/home/fileabi/state",3)!=31||pathconf("/home/fileabi/state",4)!=127)return 22;
    if(!isatty(1)||isatty(fd)||*__errno_location()!=25)return 23;
    int exclusive=openat(AT_FDCWD,"/home/fileabi/exclusive",O_CREAT|O_EXCL|O_RDWR,0666);
    if(exclusive<64||write(exclusive,"kept",4)!=4||close(exclusive))return 24;
    if(openat(AT_FDCWD,"/home/fileabi/exclusive",O_CREAT|O_EXCL|O_TRUNC|O_RDWR,0666)!=-1||
       *__errno_location()!=17||fstatat(AT_FDCWD,"/home/fileabi/exclusive",&information,0)||information.size!=4)return 25;
    if(close(fd)||close(copy)||close(high)||close(cloexec))return 26;
    if(read(fd,left,1)!=-1||*__errno_location()!=9)return 27;
    int recycled=openat(AT_FDCWD,"/home/fileabi/state",O_RDWR,0);
    if(recycled!=fd)return 28;
    if(close(recycled))return 30;
    if(readv(-1,0,0)!=0||writev(-1,0,0)!=0)return 29;
    void *directory=opendir("/home/fileabi");if(!directory)return 31;
    struct dirent_abi *entry;int found=0;while((entry=readdir(directory)))if(equal(entry->name,"state"))found=1;
    if(!found||closedir(directory))return 32;
    int directory_fd=openat(AT_FDCWD,"/home/fileabi",O_DIRECTORY,0);if(directory_fd<64)return 33;
    directory=fdopendir(directory_fd);if(!directory)return 34;
    struct dirent_abi supplied,*result=0;
    if(readdir_r(directory,&supplied,&result)||!result||closedir(directory))return 35;
    char file_template[]="/home/fileabi/tmpXXXXXX";int temporary=mkstemp(file_template);
    if(temporary<64||write(temporary,"temp",4)!=4||close(temporary))return 36;
    if(access(file_template,0)||unlinkat(AT_FDCWD,file_template,0)||access(file_template,0)!=-1)return 37;
    char directory_template[]="/home/fileabi/dirXXXXXX";
    if(mkdtemp(directory_template)!=directory_template||unlinkat(AT_FDCWD,directory_template,AT_REMOVEDIR))return 38;
    char resolved[128];if(realpath("/home/fileabi/../fileabi/state",resolved)!=resolved||
       !equal(resolved,"/home/fileabi/state"))return 39;
    if(realpath("/home/fileabi/state/../state",resolved)||*__errno_location()!=20)return 40;
    int asset=openat(AT_FDCWD,"/assets/large",0,0);if(asset<64)return 41;
    if(fstat(asset,&information)||information.size!=131072)return 42;
    unsigned char block[700];if(pread(asset,block,sizeof(block),333)!=sizeof(block))return 43;
    for(unsigned i=0;i<sizeof(block);++i)if(block[i]!=(unsigned char)((333+i)%251))return 44;
    if(pread(asset,block,sizeof(block),65530)!=sizeof(block))return 50;
    for(unsigned i=0;i<sizeof(block);++i)if(block[i]!=(unsigned char)((65530+i)%251))return 51;
    if(lseek(asset,-16,2)!=131056||read(asset,block,32)!=16)return 45;
    if(write(asset,"x",1)!=-1||*__errno_location()!=9)return 46;
    if(fsync(asset)||access("/assets/large",2)!=-1||*__errno_location()!=30)return 48;
    if(close(asset))return 47;
    print("ELF regular-file descriptor ABI and shared lifetime PASS\n");
    return 38;
}
