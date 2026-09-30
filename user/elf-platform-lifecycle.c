#include <stddef.h>
#include <stdint.h>
struct timespec { int64_t seconds,nanoseconds; };
extern int clock_gettime(int,struct timespec *),clock_nanosleep(int,int,const struct timespec *,struct timespec *);
extern int pthread_create(uint64_t *,const void *,void *(*)(void *),void *);
extern int sem_init(void *,int,unsigned),sem_wait(void *);
extern int pthread_mutex_init(void *,const void *),pthread_mutex_lock(void *);
extern int pthread_cond_init(void *,const void *),pthread_cond_wait(void *,void *);
extern int pipe(int [2]),openat(int,const char *,int,...);
extern int64_t read(int,void *,size_t),write(int,const void *,size_t);
extern void *malloc(size_t);
static int ends[2];
static uint64_t semaphore[4],mutex[10],condition[12];
static volatile unsigned started;
static void print(const char *text)
{size_t length=0;while(text[length])++length;(void)write(1,text,length);}
static void *waiter(void *raw)
{
    uintptr_t kind=(uintptr_t)raw;char byte;
    __atomic_add_fetch(&started,1,__ATOMIC_RELEASE);
    if(kind==0)(void)read(ends[0],&byte,1);
    else if(kind==1)(void)sem_wait(semaphore);
    else {(void)pthread_mutex_lock(mutex);(void)pthread_cond_wait(condition,mutex);}
    /* Nothing wakes these workers normally; process stop must remove them. */
    for(;;){struct timespec pause={1,0};(void)clock_nanosleep(1,0,&pause,0);}
    return 0;
}
uint64_t _start(void)
{
    int fd=openat(-100,"/home/live-open",0100|2,0600);
    if(fd<0||write(fd,"held",4)!=4||pipe(ends)||sem_init(semaphore,0,0)||
       pthread_mutex_init(mutex,0)||pthread_cond_init(condition,0)||!malloc(32768))return 1;
    uint64_t threads[3];
    for(uintptr_t i=0;i<3;++i)if(pthread_create(&threads[i],0,waiter,(void *)i))return 2;
    struct timespec pause={0,200000000},begin,now;
    while(__atomic_load_n(&started,__ATOMIC_ACQUIRE)!=3)if(clock_nanosleep(1,0,&pause,0))return 3;
    if(clock_nanosleep(1,0,&pause,0)||clock_gettime(1,&begin))return 4;
    print("ELF live workers blocked PASS\n");
    int reported=0;
    for(;;){
        if(clock_nanosleep(1,0,&pause,0)||clock_gettime(1,&now))return 5;
        if(!reported&&now.seconds-begin.seconds>=61){
            reported=1;print("ELF live runtime beyond 60 seconds PASS\n");
        }
    }
}
