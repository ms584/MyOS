#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <stdint.h>
#include <stddef.h>
extern int pthread_kill(pthread_t,int);
extern int pause(void);
extern void SbLogRaw(const char *);
extern int sem_init(void *,int,unsigned);
extern int sem_destroy(void *);
extern int sem_wait(void *);
extern int pipe(int [2]);
extern int close(int);
extern int64_t read(int,void *,size_t);
struct pollfd {int fd;short events,revents;};
struct epoll_event {uint32_t events;uint64_t data;} __attribute__((packed));
extern int poll(struct pollfd *,uint64_t,int);
extern int epoll_create1(int);
extern int epoll_ctl(int,int,int,const struct epoll_event *);
extern int epoll_wait(int,struct epoll_event *,int,int);
#define CHECK(x) do{if(!(x)){SbLogRaw("cross signal failed: " #x "\n");__builtin_trap();}}while(0)
#ifndef EXIT_MAIN_CASE
#define EXIT_MAIN_CASE 0
#endif
static pthread_t target,main_thread;
static volatile unsigned ready,release_worker,hits;
static unsigned mode;
static unsigned expected_signal=SIGUSR1;
static volatile uint64_t native_tid;
static uint64_t semaphore;
static int descriptors[2],epfd;
static pthread_mutex_t mutex;
static pthread_cond_t condition;
static void delay(unsigned ms)
{struct timespec t={ms/1000,(long)(ms%1000)*1000000};CHECK(!clock_nanosleep(1,0,&t,0));}
static void handler(int number)
{CHECK(number==(int)expected_signal&&pthread_equal(pthread_self(),target));__atomic_add_fetch(&hits,1,__ATOMIC_RELEASE);}
static int64_t raw(unsigned operation,uint64_t tid,uint64_t signal_number,uint64_t reserved)
{register uint64_t a __asm__("rax")=49,t __asm__("r10")=reserved;
 __asm__ volatile("int $0x80":"+a"(a):"D"((uint64_t)operation),"S"(tid),"d"(signal_number),"r"(t):"memory","cc");return (int64_t)a;}
static uint64_t current_tid(void)
{register uint64_t a __asm__("rax")=28;
 __asm__ volatile("int $0x80":"+a"(a)::"rdi","rsi","rdx","r10","memory","cc");return a;}
static void *worker(void *unused)
{
 (void)unused;
 CHECK(raw(7,0,0,0)==-EINVAL);
 __atomic_store_n(&native_tid,current_tid(),__ATOMIC_RELEASE);
 if(mode==1){sigset_t set;CHECK(!sigemptyset(&set)&&!sigaddset(&set,SIGUSR1));
  CHECK(!pthread_sigmask(SIG_BLOCK,&set,0));__atomic_store_n(&ready,1,__ATOMIC_RELEASE);
  while(!__atomic_load_n(&release_worker,__ATOMIC_ACQUIRE))delay(10);
  CHECK(!hits);CHECK(!pthread_sigmask(SIG_UNBLOCK,&set,0));CHECK(hits==1);
  __atomic_store_n(&ready,2,__ATOMIC_RELEASE);return (void *)96;}
 if(mode==7)CHECK(!pthread_mutex_lock(&mutex));
 __atomic_store_n(&ready,1,__ATOMIC_RELEASE);
 if(mode==0||mode==9)CHECK(pause()==-1&&errno==EINTR);
 else if(mode==2){struct timespec t={2,0},left={-1,-1};errno=123;
  CHECK(clock_nanosleep(1,0,&t,&left)==EINTR&&errno==123);
  CHECK(left.tv_sec>=0&&left.tv_nsec>=0&&left.tv_nsec<1000000000&&(left.tv_sec||left.tv_nsec));}
 else if(mode==3)CHECK(sem_wait(&semaphore)==-1&&errno==EINTR&&semaphore==0);
 else if(mode==4){unsigned char byte;CHECK(read(descriptors[0],&byte,1)==-1&&errno==EINTR);}
 else if(mode==5){struct pollfd p={descriptors[0],1,0};CHECK(poll(&p,1,2000)==-1&&errno==EINTR);}
 else if(mode==6){struct epoll_event event;CHECK(epoll_wait(epfd,&event,1,2000)==-1&&errno==EINTR);}
 else if(mode==7){CHECK(!pthread_cond_wait(&condition,&mutex));
  CHECK(pthread_mutex_trylock(&mutex)==EBUSY);CHECK(!pthread_mutex_unlock(&mutex));}
 else if(mode==8){struct timespec until;CHECK(!clock_gettime(0,&until));until.tv_sec+=2;
  CHECK(!pthread_mutex_timedlock(&mutex,&until));CHECK(!pthread_mutex_unlock(&mutex));}
 CHECK(hits==1);__atomic_store_n(&ready,2,__ATOMIC_RELEASE);return (void *)96;
}
static void *send_main(void *unused)
{(void)unused;delay(50);CHECK(!pthread_kill(main_thread,SIGUSR1));return (void *)97;}
static void *terminate_main(void *unused)
{(void)unused;delay(50);CHECK(!pthread_kill(main_thread,SIGTERM));return 0;}
static void *observe_main_exit(void *unused)
{(void)unused;void *result;CHECK(!pthread_join(main_thread,&result)&&result==(void *)99);
 errno=555;CHECK(pthread_kill(main_thread,0)==ESRCH&&errno==555);
 CHECK(pthread_kill(main_thread,SIGUSR1)==ESRCH&&errno==555);
 CHECK(raw(5,native_tid,0,0)==-ESRCH&&raw(5,native_tid,SIGUSR1,0)==-ESRCH);
 CHECK(!hits);SbLogRaw("native exited main identity/signal liveness PASS\n");return 0;}
