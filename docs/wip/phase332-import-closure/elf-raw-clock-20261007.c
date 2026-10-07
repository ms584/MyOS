#include <stdint.h>
struct timespec {int64_t seconds,nanoseconds;};
extern int clock_gettime(int,struct timespec *);
extern int clock_nanosleep(int,int,const struct timespec *,struct timespec *);
extern int *__errno_location(void);
extern void SbLogRaw(const char *);
extern int pthread_create(uint64_t *,const void *,void *(*)(void *),void *);
extern int pthread_join(uint64_t,void **);
static int64_t nanos(struct timespec t){return t.seconds*1000000000+t.nanoseconds;}
static unsigned worker_ok;
static void *worker(void *unused)
{(void)unused;struct timespec raw,mono;worker_ok=!clock_gettime(4,&raw)&&!clock_gettime(1,&mono)&&nanos(mono)>=nanos(raw)&&nanos(mono)-nanos(raw)<100000000;return 0;}
#define CHECK(x) do{if(!(x)){SbLogRaw("Raw clock FAIL: " #x "\n");return __LINE__;}}while(0)
int _start(void)
{
 struct {uint64_t before;struct timespec value;uint64_t after;} guarded={42,{7,9},99};
 CHECK(!clock_gettime(4,&guarded.value));
 CHECK(guarded.before==42&&guarded.after==99&&guarded.value.nanoseconds>=0&&guarded.value.nanoseconds<1000000000);
 CHECK(guarded.value.nanoseconds%10000000==0);
 struct timespec first=guarded.value,interval={0,50000000},mono;
 CHECK(!clock_nanosleep(1,0,&interval,0));
 CHECK(!clock_gettime(4,&guarded.value)&&nanos(guarded.value)>=nanos(first)+50000000);
 CHECK(!clock_gettime(1,&mono)&&nanos(mono)>=nanos(guarded.value));
 CHECK(nanos(mono)-nanos(guarded.value)<100000000);
 CHECK(clock_gettime(4,0)==-1&&*__errno_location()==14);
 guarded.value=(struct timespec){7,9};
 CHECK(clock_gettime(99,&guarded.value)==-1&&*__errno_location()==22&&guarded.value.seconds==7&&guarded.value.nanoseconds==9);
 CHECK(clock_nanosleep(4,0,&interval,0)==22);
 uint64_t tid;CHECK(!pthread_create(&tid,0,worker,0)&&!pthread_join(tid,0)&&worker_ok);
 SbLogRaw("native raw monotonic PIT/ABI/guards/elapsed/worker/relaunch PASS\n");return 94;
}
