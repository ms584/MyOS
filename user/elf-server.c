#include <stddef.h>
#include <stdint.h>
#include <signal.h>
struct address {uint16_t family;unsigned char port[2],ip[4],padding[8];};
struct pollfd {int fd;int16_t events,revents;};
struct epoll_event {uint32_t events;uint64_t data;} __attribute__((packed));
struct timespec {int64_t seconds,nanoseconds;};
struct timeval {int64_t seconds,microseconds;};
extern int socket(int,int,int),bind(int,const void *,uint32_t),listen(int,int),accept(int,void *,uint32_t *);
extern int close(int),dup(int),eventfd(unsigned,int),fcntl(int,int,...),poll(struct pollfd *,size_t,int);
extern int getsockname(int,void *,uint32_t *),getpeername(int,void *,uint32_t *);
extern int setsockopt(int,int,int,const void *,uint32_t),shutdown(int,int),*__errno_location(void);
extern int64_t read(int,void *,size_t),write(int,const void *,size_t);
extern int pthread_create(uint64_t *,const void *,void *(*)(void *),void *),pthread_join(uint64_t,void **);
extern int clock_nanosleep(int,int,const struct timespec *,struct timespec *);
extern int pthread_kill(uint64_t,int),epoll_create1(int),epoll_ctl(int,int,int,const struct epoll_event *);
extern int epoll_wait(int,struct epoll_event *,int,int);
static void message(const char *s){size_t n=0;while(s[n])++n;write(1,s,n);}
static int waiting_fd,waiting_result,waiting_error;
static volatile unsigned entered;
static volatile unsigned hits;
static void handler(int number){if(number==SIGUSR1)__atomic_add_fetch(&hits,1,__ATOMIC_RELEASE);}
static void *waiter(void *unused){(void)unused;__atomic_store_n(&entered,1,__ATOMIC_RELEASE);
 waiting_result=accept(waiting_fd,0,0);waiting_error=*__errno_location();return 0;}
static int new_listener(void){int fd=socket(2,1,0);struct address a={2,{0xb4,1},{0},{0}};
 if(fd<0||bind(fd,&a,sizeof(a))||listen(fd,2))return -1;
 return fd;}
uint64_t _start(void)
{
 int fd=socket(2,1|2048|524288,0);if(fd<0)return 1;
 struct address local={2,{0xb4,0},{0},{0}},name={0};uint32_t length=sizeof(name);
 if(bind(fd,&local,sizeof(local))||getsockname(fd,&name,&length)||length!=16||name.port[0]!=0xb4||name.port[1])return 2;
 int other=socket(2,1,0);if(other<0)return 3;
 if(bind(other,&local,sizeof(local))!=-1||*__errno_location()!=98)return 4;
 local.ip[0]=192;if(bind(other,&local,16)!=-1||*__errno_location()!=99)return 5;
 local.ip[0]=0;local.port[0]=local.port[1]=0;
 if(bind(other,&local,16)||getsockname(other,&name,&length)||(!name.port[0]&&!name.port[1])||close(other))return 6;
 if(listen(fd,-1)!=-1||*__errno_location()!=22||listen(fd,2)||listen(fd,2))return 7;
 if(accept(fd,0,0)!=-1||*__errno_location()!=11)return 8;
 int e=eventfd(0,0);if(e<0||listen(e,1)!=-1||*__errno_location()!=88||close(e))return 9;
 if(listen(-1,1)!=-1||*__errno_location()!=9)return 10;
 if(fcntl(fd,4,0))return 11;
 struct timeval timeout={0,30000};
 if(setsockopt(fd,1,20,&timeout,16)||accept(fd,0,0)!=-1||*__errno_location()!=11)return 12;
 timeout.microseconds=0;if(setsockopt(fd,1,20,&timeout,16))return 13;
#ifdef CANCEL_ONLY
 message("native server cancel ready\n");
 for(;;){int child=accept(fd,0,0);if(child<0)return 14;}
#else
 message("native server ready\n");
 struct pollfd p={fd,1,0};if(poll(&p,1,5000)!=1||!(p.revents&1))return 15;
 int ep=epoll_create1(0);struct epoll_event ev={1,1234},seen={0};
 if(ep<0||epoll_ctl(ep,1,fd,&ev)||epoll_wait(ep,&seen,1,1000)!=1||!(seen.events&1)||seen.data!=1234||close(ep))return 32;
 /* Table exhaustion must not consume the pending peer. */
 int filled[32],count=0;
 while(count<32){int n=eventfd(0,0);if(n<0)break;filled[count++]=n;}
 if(count!=31||accept(fd,0,0)!=-1||*__errno_location()!=24)return 16;
 if(close(filled[--count]))return 17;
 unsigned char truncated[4]={0,0,0,0xa5};length=3;
 int first=accept(fd,truncated,&length);
 if(first<0||length!=16||truncated[0]!=2||truncated[1]||truncated[3]!=0xa5)return 18;
 if(fcntl(first,3)!=2||fcntl(first,1)!=0)return 19;
 for(int i=0;i<count;++i)if(close(filled[i]))return 20;
 int alias=dup(fd);if(alias<0||close(fd))return 21;
 int second=accept(alias,0,0);if(second<0||close(alias))return 22;
 /* Accepted children survive the final listener close, retain real peer. */
 length=16;if(getpeername(first,&name,&length)||name.family!=2||name.ip[0]!=10||name.ip[3]!=2)return 23;
 int children[2]={first,second};
 for(unsigned c=0;c<2;++c){unsigned total=0;unsigned char data[233];
   while(total<4096){size_t n=4096-total;if(n>sizeof(data))n=sizeof(data);
     int64_t got=read(children[c],data,n);if(got<=0)return 24;
     for(int64_t j=0;j<got;++j)if(data[j]!=(unsigned char)((total+j)*17+c))return 25;
     int64_t sent=0;while(sent<got){int64_t nout=write(children[c],data+sent,(size_t)(got-sent));if(nout<=0)return 26;sent+=nout;}
     total+=(unsigned)got;
   }
   /* Peer confirms all queued bytes arrived before process teardown. */
   if(read(children[c],data,1)!=1||data[0]!=0xa5||read(children[c],data,1)!=0||
      shutdown(children[c],1)||close(children[c]))return 27;
 }
 waiting_fd=new_listener();if(waiting_fd<0)return 28;uint64_t worker;entered=0;
 if(pthread_create(&worker,0,waiter,0))return 29;
 struct timespec delay={0,20000000};while(!__atomic_load_n(&entered,__ATOMIC_ACQUIRE))clock_nanosleep(1,0,&delay,0);
 if(clock_nanosleep(1,0,&delay,0)||close(waiting_fd))return 30;
 e=eventfd(0,0);if(e!=waiting_fd||pthread_join(worker,0)||waiting_result!=-1||waiting_error!=9||close(e))return 31;
 waiting_fd=new_listener();if(waiting_fd<0)return 33;
 if(signal(SIGUSR1,handler)==SIG_ERR)return 34;
 entered=hits=0;if(pthread_create(&worker,0,waiter,0))return 35;
 while(!__atomic_load_n(&entered,__ATOMIC_ACQUIRE))clock_nanosleep(1,0,&delay,0);
 if(clock_nanosleep(1,0,&delay,0)||pthread_kill(worker,SIGUSR1)||pthread_join(worker,0)||
    waiting_result!=-1||waiting_error!=4||hits!=1||close(waiting_fd))return 36;
 message("native TCP server handshake/queue/EMFILE/echo/close-reuse PASS\n");return 88;
#endif
}
