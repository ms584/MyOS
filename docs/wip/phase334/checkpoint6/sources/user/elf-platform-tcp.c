#include <stddef.h>
#include <stdint.h>
#define NONBLOCK 04000
#define CLOEXEC 02000000
struct address { uint16_t family;unsigned char port[2],ip[4],pad[8]; };
struct timespec { int64_t seconds,nanoseconds; };
struct pollfd { int fd;uint16_t events,revents; };
struct event { uint32_t events;uint64_t data __attribute__((packed)); } __attribute__((packed));
extern int socket(int,int,int),connect(int,const void *,uint32_t),shutdown(int,int),close(int),dup(int),fcntl(int,int,...);
extern int64_t send(int,const void *,size_t,int),recv(int,void *,size_t,int),write(int,const void *,size_t);
extern int *__errno_location(void),poll(struct pollfd *,uint64_t,int),ioctl_FIONREAD(int,int *);
extern int getsockname(int,void *,uint32_t *),getpeername(int,void *,uint32_t *),getsockopt(int,int,int,void *,uint32_t *);
extern int epoll_create1(int),epoll_ctl(int,int,int,struct event *),epoll_wait(int,struct event *,int,int);
extern int pthread_create(uint64_t *,const void *,void *(*)(void *),void *),pthread_join(uint64_t,void **);
extern int clock_nanosleep(int,int,const struct timespec *,struct timespec *);
static struct address address(unsigned port)
{struct address a={2,{(unsigned char)(port>>8),(unsigned char)port},{10,0,2,2},{0}};return a;}
static int begin(unsigned port,int nonblock)
{
    int fd=socket(2,1|CLOEXEC|(nonblock?NONBLOCK:0),6);if(fd<0)return -1;
    struct address a=address(port);int rc=connect(fd,&a,sizeof(a));
    if(nonblock){if(rc!=-1||*__errno_location()!=115){close(fd);return -1;}
        struct pollfd p={fd,4,0};if(poll(&p,1,7000)!=1){close(fd);return -1;}
        int error=-1;uint32_t size=4;
        if(getsockopt(fd,1,4,&error,&size)||error||fcntl(fd,4,2L)){close(fd);return -1;}}
    else if(rc){close(fd);return -1;}
    return fd;
}
struct worker { int fd;unsigned side,result; };
static void *transfer(void *raw)
{
    struct worker *w=raw;unsigned char buffer[1700];size_t sent=0,received=0;
    while(sent<32768){size_t count=sizeof(buffer);if(count>32768-sent)count=32768-sent;
        for(size_t i=0;i<count;++i)buffer[i]=(unsigned char)((sent+i+w->side*73)%251);
        int64_t rc=send(w->fd,buffer,count,16384);if(rc<=0){w->result=1;return 0;}sent+=(size_t)rc;}
    if(shutdown(w->fd,1)){w->result=2;return 0;}
    if(send(w->fd,"z",1,16384)!=-1||*__errno_location()!=32){w->result=3;return 0;}
    while(received<20000){int64_t rc=recv(w->fd,buffer,sizeof(buffer),0);if(rc<=0){w->result=4;return 0;}
        for(int64_t i=0;i<rc;++i)if(buffer[i]!=(unsigned char)((received+(size_t)i+w->side*19)%253)){w->result=5;return 0;}
        received+=(size_t)rc;}
    if(received!=20000||recv(w->fd,buffer,1,0)!=0)w->result=6;
    return 0;
}
static volatile unsigned waiting;
static int waited_fd,waited_error;static int64_t waited_result;
static void *reader(void *unused)
{(void)unused;char byte;__atomic_store_n(&waiting,1,__ATOMIC_RELEASE);
 waited_result=recv(waited_fd,&byte,1,0);waited_error=*__errno_location();return 0;}
