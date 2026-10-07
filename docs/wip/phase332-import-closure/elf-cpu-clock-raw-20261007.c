#include <stdint.h>
#include <stddef.h>
struct timespec { int64_t tv_sec,tv_nsec; };
extern int clock_gettime(int,struct timespec *);
extern int clock_nanosleep(int,int,const struct timespec *,struct timespec *);
extern int pthread_create(uint64_t *,const void *,void *(*)(void *),void *);
extern int pthread_join(uint64_t,void **);
extern int *__errno_location(void);
extern void SbLogRaw(const char *);
static uint64_t now(int id)
{struct timespec t;if(clock_gettime(id,&t))return UINT64_MAX;return t.tv_sec*1000000000ull+t.tv_nsec;}
static volatile uint64_t worker_start,worker_end;
static void burn(uint64_t duration)
{uint64_t until=now(1)+duration;while(now(1)<until)__asm__ volatile("":::"memory");}
static void *worker(void *unused)
{(void)unused;worker_start=now(3);burn(500000000ull);worker_end=now(3);return 0;}
#define CHECK(x) do{if(!(x)){SbLogRaw("CPU clock FAIL: " #x "\n");return __LINE__;}}while(0)
int _start(void)
{
 struct {uint64_t before;struct timespec value;uint64_t after;} guarded={0xabc123,{-3,-7},0xabc123};
 CHECK(clock_gettime(99,&guarded.value)==-1&&*__errno_location()==22);
 CHECK(guarded.value.tv_sec==-3&&guarded.value.tv_nsec==-7&&guarded.before==guarded.after);
 CHECK(clock_gettime(3,0)==-1&&*__errno_location()==14);
 uint64_t initial=now(3);CHECK(initial!=UINT64_MAX);burn(300000000ull);
 uint64_t active=now(3);CHECK(active>initial);
 uint64_t tid;CHECK(!pthread_create(&tid,0,worker,0));
 uint64_t parent=now(3),process=now(2),wall=now(1);
 struct timespec sleep={0,700000000};CHECK(!clock_nanosleep(1,0,&sleep,0));
 CHECK(!pthread_join(tid,0));
 CHECK(worker_start<=10000000ull&&worker_end>=worker_start+10000000ull);
 CHECK(now(3)<=parent+50000000ull); /* sleeping parent is not wall-clock charged */
 CHECK(now(2)>=process+10000000ull&&now(1)>=wall+700000000ull);
 CHECK(now(2)>=now(3)&&now(2)!=UINT64_MAX);
 /* Recycled worker slot starts with fresh accounting. */
 CHECK(!pthread_create(&tid,0,worker,0)&&!pthread_join(tid,0));
 CHECK(worker_start<=10000000ull&&worker_end>=worker_start+10000000ull);
 CHECK(guarded.before==0xabc123&&guarded.after==0xabc123);
 SbLogRaw("native sampled CPU clocks busy/sleep/process/thread/reuse PASS\n");return 89;
}
