#include <stddef.h>
#include <stdint.h>
struct timeval { int64_t seconds,microseconds; };
struct timespec { int64_t seconds,nanoseconds; };
struct address { uint16_t family;unsigned char port[2],ip[4],pad[8]; };
struct pollfd { int fd;uint16_t events,revents; };
extern int socket(int,int,int),socketpair(int,int,int,int *),close(int),dup(int),eventfd(unsigned,int);
extern int setsockopt(int,int,int,const void *,uint32_t),getsockopt(int,int,int,void *,uint32_t *);
extern int connect(int,const void *,uint32_t),shutdown(int,int),fcntl(int,int,...),poll(struct pollfd *,uint64_t,int);
extern int64_t send(int,const void *,size_t,int),recv(int,void *,size_t,int),read(int,void *,size_t),write(int,const void *,size_t);
extern int *__errno_location(void),clock_gettime(int,struct timespec *),clock_nanosleep(int,int,const struct timespec *,struct timespec *);
extern int pthread_create(uint64_t *,const void *,void *(*)(void *),void *),pthread_join(uint64_t,void **);
static void pause_ms(unsigned ms)
{struct timespec t={ms/1000,(ms%1000)*1000000};clock_nanosleep(1,0,&t,0);}
static uint64_t now_ms(void)
{struct timespec t;if(clock_gettime(1,&t))return UINT64_MAX;return (uint64_t)t.seconds*1000+(uint64_t)t.nanoseconds/1000000;}
static int timeout_set(int fd,int option,int64_t seconds,int64_t usec)
{struct timeval t={seconds,usec};return setsockopt(fd,1,option,&t,sizeof(t));}
static int timeout_is(int fd,int option,int64_t seconds,int64_t usec)
{struct timeval t={-2,-2};uint32_t n=sizeof(t);return !getsockopt(fd,1,option,&t,&n)&&n==16&&t.seconds==seconds&&t.microseconds==usec;}
static int timed_empty(int fd,int using_read,uint64_t minimum,uint64_t maximum)
{char byte;uint64_t start=now_ms();int64_t rc=using_read?read(fd,&byte,1):recv(fd,&byte,1,0);
 int error=*__errno_location();uint64_t elapsed=now_ms()-start;return rc==-1&&error==11&&minimum<=elapsed&&elapsed<=maximum;}
static int waking_fd,waited_fd,waited_error;static int64_t waited_result;
static uint64_t waited_elapsed;static volatile unsigned waiting,waking_error;
static void *waker(void *unused)
{(void)unused;for(unsigned i=0;i<12;++i){pause_ms(20);if(send(waking_fd,"W",1,16384)!=1)waking_error=1;}return 0;}
static void *reader(void *unused)
{(void)unused;char byte;uint64_t start=now_ms();__atomic_store_n(&waiting,1,__ATOMIC_RELEASE);
 waited_result=recv(waited_fd,&byte,1,0);waited_error=*__errno_location();waited_elapsed=now_ms()-start;return 0;}
