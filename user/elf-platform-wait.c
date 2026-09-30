#include <stddef.h>
#include <stdint.h>

#define O_NONBLOCK 04000
#define EFD_SEMAPHORE 1
#define POLLIN 0x0001
#define POLLOUT 0x0004
#define POLLHUP 0x0010
#define EPOLL_CTL_ADD 1
#define EPOLL_CTL_DEL 2

struct timespec { int64_t tv_sec,tv_nsec; };
struct pollfd { int32_t fd;uint16_t events,revents; };
struct epoll_event { uint32_t events;uint64_t data __attribute__((packed)); } __attribute__((packed));
extern int *__errno_location(void);
extern int eventfd(unsigned,int),pipe(int [2]),pipe2(int [2],int),close(int);
extern int64_t read(int,void *,size_t),write(int,const void *,size_t);
extern int poll(struct pollfd *,uint64_t,int32_t);
extern int epoll_create1(int),epoll_ctl(int,int,int,struct epoll_event *);
extern int epoll_wait(int,struct epoll_event *,int,int),ioctl_FIONREAD(int,int *);
extern int pthread_create(uint64_t *,const void *,void *(*)(void *),void *),pthread_join(uint64_t,void **);
extern int clock_nanosleep(int,int,const struct timespec *,struct timespec *);

static int cancel_pipe[2],cancel_errno;
static int64_t cancel_result;
static void *blocked_reader(void *unused)
{
    (void)unused;unsigned char byte;
    cancel_result=read(cancel_pipe[0],&byte,1);cancel_errno=*__errno_location();return 0;
}
static void print(const char *text)
{
    size_t length=0;while(text[length])++length;(void)write(1,text,length);
}
uint64_t _start(void)
{
    uint64_t value=0;int available=-1;
    int event=eventfd(2,EFD_SEMAPHORE|O_NONBLOCK);if(event<0)return 1;
    if(ioctl_FIONREAD(event,&available)||available!=8||read(event,&value,8)!=8||value!=1)return 2;
    int epoll=epoll_create1(0);if(epoll<0)return 3;
    struct epoll_event registration={POLLIN,0x1122334455667788ull},ready={0,0};
    if(epoll_ctl(epoll,EPOLL_CTL_ADD,event,&registration)||epoll_wait(epoll,&ready,1,0)!=1||
       ready.events!=POLLIN||ready.data!=registration.data)return 4;
    if(read(event,&value,8)!=8||value!=1||epoll_wait(epoll,&ready,1,0)!=0)return 5;
    if(read(event,&value,8)!=-1||*__errno_location()!=11)return 6;
    value=5;if(write(event,&value,8)!=8||epoll_wait(epoll,&ready,1,0)!=1)return 7;
    if(epoll_ctl(epoll,EPOLL_CTL_DEL,event,0)||close(event)||close(epoll))return 8;

    int ends[2];if(pipe2(ends,O_NONBLOCK))return 9;
    struct pollfd descriptors[2]={{ends[0],POLLIN,0},{ends[1],POLLOUT,0}};
    if(poll(descriptors,2,0)!=1||descriptors[0].revents||descriptors[1].revents!=POLLOUT)return 10;
    const char message[]="abc";char copy[4]={0};
    if(write(ends[1],message,3)!=3||ioctl_FIONREAD(ends[0],&available)||available!=3)return 11;
    descriptors[0].revents=descriptors[1].revents=0;
    if(poll(descriptors,2,0)!=2||!(descriptors[0].revents&POLLIN))return 12;
    if(read(ends[0],copy,3)!=3||copy[0]!='a'||copy[2]!='c')return 13;
    descriptors[0].revents=0;if(poll(descriptors,1,20)!=0)return 14;
    if(close(ends[1]))return 15;
    descriptors[0].revents=0;
    if(poll(descriptors,1,0)!=1||!(descriptors[0].revents&POLLHUP)||read(ends[0],copy,1)!=0)return 16;
    if(close(ends[0]))return 17;

    if(pipe(cancel_pipe))return 18;
    uint64_t thread;
    if(pthread_create(&thread,0,blocked_reader,0))return 19;
    struct timespec pause={0,20000000};if(clock_nanosleep(1,0,&pause,0))return 20;
    if(close(cancel_pipe[0])||pthread_join(thread,0))return 21;
    if(cancel_result!=-1||cancel_errno!=9||close(cancel_pipe[1]))return 22;

    for(unsigned i=0;i<64;++i){int descriptor=eventfd(i,0);if(descriptor<0||close(descriptor))return 23;}
    print("ELF descriptor lifetime, eventfd, pipe, poll, epoll and FIONREAD PASS\n");
    return 36;
}
