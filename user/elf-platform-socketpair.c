#include <stddef.h>
#include <stdint.h>

#define NONBLOCK 04000
#define CLOEXEC 02000000
#define PEEK 2
#define DONTWAIT 64
#define NOSIGNAL 16384
#define IN 1
#define OUT 4
#define HUP 16
#define RDHUP 0x2000
struct timespec { int64_t seconds,nanoseconds; };
struct pollfd { int32_t fd;uint16_t events,revents; };
struct epoll_event { uint32_t events;uint64_t data __attribute__((packed)); } __attribute__((packed));
extern int socketpair(int,int,int,int [2]),shutdown(int,int),close(int),dup(int),fcntl(int,int,...);
extern int64_t send(int,const void *,size_t,int),recv(int,void *,size_t,int);
extern int64_t read(int,void *,size_t),write(int,const void *,size_t);
extern int *__errno_location(void),ioctl_FIONREAD(int,int *),poll(struct pollfd *,uint64_t,int32_t);
extern int eventfd(unsigned,int),epoll_create1(int),epoll_ctl(int,int,int,struct epoll_event *);
extern int epoll_wait(int,struct epoll_event *,int,int);
extern int pthread_create(uint64_t *,const void *,void *(*)(void *),void *),pthread_join(uint64_t,void **);
extern int clock_nanosleep(int,int,const struct timespec *,struct timespec *);
static int blocked_fd,blocked_errno,blocked_writing;
static int64_t blocked_result;
static volatile unsigned started;
static void *blocked_reader(void *unused)
{
    (void)unused;char byte='w';__atomic_store_n(&started,1,__ATOMIC_RELEASE);
    blocked_result=blocked_writing?send(blocked_fd,&byte,1,NOSIGNAL):recv(blocked_fd,&byte,1,0);
    blocked_errno=*__errno_location();return 0;
}
static int start_waiter(uint64_t *thread,int fd,int writing)
{
    blocked_fd=fd;blocked_writing=writing;started=0;blocked_errno=0;blocked_result=-99;
    if(pthread_create(thread,0,blocked_reader,0))return -1;
    struct timespec pause={0,20000000};
    while(!__atomic_load_n(&started,__ATOMIC_ACQUIRE))if(clock_nanosleep(1,0,&pause,0))return -1;
    return clock_nanosleep(1,0,&pause,0);
}
struct reader { int fd;volatile unsigned started;int64_t result;char byte; };
static void *parallel_reader(void *raw)
{
    struct reader *r=raw;__atomic_store_n(&r->started,1,__ATOMIC_RELEASE);
    r->result=read(r->fd,&r->byte,1);return 0;
}
uint64_t _start(void)
{
    int fd[2],available=-1;char buffer[4096]={0};
    if(socketpair(2,1,0,fd)!=-1||*__errno_location()!=97)return 1;
    if(socketpair(1,2,0,fd)!=-1||*__errno_location()!=93)return 2;
    if(socketpair(1,1|NONBLOCK|CLOEXEC,0,fd))return 3;
    if(fcntl(fd[0],1)!=1||!(fcntl(fd[0],3)&NONBLOCK))return 4;
    if(recv(fd[0],buffer,1,0)!=-1||*__errno_location()!=11)return 5;
    int duplicate=dup(fd[0]);if(duplicate<0||fcntl(duplicate,1)!=0)return 6;
    if(fcntl(duplicate,4,2L)||(fcntl(fd[0],3)&NONBLOCK)||!(fcntl(fd[1],3)&NONBLOCK))return 7;
    if(recv(duplicate,buffer,1,DONTWAIT)!=-1||*__errno_location()!=11)return 8;
    int ep=epoll_create1(0);struct epoll_event registration={IN|RDHUP,0x123456789abcdef0ull},event={0,0};
    if(ep<0||epoll_ctl(ep,1,duplicate,&registration)||epoll_wait(ep,&event,1,0))return 9;
    if(send(fd[1],"abc",3,NOSIGNAL)!=3||ioctl_FIONREAD(duplicate,&available)||available!=3)return 10;
    if(recv(fd[0],buffer,2,PEEK)!=2||buffer[0]!='a'||buffer[1]!='b'||
       ioctl_FIONREAD(duplicate,&available)||available!=3)return 11;
    if(epoll_wait(ep,&event,1,0)!=1||event.events!=IN||event.data!=registration.data)return 12;
    if(close(fd[0])||read(duplicate,buffer,3)!=3||buffer[2]!='c')return 13;
    if(write(duplicate,"xy",2)!=2||recv(fd[1],buffer,2,0)!=2||buffer[1]!='y')return 14;
    if(send(fd[1],buffer,sizeof(buffer),NOSIGNAL)!=4096)return 15;
    struct pollfd writable={fd[1],OUT,0};
    if(poll(&writable,1,0)||send(fd[1],"z",1,NOSIGNAL)!=-1||*__errno_location()!=11)return 16;
    if(read(duplicate,buffer,1)!=1||poll(&writable,1,0)!=1||!(writable.revents&OUT))return 17;
    if(read(duplicate,buffer,4096)!=4095||epoll_wait(ep,&event,1,0))return 18;
    if(shutdown(fd[1],1)||epoll_wait(ep,&event,1,0)!=1||(event.events&(IN|RDHUP))!=(IN|RDHUP))return 19;
    if(read(duplicate,buffer,1)!=0||send(fd[1],"z",1,NOSIGNAL)!=-1||*__errno_location()!=32)return 20;
    if(write(duplicate,"r",1)!=1||read(fd[1],buffer,1)!=1||buffer[0]!='r')return 21;
    if(shutdown(fd[1],0)||send(duplicate,"z",1,NOSIGNAL)!=-1||*__errno_location()!=32)return 22;
    writable=(struct pollfd){duplicate,OUT,0};
    if(poll(&writable,1,0)!=1||(writable.revents&(OUT|HUP))!=(OUT|HUP))return 23;
    if(shutdown(duplicate,3)!=-1||*__errno_location()!=22)return 24;
    if(close(duplicate)||epoll_wait(ep,&event,1,0)||close(fd[1])||close(ep))return 25;

    /* Closing only the waited-on descriptor cancels it, not its duplicate. */
    if(socketpair(1,1,0,fd))return 26;
    duplicate=dup(fd[0]);uint64_t thread;
    if(duplicate<0||start_waiter(&thread,fd[0],0)||close(fd[0])||pthread_join(thread,0))return 27;
    if(blocked_result!=-1||blocked_errno!=9||write(fd[1],"k",1)!=1||read(duplicate,buffer,1)!=1)return 28;
    int reused=eventfd(0,NONBLOCK);if(reused!=fd[0])return 29;
    if(close(reused)||close(duplicate)||read(fd[1],buffer,1)!=0||close(fd[1]))return 30;
    if(socketpair(1,1,0,fd)||start_waiter(&thread,fd[0],0)||shutdown(fd[1],1)||pthread_join(thread,0))return 31;
    if(blocked_result!=0||close(fd[0])||close(fd[1]))return 32;

    /* Second-slot failure must roll back the first install and heap allocation. */
    int occupied[31];for(unsigned i=0;i<31;++i){occupied[i]=eventfd(0,0);if(occupied[i]<0)return 33;}
    int unchanged[2]={-7,-8};
    if(socketpair(1,1,0,unchanged)!=-1||*__errno_location()!=24||unchanged[0]!=-7||unchanged[1]!=-8)return 34;
    if(close(occupied[30])||socketpair(1,1,0,fd)||close(fd[0])||close(fd[1]))return 35;
    for(unsigned i=0;i<30;++i)if(close(occupied[i]))return 36;
    for(unsigned i=0;i<64;++i)if(socketpair(1,1,0,fd)||close(fd[0])||close(fd[1]))return 37;

    if(socketpair(1,1,0,fd)||write(fd[0],buffer,4096)!=4096||start_waiter(&thread,fd[0],1))return 41;
    if(read(fd[1],buffer,1)!=1||pthread_join(thread,0)||blocked_result!=1)return 42;
    if(ioctl_FIONREAD(fd[1],&available)||available!=4096||close(fd[0])||close(fd[1]))return 43;
    if(socketpair(1,1,0,fd)||write(fd[0],buffer,4096)!=4096||start_waiter(&thread,fd[0],1))return 44;
    if(close(fd[0])||pthread_join(thread,0)||blocked_result!=-1||blocked_errno!=9)return 45;
    if(read(fd[1],buffer,4096)!=4096||read(fd[1],buffer,1)!=0||close(fd[1]))return 46;

    if(socketpair(1,1,0,fd))return 47;
    struct reader readers[2]={{fd[0],0,-99,0},{fd[0],0,-99,0}};uint64_t threads[2];
    for(unsigned i=0;i<2;++i)if(pthread_create(&threads[i],0,parallel_reader,&readers[i]))return 48;
    struct timespec pause={0,20000000};
    while(!__atomic_load_n(&readers[0].started,__ATOMIC_ACQUIRE)||!__atomic_load_n(&readers[1].started,__ATOMIC_ACQUIRE))
        if(clock_nanosleep(1,0,&pause,0))return 49;
    if(clock_nanosleep(1,0,&pause,0)||write(fd[1],"AB",2)!=2)return 50;
    for(unsigned i=0;i<2;++i)if(pthread_join(threads[i],0)||readers[i].result!=1)return 51;
    if(!((readers[0].byte=='A'&&readers[1].byte=='B')||(readers[0].byte=='B'&&readers[1].byte=='A')))return 52;
    if(close(fd[0])||close(fd[1]))return 53;
    const char marker[]="ELF socketpair duplex, shutdown, readiness and cancellation PASS\n";
    (void)write(1,marker,sizeof(marker)-1);return 40;
}
