#include <stdint.h>
#include <stddef.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
static void fill(void *raw,unsigned char value,size_t length)
{unsigned char *p=raw;for(size_t i=0;i<length;++i)p[i]=value;}
static int differs(const void *a,const void *b,size_t length)
{const unsigned char *x=a,*y=b;for(size_t i=0;i<length;++i)if(x[i]!=y[i])return 1;return 0;}
static int names_differ(const char *a,const char *b)
{while(*a&&*a==*b){++a;++b;}return *a!=*b;}
#define PROT_READ 1
#define PROT_WRITE 2
#define MAP_PRIVATE 2
#define MAP_ANONYMOUS 0x20
#define MAP_FAILED ((void *)-1)
extern void *mmap(void *,size_t,int,int,int,off_t);
extern int mprotect(void *,size_t,int);
extern int munmap(void *,size_t);
extern int rename(const char *,const char *);
extern void SbLogRaw(const char *);
struct dirent_abi {uint64_t ino;int64_t offset;uint16_t length;unsigned char type;char name[256];};
extern void *opendir(const char *);
extern void *fdopendir(int);
extern struct dirent_abi *readdir(void *);
extern int closedir(void *);
struct flock_abi {int16_t type,whence;uint32_t padding;int64_t start,length;int32_t pid;uint32_t tail;};
_Static_assert(sizeof(struct flock_abi)==32&&offsetof(struct flock_abi,start)==8&&offsetof(struct flock_abi,pid)==24,"pinned flock ABI");
#define CHECK(x) do{if(!(x)){SbLogRaw("native FS runtime failure: " #x "\n");return __LINE__;}}while(0)
int _start(void)
{
 struct stat st;char buffer[16]={0};
 extern long sysconf(int);struct limit_abi {uint64_t current,maximum;};
 extern int getrlimit(int,struct limit_abi *);struct limit_abi limit;
 CHECK(sysconf(4)==192&&!getrlimit(7,&limit)&&limit.current==192&&limit.maximum==192);
 CHECK(!fstatat(AT_FDCWD,"/etc/motd",&st,0x100)&&S_ISREG(st.st_mode));
 fill(&st,0xa5,sizeof(st));struct stat unchanged=st;
 CHECK(fstatat(AT_FDCWD,"/etc/motd",&st,0x101)==-1&&errno==22&&!differs(&st,&unchanged,sizeof(st)));
 CHECK(fstatat(AT_FDCWD,"/home/no-nofollow-file",&st,0x100)==-1&&errno==2&&!differs(&st,&unchanged,sizeof(st)));
 int readonly=openat(AT_FDCWD,"/etc/motd",O_RDONLY|0x20000);CHECK(readonly>=0&&!close(readonly));
 CHECK(!mkdir("/home/runtime-src",0777));
 int dir=openat(AT_FDCWD,"/home/runtime-src",O_RDONLY|O_DIRECTORY);CHECK(dir>=0);
 int file=openat(dir,"file",O_RDWR|O_CREAT|O_EXCL,0600);CHECK(file>=0&&write(file,"source",6)==6);
 int old=openat(dir,"old",O_RDWR|O_CREAT,0600);CHECK(old>=0&&write(old,"old",3)==3);
 int alias=dup(dir);CHECK(alias>=0);
 void *stream=fdopendir(alias);CHECK(stream);
 CHECK(!rename("/home/runtime-src","/home/runtime-moved"));
 CHECK(!fstat(dir,&st)&&S_ISDIR(st.st_mode));
 CHECK(!fstatat(dir,"file",&st,0)&&st.st_size==6);
 CHECK(!fstatat(dir,"file",&st,0x100)&&st.st_size==6);
 CHECK(fstatat(dir,"file/../old",&st,0)==-1&&errno==20);
 CHECK(!rename("/home/runtime-moved/file","/home/runtime-moved/old"));
 CHECK(!fstat(old,&st)&&st.st_size==3&&!lseek(old,0,SEEK_SET)&&read(old,buffer,16)==3&&!differs(buffer,"old",3));
 struct dirent_abi *entry=readdir(stream);CHECK(entry&&!names_differ(entry->name,"old")&&!readdir(stream));
 CHECK(!closedir(stream));
 CHECK(!unlinkat(dir,"old",0)&&fstatat(dir,"old",&st,0)==-1&&errno==2);
 CHECK(!fstat(file,&st)&&st.st_size==6&&!close(file)&&!close(old));
 CHECK(!close(dir)&&!unlinkat(AT_FDCWD,"/home/runtime-moved",AT_REMOVEDIR));
 /* Retain stream's original open description across fd close/reuse. */
 CHECK(!mkdir("/home/runtime-stream",0777));
 dir=openat(AT_FDCWD,"/home/runtime-stream",O_RDONLY);CHECK(dir>=0);
 file=openat(dir,"kept",O_RDWR|O_CREAT,0600);CHECK(file>=0&&!close(file));
 stream=fdopendir(dir);CHECK(stream&&!close(dir));
 int reused=openat(AT_FDCWD,"/etc/motd",O_RDONLY);CHECK(reused==dir);
 entry=readdir(stream);CHECK(entry&&!names_differ(entry->name,"kept"));
 CHECK(closedir(stream)==-1&&errno==9); /* Must NOT close reused motd. */
 CHECK(read(reused,buffer,1)==1&&!close(reused));
 CHECK(!unlinkat(AT_FDCWD,"/home/runtime-stream/kept",0)&&!unlinkat(AT_FDCWD,"/home/runtime-stream",AT_REMOVEDIR));
 /* Kernel-pointer validation and process-owned advisory lock ABI. */
 file=openat(AT_FDCWD,"/home/runtime-lock",O_RDWR|O_CREAT,0600);CHECK(file>=0);
 struct {unsigned char before[16];struct flock_abi lock;unsigned char after[32];} guarded;
 fill(&guarded,0xa5,sizeof(guarded));guarded.lock=(struct flock_abi){1,SEEK_SET,0xffffffff,0,0,0,0xffffffff};
 CHECK(!fcntl(file,6,&guarded.lock));
 guarded.lock.type=1;CHECK(!fcntl(file,5,&guarded.lock)&&guarded.lock.type==2);
 for(unsigned i=0;i<16;++i)CHECK(guarded.before[i]==0xa5);
 for(unsigned i=0;i<32;++i)CHECK(guarded.after[i]==0xa5);
 CHECK(fcntl(file,5,(void *)1)==-1&&errno==14);
 unsigned char *pages=mmap(0,8192,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);CHECK(pages!=MAP_FAILED);
 fill(pages,0xa5,8192);CHECK(!mprotect(pages+4096,4096,PROT_READ));
 CHECK(fcntl(file,5,pages+4096-16)==-1&&errno==14);
 for(unsigned i=4096-16;i<4096+16;++i)CHECK(pages[i]==0xa5);
 CHECK(!munmap(pages,8192));
 guarded.lock=(struct flock_abi){2,SEEK_SET,0,0,0,0,0};CHECK(!fcntl(file,6,&guarded.lock));
 CHECK(fcntl(file,7,&guarded.lock)==-1&&errno==95); /* Waiting is not yet wired. */
 CHECK(!close(file)&&!unlinkat(AT_FDCWD,"/home/runtime-lock",0));
 /* Readonly virtual asset directory and root adapter stay usable. */
 stream=opendir("/assets");CHECK(stream&&readdir(stream)&&!closedir(stream));
 dir=openat(AT_FDCWD,"/",O_RDONLY|O_DIRECTORY);CHECK(dir>=0);
 stream=opendir("/");CHECK(stream&&readdir(stream)&&!closedir(stream));
 CHECK(!fstatat(dir,"assets",&st,0)&&S_ISDIR(st.st_mode)&&!close(dir));
 /* Exact kernel handle capacity, independent offsets, full failure and reuse.
  * All descriptors are closed before return; lifecycle checker also compares
  * physical pages, heap and remaining processes over four launches. */
 int handles[64];for(unsigned i=0;i<64;++i){
  handles[i]=openat(AT_FDCWD,"/etc/motd",O_RDONLY|0x20000);CHECK(handles[i]>=0);
 }
 CHECK(openat(AT_FDCWD,"/etc/motd",O_RDONLY)==-1&&errno==24);
 for(unsigned i=0;i<64;++i)CHECK(read(handles[i],buffer,1)==1);
 int released=handles[7];CHECK(!close(released));
 handles[7]=openat(AT_FDCWD,"/etc/motd",O_RDONLY);CHECK(handles[7]==released);
 for(unsigned i=0;i<64;++i)CHECK(!close(handles[i]));
 /* Fill all128 runtime slots using genuine eventfd objects. Failed open
  * must reserve before creating a kernel handle and return EMFILE. */
 extern int eventfd(unsigned int,int);int events[128];
 for(unsigned i=0;i<128;++i){events[i]=eventfd(0,0);CHECK(events[i]>=0);}
 CHECK(openat(AT_FDCWD,"/etc/motd",O_RDONLY)==-1&&errno==24);
 for(unsigned i=0;i<128;++i)CHECK(!close(events[i]));
 readonly=openat(AT_FDCWD,"/etc/motd",O_RDONLY);CHECK(readonly>=0&&!close(readonly));
 SbLogRaw("native nofollow and64 kernel/128 runtime descriptor capacity/recovery PASS\n");
 SbLogRaw("native runtime inode DIR/rename/open replacement/stream close-reuse/unlinkat/typed fcntl guards/root-assets PASS\n");return 88;
}