int _start(void)
{
 CHECK(signal(SIGUSR1,handler)!=SIG_ERR);main_thread=pthread_self();
 if(EXIT_MAIN_CASE==1){native_tid=current_tid();target=main_thread;pthread_t observer;
  CHECK(!pthread_create(&observer,0,observe_main_exit,0));pthread_exit((void *)99);}
 if(EXIT_MAIN_CASE==2){mode=0;CHECK(!pthread_create(&target,0,worker,0));
  while(!__atomic_load_n(&ready,__ATOMIC_ACQUIRE)){delay(10);}
  delay(50);SbLogRaw("native targeted default termination ready\n");
  CHECK(!pthread_kill(target,SIGTERM));for(;;){delay(100);}}
 if(EXIT_MAIN_CASE==3){target=main_thread;pthread_t sender;
  CHECK(!pthread_create(&sender,0,terminate_main,0));
  SbLogRaw("native targeted default termination ready\n");
  for(;;){(void)pause();}}
 CHECK(!pthread_mutex_init(&mutex,0)&&!pthread_cond_init(&condition,0));
 CHECK(!sem_init(&semaphore,0,0));CHECK(!pipe(descriptors));
 epfd=epoll_create1(0);CHECK(epfd>=0);struct epoll_event event={1,123};
 CHECK(!epoll_ctl(epfd,1,descriptors[0],&event));
 for(mode=0;mode<10;++mode){ready=release_worker=hits=0;
  if(mode==9){expected_signal=SIGUSR2;CHECK(signal(SIGUSR1,SIG_IGN)!=SIG_ERR);
   CHECK(signal(SIGUSR2,handler)!=SIG_ERR);}
  if(mode==8)CHECK(!pthread_mutex_lock(&mutex));
  CHECK(!pthread_create(&target,0,worker,0));
  while(!__atomic_load_n(&ready,__ATOMIC_ACQUIRE))delay(10);
  delay(50);errno=111;CHECK(!pthread_kill(target,0)&&errno==111);
  uint64_t tid=__atomic_load_n(&native_tid,__ATOMIC_ACQUIRE);
  CHECK(!raw(5,tid,0,0)&&raw(5,tid,0,1)==-EINVAL);
  CHECK(raw(5,tid,(UINT64_C(1)<<32)|SIGUSR1,0)==-EINVAL);
  CHECK(raw(5,0,0,0)==-ESRCH&&raw(5,tid^(UINT64_C(1)<<32),0,0)==-ESRCH);
  CHECK(raw(6,1,0,0)==-EINVAL&&errno==111);
  CHECK(pthread_kill(target,-1)==EINVAL&&errno==111);
  CHECK(!pthread_kill(target,SIGUSR1)&&errno==111);
  if(mode==1){CHECK(!pthread_kill(target,SIGUSR1)&&!hits);__atomic_store_n(&release_worker,1,__ATOMIC_RELEASE);}
  if(mode==8){for(unsigned i=0;i<100&&!hits;++i)delay(10);CHECK(hits==1);CHECK(!pthread_mutex_unlock(&mutex));}
  if(mode==9){delay(50);CHECK(!hits&&ready==1);CHECK(!pthread_kill(target,SIGUSR2));}
  for(unsigned i=0;i<100&&ready!=2;++i){delay(10);}
  CHECK(ready==2);delay(30);
  CHECK(raw(5,tid,0,0)==-ESRCH&&pthread_kill(target,0)==ESRCH);
  void *value;CHECK(!pthread_join(target,&value)&&value==(void *)96);
  CHECK(pthread_kill(target,0)==ESRCH);CHECK(hits==1);
 }
 expected_signal=SIGUSR1;CHECK(signal(SIGUSR1,handler)!=SIG_ERR);
 hits=0;target=main_thread;pthread_t sender;CHECK(!pthread_create(&sender,0,send_main,0));
 CHECK(pause()==-1&&errno==EINTR&&hits==1);void *value;
 CHECK(!pthread_join(sender,&value)&&value==(void *)97);
 CHECK(!close(epfd)&&!close(descriptors[0])&&!close(descriptors[1]));
 CHECK(!sem_destroy(&semaphore)&&!pthread_cond_destroy(&condition)&&!pthread_mutex_destroy(&mutex));
 SbLogRaw("native targeted signals/masks/pause/sleep/sem/pipe/poll/epoll/pthread waits PASS\n");return 96;
}
void finalizer(void){SbLogRaw("cross signal finalizer called\n");}
