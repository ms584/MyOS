/* Included by Mesa os_time.c only for MyOS. No kernel/shared-image change. */
#include "os_time.h"
#include <errno.h>
#include <stdint.h>
#define MYOS_NS_PER_SECOND INT64_C(1000000000)
#define MYOS_OBSERVATION_NS INT64_C(1000000)
int64_t os_time_get_nano(void)
{
    struct timespec ts;
    if(clock_gettime(CLOCK_MONOTONIC,&ts)||ts.tv_sec<0||ts.tv_nsec<0||
       ts.tv_nsec>=MYOS_NS_PER_SECOND||ts.tv_sec>INT64_MAX/MYOS_NS_PER_SECOND||
       (ts.tv_sec==INT64_MAX/MYOS_NS_PER_SECOND&&ts.tv_nsec>INT64_MAX%MYOS_NS_PER_SECOND))
        __builtin_trap();
    return ts.tv_sec*MYOS_NS_PER_SECOND+ts.tv_nsec;
}
void os_time_sleep(int64_t usecs)
{
    if(usecs<=0)return;
    struct timespec request={usecs/1000000,(usecs%1000000)*1000};
    int result;
    do{result=clock_nanosleep(CLOCK_MONOTONIC,0,&request,&request);}while(result==EINTR);
    if(result)__builtin_trap();
}
int64_t os_time_get_absolute_timeout(uint64_t timeout)
{
    if(timeout>INT64_MAX)return -1;
    int64_t now=os_time_get_nano();
    if(timeout>(uint64_t)(INT64_MAX-now))return -1;
    return now+(int64_t)timeout;
}
bool os_wait_until_zero_abs_timeout(volatile int *word,int64_t deadline)
{
    /* The API supplies an unnotified 32-bit word. Existing SYS_WAIT requires
     * 8-byte alignment/access and explicit wake, so it cannot safely serve
     * this contract. Observe after blocking timers, never spin or overread.
     * Kernel tick rounding bounds detection latency; no address-wake claim. */
    while(__atomic_load_n(word,__ATOMIC_ACQUIRE)){
        int64_t now=os_time_get_nano();
        if(deadline!=-1&&now>=deadline)return false;
        int64_t next=now>INT64_MAX-MYOS_OBSERVATION_NS?INT64_MAX:now+MYOS_OBSERVATION_NS;
        if(deadline!=-1&&next>deadline)next=deadline;
        struct timespec wake={next/MYOS_NS_PER_SECOND,next%MYOS_NS_PER_SECOND};
        int result=clock_nanosleep(CLOCK_MONOTONIC,TIMER_ABSTIME,&wake,0);
        if(result&&result!=EINTR)__builtin_trap();
    }
    return true;
}
bool os_wait_until_zero(volatile int *word,uint64_t timeout)
{
    if(!__atomic_load_n(word,__ATOMIC_ACQUIRE))return true;
    if(!timeout)return false;
    return os_wait_until_zero_abs_timeout(word,os_time_get_absolute_timeout(timeout));
}
