#include <stddef.h>
#include <stdint.h>
struct iovec {void *base;size_t length;};
struct message {void *name;uint32_t namelen,pad;struct iovec *vectors;int32_t count,pad1;
 void *control;uint32_t controllen,pad2;int32_t flags,pad3;};
_Static_assert(sizeof(struct message)==56,"message ABI");
extern int socketpair(int,int,int,int *),close(int),eventfd(unsigned,int),*__errno_location(void);
extern int64_t sendmsg(int,const struct message *,int),recvmsg(int,struct message *,int);
extern int64_t send(int,const void *,size_t,int),read(int,void *,size_t),write(int,const void *,size_t);
extern int pthread_create(uint64_t *,const void *,void *(*)(void *),void *),pthread_join(uint64_t,void **);
struct timespec {int64_t seconds,nanoseconds;};
struct multi_message {struct message message;uint32_t length,padding;};
_Static_assert(sizeof(struct multi_message)==64,"multi message ABI");
extern int64_t sendto(int,const void *,size_t,int,const void *,uint32_t);
extern int recvmmsg(int,struct multi_message *,unsigned,int,struct timespec *);
extern int clock_nanosleep(int,int,const struct timespec *,struct timespec *);
static int waited_fd,wait_error;
static int64_t wait_result;
static volatile unsigned entered;
static void *waiter(void *unused)
{
 (void)unused;char data[2];struct iovec v={data,sizeof(data)};
 struct message m={.vectors=&v,.count=1};
 __atomic_store_n(&entered,1,__ATOMIC_RELEASE);
 struct multi_message mm={.message=m};struct timespec timeout={1,0};
 wait_result=recvmmsg(waited_fd,&mm,1,0,&timeout);wait_error=*__errno_location();return 0;
}
uint64_t _start(void)
{
 int fd[2];if(socketpair(1,1,0,fd))return 1;
 struct iovec output[3]={{"ab",2},{0,0},{"cde",3}};
 struct message m={.vectors=output,.count=3};
 if(sendmsg(fd[0],&m,16384)!=5)return 2;
 char a[2]={0},b[6]={'x','x','x','x','x','x'},name[3]={0,0,'Z'};
 struct iovec input[3]={{a,1},{0,0},{b,5}};
 m=(struct message){.name=name,.namelen=1,.vectors=input,.count=3,.controllen=100,.flags=99};
 if(recvmsg(fd[1],&m,2)!=5||a[0]!='a'||b[0]!='b'||b[3]!='e'||b[4]!='x'||
    m.namelen!=2||name[0]!=1||name[1]!=0||name[2]!='Z'||m.controllen||m.flags)return 3;
 m=(struct message){.vectors=input,.count=3};
 if(recvmsg(fd[1],&m,0)!=5||b[3]!='e')return 4;
 if(recvmsg(fd[1],&m,64)!=-1||*__errno_location()!=11)return 5;
 if(sendmsg(fd[0],0,0)!=-1||*__errno_location()!=14)return 6;
 m=(struct message){.vectors=output,.count=-1};
 if(sendmsg(fd[0],&m,0)!=-1||*__errno_location()!=22)return 7;
 m.count=1025;if(sendmsg(fd[0],&m,0)!=-1||*__errno_location()!=22)return 8;
 struct iovec huge[2]={{"a",INT64_MAX},{"b",1}};
 m=(struct message){.vectors=huge,.count=2};
 if(sendmsg(fd[0],&m,0)!=-1||*__errno_location()!=22)return 9;
 m=(struct message){.vectors=output,.count=3,.controllen=1};
 if(sendmsg(fd[0],&m,0)!=-1||*__errno_location()!=95)return 10;
 m.controllen=0;m.name=name;
 if(sendmsg(fd[0],&m,0)!=-1||*__errno_location()!=106)return 11;
 m.name=0;if(sendmsg(fd[0],&m,0x40000000)!=-1||*__errno_location()!=95)return 12;
 if(recvmsg(fd[1],&m,64)!=-1||*__errno_location()!=11)return 13;
 int event=eventfd(0,0);if(event<0)return 14;
 if(sendmsg(event,&m,0)!=-1||*__errno_location()!=88||close(event))return 15;
 waited_fd=fd[1];entered=0;uint64_t worker;
 if(pthread_create(&worker,0,waiter,0))return 16;
 struct timespec delay={0,20000000};
 while(!__atomic_load_n(&entered,__ATOMIC_ACQUIRE))if(clock_nanosleep(1,0,&delay,0))return 17;
 if(clock_nanosleep(1,0,&delay,0)||close(fd[1]))return 18;
 event=eventfd(0,0);if(event!=fd[1]||pthread_join(worker,0)||wait_result!=-1||wait_error!=9)return 19;
 if(close(event)||close(fd[0]))return 20;
 if(socketpair(1,1,0,fd))return 21;
 if(sendto(fd[0],"abcdef",6,16384,0,0)!=6)return 22;
 char bytes[8]={0};struct iovec vectors[3]={{bytes,2},{bytes+2,2},{bytes+4,2}};
 struct multi_message mm[3]={0};
 for(int i=0;i<3;++i){mm[i].message.vectors=&vectors[i];mm[i].message.count=1;mm[i].length=77;}
 if(recvmmsg(fd[1],mm,3,64,0)!=3)return 23;
 for(int i=0;i<3;++i)if(mm[i].length!=2)return 24;
 for(int i=0;i<6;++i)if(bytes[i]!='a'+i)return 25;
 if(recvmmsg(fd[1],mm,1,64,0)!=-1||*__errno_location()!=11)return 26;
 struct timespec timeout={0,20000000},before={0},after={0};
 extern int clock_gettime(int,struct timespec *);
 if(clock_gettime(1,&before)||recvmmsg(fd[1],mm,1,0,&timeout)!=0||clock_gettime(1,&after))return 27;
 int64_t elapsed=(after.seconds-before.seconds)*1000000000+after.nanoseconds-before.nanoseconds;
 if(elapsed<20000000||elapsed>500000000||timeout.seconds||timeout.nanoseconds)return 28;
 timeout=(struct timespec){-1,0};
 if(recvmmsg(fd[1],mm,1,0,&timeout)!=-1||*__errno_location()!=22)return 29;
 if(recvmmsg(fd[1],mm,0,0,0)!=-1||*__errno_location()!=22)return 30;
 if(sendto(fd[0],"x",1,0,name,2)!=-1||*__errno_location()!=106)return 31;
 if(sendto(fd[0],"xy",2,0,0,0)!=2)return 32;
 mm[1].length=77;
 if(recvmmsg(fd[1],mm,3,0x10000,0)!=1||mm[0].length!=2||mm[1].length!=77)return 33;
 struct timeval {int64_t seconds,microseconds;} receive_timeout={0,20000};
 extern int setsockopt(int,int,int,const void *,uint32_t);
 if(setsockopt(fd[1],1,20,&receive_timeout,sizeof(receive_timeout)))return 34;
 timeout=(struct timespec){1,0};
 if(clock_gettime(1,&before)||recvmmsg(fd[1],mm,1,0,&timeout)!=-1||*__errno_location()!=11||clock_gettime(1,&after))return 35;
 elapsed=(after.seconds-before.seconds)*1000000000+after.nanoseconds-before.nanoseconds;
 if(elapsed<20000000||elapsed>500000000||(!timeout.seconds&&!timeout.nanoseconds))return 36;
 if(close(fd[0])||close(fd[1]))return 37;
 const char success[]="native message stream/pin/scatter/peek/close-reuse PASS\n";
 write(1,success,sizeof(success)-1);return 88;
}