static unsigned char buffer[8192];
static int64_t syscall3(uint64_t number,uint64_t first,uint64_t second,uint64_t third)
{
    register uint64_t result __asm__("rax")=number;
    __asm__ volatile("int $0x80":"+a"(result):"D"(first),"S"(second),"d"(third):"memory","cc");
    return (int64_t)result;
}
static uint64_t persistent_connect_error(void)
{
    uint64_t sequence=0,registry=0;
    struct raw_address { unsigned char ip[4];uint16_t port,reserved; };
    struct raw_status { uint32_t ready,available,error,state;struct raw_address local,peer; } status={0};
    struct raw_address peer={{10,0,2,2},REFUSED,0};
    int64_t handle=syscall3(35,(uintptr_t)&sequence,(uintptr_t)&registry,0);
    if(handle<0||syscall3(36,(uint64_t)handle,(uintptr_t)&peer,0)!=-115)return 70;
    uint64_t end=now_ms()+3000;
    do{
        if(syscall3(37,(uint64_t)handle,(uintptr_t)&status,0))return 71;
        if(status.error)break;
        pause_ms(10);
    }while(now_ms()<end);
    if(status.error!=111||status.state!=5)return 72;
    if(syscall3(37,(uint64_t)handle,(uintptr_t)&status,1)||status.error!=111)return 73;
    if(syscall3(37,(uint64_t)handle,(uintptr_t)&status,0)||status.error)return 74;
    if(syscall3(37,(uint64_t)handle,(uintptr_t)&status,2)||status.error!=111)return 75;
    if(syscall3(37,(uint64_t)handle,(uintptr_t)&status,0)||status.error||status.state!=5)return 76;
    if(syscall3(37,(uint64_t)handle,(uintptr_t)&status,3)!=-22||status.error)return 77;
    if(syscall3(41,(uint64_t)handle,0,0))return 78;
    return 0;
}
static uint64_t local_options(void)
{
    int p[2];if(socketpair(1,1,0,p))return 1;
    if(!timeout_is(p[0],20,0,0)||!timeout_is(p[1],21,0,0))return 2;
    int duplicate=dup(p[0]);if(duplicate<0)return 3;
    *__errno_location()=77;
    if(timeout_set(duplicate,20,0,80000)||*__errno_location()!=77||!timeout_is(p[0],20,0,80000)||!timeout_is(p[1],20,0,0))return 4;
    if(!timed_empty(p[0],0,70,240)||!timed_empty(duplicate,1,70,240))return 5;
    struct timeval invalid={0,1000000};
    if(setsockopt(p[0],1,20,&invalid,sizeof(invalid))!=-1||*__errno_location()!=33||!timeout_is(p[0],20,0,80000))return 6;
    invalid.microseconds=-1;
    if(setsockopt(p[0],1,20,&invalid,sizeof(invalid))!=-1||*__errno_location()!=33)return 7;
    if(setsockopt(p[0],1,20,&invalid,8)!=-1||*__errno_location()!=22)return 8;
    if(setsockopt(p[0],1,20,0,16)!=-1||*__errno_location()!=14)return 9;
    if(setsockopt(p[0],1,99,&invalid,16)!=-1||*__errno_location()!=92)return 10;
    if(setsockopt(-1,1,20,&invalid,16)!=-1||*__errno_location()!=9)return 11;
    int event=eventfd(0,0);
    if(event<0||setsockopt(event,1,20,&invalid,16)!=-1||*__errno_location()!=88||close(event))return 12;
    unsigned char short_output[20];for(unsigned i=0;i<20;++i)short_output[i]=0xaa;
    uint32_t n=9;
    if(getsockopt(p[0],1,20,short_output,&n)||n!=9)return 13;
    for(unsigned i=9;i<20;++i)if(short_output[i]!=0xaa)return 14;
    n=0;if(getsockopt(p[0],1,21,0,&n)||n)return 15;
    if(timeout_set(p[0],20,0,1)||!timeout_is(p[0],20,0,10000))return 16;
    if(timeout_set(p[0],20,-1,0)||!timeout_is(p[0],20,0,0)||!timed_empty(p[0],0,0,60))return 17;
    if(timeout_set(p[0],20,INT64_MAX,0)||!timeout_is(p[0],20,0,0))return 18;
    if(timeout_set(p[0],20,0,80000))return 19;
    uint64_t start=now_ms();char byte;
    if(recv(p[0],&byte,1,64)!=-1||*__errno_location()!=11||now_ms()-start>=70)return 20;
    if(fcntl(p[0],4,04000L))return 21;
    start=now_ms();
    if(recv(duplicate,&byte,1,0)!=-1||*__errno_location()!=11||now_ms()-start>=70||fcntl(p[0],4,0L))return 22;
    if(send(p[1],"K",1,16384)!=1||recv(duplicate,buffer,sizeof(buffer),0)!=1||buffer[0]!='K')return 23;
    if(timeout_set(p[0],21,0,80000)||!timeout_is(duplicate,21,0,80000)||!timeout_is(p[1],21,0,0))return 24;
    if(send(p[0],buffer,sizeof(buffer),16384)!=4096)return 25;
    start=now_ms();if(write(duplicate,"X",1)!=-1||*__errno_location()!=11)return 26;
    uint64_t elapsed=now_ms()-start;if(elapsed<70||elapsed>240)return 27;
    if(recv(p[1],buffer,sizeof(buffer),0)!=4096)return 28;
    /* Opposite-direction traffic wakes the same object but cannot renew recv's deadline. */
    waking_fd=p[0];waking_error=0;uint64_t thread;
    if(pthread_create(&thread,0,waker,0)||!timed_empty(p[0],0,70,200)||pthread_join(thread,0)||waking_error)return 29;
    if(recv(p[1],buffer,sizeof(buffer),0)!=12)return 30;
    /* Changes apply to future calls, not to an already captured deadline. */
    waited_fd=duplicate;waiting=0;
    if(pthread_create(&thread,0,reader,0))return 31;
    while(!__atomic_load_n(&waiting,__ATOMIC_ACQUIRE))pause_ms(1);
    pause_ms(20);if(timeout_set(p[0],20,0,500000)||pthread_join(thread,0)||waited_result!=-1||waited_error!=11||waited_elapsed>240)return 32;
    if(timeout_set(p[0],20,2,0))return 33;
    waited_fd=duplicate;waiting=0;if(pthread_create(&thread,0,reader,0))return 34;
    while(!__atomic_load_n(&waiting,__ATOMIC_ACQUIRE))pause_ms(1);
    pause_ms(30);if(close(duplicate))return 35;
    event=eventfd(0,0);if(event!=duplicate||pthread_join(thread,0)||waited_result!=-1||waited_error!=9||waited_elapsed>500||close(event))return 36;
    if(send(p[1],"Q",1,16384)!=1||recv(p[0],&byte,1,0)!=1||byte!='Q')return 37;
    if(timeout_set(p[0],20,-1,0)||shutdown(p[1],1)||recv(p[0],&byte,1,0)!=0)return 38;
    if(close(p[0])||close(p[1]))return 39;
    return 0;
}
uint64_t _start(void)
{
    uint64_t error=local_options();if(error)return error;
    error=persistent_connect_error();if(error)return error;
    int fd=socket(2,1,0);if(fd<0)return 40;
    struct address address={2,{(unsigned char)(PORT>>8),(unsigned char)PORT},{10,0,2,2},{0}};
    static const char connect_ready[]="TCP connect stall ready\n",connect_checked[]="TCP connect timeout checked\n";
    write(1,connect_ready,sizeof(connect_ready)-1);pause_ms(500);
    if(timeout_set(fd,21,0,80000))return 56;
    uint64_t start=now_ms();
    if(connect(fd,&address,sizeof(address))!=-1||*__errno_location()!=115)return 41;
    uint64_t elapsed=now_ms()-start;if(elapsed<70||elapsed>240)return 57;
    struct pollfd pending={fd,4,0};if(poll(&pending,1,0)!=0)return 58;
    write(1,connect_checked,sizeof(connect_checked)-1);pause_ms(500);
    if(timeout_set(fd,21,0,0)||poll(&pending,1,4000)!=1||(pending.revents&8))return 59;
    int connect_error=-1;uint32_t size=4;
    if(getsockopt(fd,1,4,&connect_error,&size)||connect_error)return 60;
    char byte;if(recv(fd,&byte,1,0)!=1||byte!='s')return 42;
    int duplicate=dup(fd);if(duplicate<0||timeout_set(duplicate,20,0,80000)||!timeout_is(fd,20,0,80000))return 43;
    if(!timed_empty(fd,0,70,240)||!timed_empty(duplicate,1,70,240))return 44;
    if(timeout_set(fd,21,0,80000)||!timeout_is(duplicate,21,0,80000))return 45;
    static const char send_ready[]="TCP send stall ready\n",send_checked[]="TCP send timeout checked\n";
    write(1,send_ready,sizeof(send_ready)-1);pause_ms(500);
    for(unsigned i=0;i<sizeof(buffer);++i)buffer[i]='Z';
    size_t filled=0;
    for(;;){int64_t count=send(fd,buffer,sizeof(buffer),16384|64);
        if(count>0){if(count>(int64_t)sizeof(buffer))return 46;filled+=(size_t)count;if(filled>65536)return 47;continue;}
        if(count!=-1||*__errno_location()!=11)return 48;
        struct pollfd p={fd,4,0};int ready=poll(&p,1,30);if(ready<0)return 49;if(!ready)break;
    }
    if(filled<4096)return 50;
    start=now_ms();
    if(send(duplicate,"X",1,16384)!=-1||*__errno_location()!=11)return 51;
    elapsed=now_ms()-start;if(elapsed<70||elapsed>240)return 52;
    write(1,send_checked,sizeof(send_checked)-1);
    if(timeout_set(fd,20,0,0)||shutdown(fd,1))return 53;
    if(recv(duplicate,buffer,sizeof(buffer),0)!=2||buffer[0]!='r'||buffer[1]!='d'||recv(fd,&byte,1,0)!=0)return 54;
    if(close(fd)||close(duplicate))return 55;
    static const char result[]="Socket timeval/deadline/dup/backpressure/cancel PASS\n";
    write(1,result,sizeof(result)-1);return 46;
}