uint64_t _start(void)
{
#ifdef TCP_NIC_ABSENT
    int fd=socket(2,1,0);struct address a=address(8080);
    if(fd<0||connect(fd,&a,sizeof(a))!=-1||*__errno_location()!=101||close(fd))return 79;
    return 45;
#elif defined(TCP_LINK_LOSS)
    waited_fd=begin(PORT_C,0);if(waited_fd<0)return 90;
    char byte;if(send(waited_fd,"A",1,16384)!=1||recv(waited_fd,&byte,1,0)!=1||byte!='a')return 91;
    uint64_t thread;if(pthread_create(&thread,0,reader,0))return 92;
    while(!__atomic_load_n(&waiting,__ATOMIC_ACQUIRE)){}
    static const char ready[]="TCP idle link-loss ready\n";write(1,ready,sizeof(ready)-1);
    struct pollfd event={waited_fd,1,0};
    if(poll(&event,1,7000)!=1||(event.revents&8)==0)return 93;
    if(pthread_join(thread,0)||waited_result!=-1||waited_error!=100)return 94;
    int error=-1;uint32_t size=4;
    if(getsockopt(waited_fd,1,4,&error,&size)||error!=100)return 95;
    if(getsockopt(waited_fd,1,4,&error,&size)||error!=0)return 96;
    if(send(waited_fd,"x",1,16384)!=-1||*__errno_location()!=100||close(waited_fd))return 97;
    static const char recover[]="TCP link recovery ready\n";write(1,recover,sizeof(recover)-1);
    struct timespec delay={2,0};clock_nanosleep(1,0,&delay,0);
    int fd=begin(PORT_C,0);if(fd<0)return 98;
    if(send(fd,"R",1,16384)!=1||recv(fd,&byte,1,0)!=1||byte!='r'||close(fd))return 99;
    static const char done[]="TCP idle carrier loss/blocked recv/poll/reconnect PASS\n";
    write(1,done,sizeof(done)-1);return 46;
#elif defined(TCP_CANCEL)
    waited_fd=begin(PORT_C,0);if(waited_fd<0)return 80;
    uint64_t thread;if(pthread_create(&thread,0,reader,0))return 81;
    while(!__atomic_load_n(&waiting,__ATOMIC_ACQUIRE)){}
    write(1,"TCP blocked worker ready\n",25);
    for(;;){struct timespec pause={1,0};clock_nanosleep(1,0,&pause,0);}
#else
    if(socket(10,1,0)!=-1||*__errno_location()!=97)return 1;
    if(socket(2,2,0)!=-1||*__errno_location()!=93)return 2;
    int occupied[32];for(unsigned i=0;i<32;++i){occupied[i]=socket(2,1,0);if(occupied[i]<0)return 40;}
    if(socket(2,1,0)!=-1||*__errno_location()!=24)return 41;
    struct address unbound;uint32_t unbound_size=sizeof(unbound);
    if(getsockname(occupied[0],&unbound,&unbound_size)||unbound.port[0]||unbound.port[1]||
       unbound.ip[0]||unbound.ip[1]||unbound.ip[2]||unbound.ip[3])return 48;
    for(unsigned i=0;i<32;++i)if(close(occupied[i]))return 42;
    int fd[2]={begin(PORT_A,1),begin(PORT_B,0)};if(fd[0]<0||fd[1]<0)return 3;
    struct address local[2],peer;uint32_t size=sizeof(peer);
    if(getpeername(fd[0],&peer,&size)||size!=16||peer.port[0]!=(PORT_A>>8)||peer.port[1]!=(PORT_A&255))return 4;
    for(unsigned i=0;i<2;++i){size=sizeof(local[i]);if(getsockname(fd[i],&local[i],&size)||local[i].family!=2)return 5;}
    if(local[0].port[0]==local[1].port[0]&&local[0].port[1]==local[1].port[1])return 6;
    int option=-7;size=4;
    if(getsockopt(fd[0],1,3,&option,&size)||option!=1||size!=4)return 43;
    if(getsockopt(fd[0],1,7,&option,&size)||option!=4096)return 44;
    if(getsockopt(fd[0],1,8,&option,&size)||option!=8192)return 45;
    option=-7;size=4;if(getsockopt(fd[0],1,99,&option,&size)!=-1||*__errno_location()!=92||option!=-7||size!=4)return 46;
    unsigned char short_option[4]={0xaa,0xaa,0xaa,0xaa};size=2;
    if(getsockopt(fd[0],1,4,short_option,&size)||size!=2||short_option[0]||short_option[1]||
       short_option[2]!=0xaa||short_option[3]!=0xaa)return 47;
    size=0;if(getsockopt(fd[0],1,4,0,&size)||size)return 49;
    unsigned char tiny[4]={0xaa,0xaa,0xaa,0xaa};size=2;
    if(getsockname(fd[0],tiny,&size)||size!=16||tiny[2]!=0xaa||tiny[3]!=0xaa)return 7;
    char byte; if(recv(fd[0],&byte,1,64)!=-1||*__errno_location()!=11)return 8;
    int ep=epoll_create1(0);if(ep<0)return 9;
    for(unsigned i=0;i<2;++i){struct event e={1|0x2000,i+1};if(epoll_ctl(ep,1,fd[i],&e))return 10;}
    struct event events[2];if(epoll_wait(ep,events,2,30)!=0)return 11;
    if(send(fd[0],"A",1,16384)!=1||send(fd[1],"B",1,16384)!=1)return 12;
    unsigned seen=0;
    while(seen!=3){int n=epoll_wait(ep,events,2,5000);if(n<=0)return 13;
        for(int i=0;i<n;++i){unsigned side=(unsigned)events[i].data-1;if(side>1)return 14;
            int available;if(ioctl_FIONREAD(fd[side],&available)||available<1)return 15;
            if(recv(fd[side],&byte,1,2)!=1||byte!=(char)('a'+side))return 16;
            if(recv(fd[side],&byte,1,0)!=1||byte!=(char)('a'+side))return 17;
            seen|=1u<<side;}}
    int duplicate=dup(fd[0]);if(duplicate<0||close(fd[0]))return 18;fd[0]=duplicate;
    struct worker workers[2]={{fd[0],0,0},{fd[1],1,0}};uint64_t threads[2];
    for(unsigned i=0;i<2;++i)if(pthread_create(&threads[i],0,transfer,&workers[i]))return 19;
    for(unsigned i=0;i<2;++i)if(pthread_join(threads[i],0)||workers[i].result)return 20+workers[i].result;
    if(close(fd[0])||close(fd[1])||close(ep))return 27;
    int bad=socket(2,1|NONBLOCK,0);struct address a=address(PORT_REFUSED);
    if(bad<0||connect(bad,&a,sizeof(a))!=-1||*__errno_location()!=115)return 28;
    struct pollfd p={bad,4,0};if(poll(&p,1,7000)!=1||(p.revents&8)==0)return 29;
    int error;size=4;if(getsockopt(bad,1,4,&error,&size)||error!=111)return 30;
    if(getsockopt(bad,1,4,&error,&size)||error!=0||close(bad))return 31;
    waited_fd=begin(PORT_C,0);if(waited_fd<0)return 32;
    duplicate=dup(waited_fd);uint64_t thread;waiting=0;
    if(duplicate<0||pthread_create(&thread,0,reader,0))return 33;
    struct timespec pause={0,30000000};while(!__atomic_load_n(&waiting,__ATOMIC_ACQUIRE))clock_nanosleep(1,0,&pause,0);
    clock_nanosleep(1,0,&pause,0);
    if(close(waited_fd)||pthread_join(thread,0)||waited_result!=-1||waited_error!=9)return 34;
    int reused=socket(2,1,0);if(reused!=waited_fd||close(reused))return 35;
    if(send(duplicate,"Q",1,16384)!=1||recv(duplicate,&byte,1,0)!=1||byte!='q'||close(duplicate))return 36;
    static const char message[]="TCP concurrent streams/readiness/EOF/cancel PASS\n";
    write(1,message,sizeof(message)-1);return 44;
#endif
}
