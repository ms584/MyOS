#include <stdint.h>
#include <stddef.h>
struct limit {uint64_t current,maximum;};
extern int getrlimit(int,struct limit *);
extern int *__errno_location(void);
extern void SbLogRaw(const char *);
extern int pthread_create(uint64_t *,const void *,void *(*)(void *),void *);
extern int pthread_join(uint64_t,void **);
extern void *mmap(void *,size_t,int,int,int,int64_t);
extern int munmap(void *,size_t);
static volatile unsigned worker_ok;
static void *worker(void *unused)
{(void)unused;struct limit l;worker_ok=!getrlimit(2,&l)&&l.current==UINT64_MAX&&l.maximum==UINT64_MAX;return 0;}
#define CHECK(x) do{if(!(x)){SbLogRaw("Data limit FAIL: " #x "\n");return __LINE__;}}while(0)
int _start(void)
{
 struct {uint64_t before;struct limit value;uint64_t after;} guarded={0xabc123,{7,9},0xabc123};
 CHECK(!getrlimit(2,&guarded.value)&&guarded.value.current==UINT64_MAX&&guarded.value.maximum==UINT64_MAX);
 CHECK(guarded.before==0xabc123&&guarded.after==0xabc123);
 CHECK(getrlimit(2,0)==-1&&*__errno_location()==14);
 guarded.value=(struct limit){7,9};CHECK(getrlimit(99,&guarded.value)==-1&&*__errno_location()==22);
 CHECK(guarded.value.current==7&&guarded.value.maximum==9&&guarded.after==0xabc123);
 void *memory=mmap(0,4096,3,34,-1,0);CHECK(memory!=(void *)-1);
 *(volatile uint64_t *)memory=42;CHECK(!getrlimit(2,&guarded.value)&&guarded.value.current==UINT64_MAX);
 CHECK(!munmap(memory,4096));
 uint64_t tid;CHECK(!pthread_create(&tid,0,worker,0)&&!pthread_join(tid,0)&&worker_ok);
 SbLogRaw("native data rlimit policy/ABI/guards/VM independence/worker PASS\n");return 91;
}
