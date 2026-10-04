/* Test-only transparent link wrappers; never a runtime implementation. */
#include <pthread.h>
#include <unistd.h>
#include <stdint.h>
#include <stddef.h>
static unsigned count;
static void trace(const char *label,uintptr_t first,uintptr_t second)
{
    unsigned sequence=__atomic_fetch_add(&count,1,__ATOMIC_RELAXED);
    /* Keep late locks/once/guards visible; cap only allocation chatter. */
    int large=(label[6]=='m'||label[6]=='c')&&label[12]==0&&((label[6]=='m'&&first>=65536)||
                           (label[6]=='c'&&second&&first>=65536/second));
    if(sequence>=96&&label[6]!='l'&&label[6]!='o'&&label[6]!='g'&&label[6]!='f'&&!large)return;
    char text[112];unsigned n=0;
    while(*label)text[n++]=*label++;
    for(unsigned argument=0;argument<2;++argument){
        uintptr_t value=argument?second:first;text[n++]=' ';
        for(int shift=60;shift>=0;shift-=4)text[n++]="0123456789abcdef"[(value>>shift)&15];
    }
    text[n++]='\n';write(1,text,n);
}
extern void *__real_malloc(size_t);
extern void *__real_calloc(size_t,size_t);
extern int __real_pthread_mutex_lock(pthread_mutex_t *);
extern int __real_pthread_once(pthread_once_t *,void (*)(void));
extern int __real___cxa_guard_acquire(uint64_t *);
extern void __real___cxa_guard_release(uint64_t *);
void *__wrap_malloc(size_t size)
{trace("trace malloc",size,0);void *p=__real_malloc(size);trace("trace malloc done",(uintptr_t)p,0);
 if(!p&&size){trace("trace failed malloc",size,(uintptr_t)__builtin_return_address(0));
     trace("trace failed RAM",(uintptr_t)sysconf(_SC_PHYS_PAGES),(uintptr_t)sysconf(_SC_AVPHYS_PAGES));}
 return p;}
void *__wrap_calloc(size_t count,size_t size)
{trace("trace calloc",count,size);
 if(size&&count>=65536/size)trace("trace large caller",count*size,(uintptr_t)__builtin_return_address(0));
 void *p=__real_calloc(count,size);trace("trace calloc done",(uintptr_t)p,0);
 if(!p&&count&&size){trace("trace failed calloc",count,size);
     trace("trace failed RAM",(uintptr_t)sysconf(_SC_PHYS_PAGES),(uintptr_t)sysconf(_SC_AVPHYS_PAGES));}
 return p;}
int __wrap_pthread_mutex_lock(pthread_mutex_t *mutex)
{trace("trace lock",(uintptr_t)mutex,(uintptr_t)pthread_self());
 int rc=__real_pthread_mutex_lock(mutex);trace("trace lock done",(uintptr_t)mutex,(uintptr_t)rc);return rc;}
int __wrap_pthread_once(pthread_once_t *once,void (*routine)(void))
{trace("trace once",(uintptr_t)once,(uintptr_t)routine);
 int rc=__real_pthread_once(once,routine);trace("trace once done",(uintptr_t)once,(uintptr_t)rc);return rc;}
int __wrap___cxa_guard_acquire(uint64_t *guard)
{trace("trace guard",(uintptr_t)guard,__atomic_load_n(guard,__ATOMIC_RELAXED));
 int rc=__real___cxa_guard_acquire(guard);trace("trace guard done",(uintptr_t)guard,(uintptr_t)rc);return rc;}
void __wrap___cxa_guard_release(uint64_t *guard)
{trace("trace guard release",(uintptr_t)guard,0);__real___cxa_guard_release(guard);}
