#include <stdint.h>
#include <stddef.h>
struct fs_timespec {int64_t seconds,nanoseconds;};
extern int chmod(const char *,uint32_t),fchmod(int,uint32_t),fchown(int,uint32_t,uint32_t);
extern int utimensat(int,const char *,const struct fs_timespec[2],int);
extern int symlink(const char *,const char *),inotify_init1(int);
extern int inotify_add_watch(int,const char *,uint32_t),inotify_rm_watch(int,int);
extern int64_t readlink(const char *,char *,size_t);
extern int *__errno_location(void);
extern void SbLogRaw(const char *);
extern int pthread_create(uint64_t *,const void *,void *(*)(void *),void *);
extern int pthread_join(uint64_t,void **);
static unsigned worker_ok;
static void *worker(void *unused)
{(void)unused;worker_ok=inotify_init1(0)==-1&&*__errno_location()==38;return 0;}
#define CHECK(x) do{if(!(x)){SbLogRaw("FS unavailable FAIL: " #x "\n");return __LINE__;}}while(0)
#define UNAVAILABLE(x) do{*__errno_location()=0;CHECK((x)==-1&&*__errno_location()==38);}while(0)
int _start(void)
{
 struct fs_timespec times[2]={{7,8},{9,10}};
 struct {uint64_t before;char data[64];uint64_t after;} buffer={.before=42,.after=99};
 for(unsigned i=0;i<64;++i)buffer.data[i]=(char)0xa5;
 for(unsigned repeat=0;repeat<64;++repeat){
  UNAVAILABLE(chmod("/etc/motd",0600));UNAVAILABLE(fchmod(-1,0600));
  UNAVAILABLE(fchown(-1,0,0));UNAVAILABLE(utimensat(-100,"/etc/motd",times,0));
  UNAVAILABLE(symlink("/etc/motd","/home/unavailable-link"));
  UNAVAILABLE(readlink("/etc/motd",buffer.data,sizeof(buffer.data)));
  UNAVAILABLE(inotify_init1(0));UNAVAILABLE(inotify_add_watch(-1,"/home",1));
  UNAVAILABLE(inotify_rm_watch(-1,1));
 }
 CHECK(buffer.before==42&&buffer.after==99&&times[0].seconds==7&&times[1].nanoseconds==10);
 for(unsigned i=0;i<64;++i)CHECK((unsigned char)buffer.data[i]==0xa5);
 UNAVAILABLE(readlink(0,0,0));UNAVAILABLE(chmod(0,0));
 *__errno_location()=17;uint64_t tid;
 CHECK(!pthread_create(&tid,0,worker,0)&&!pthread_join(tid,0)&&worker_ok);
 CHECK(*__errno_location()==17);
 SbLogRaw("native unavailable POSIX9 ENOSYS/no outputs/no descriptors/thread errno/relaunch PASS\n");return 93;
}
