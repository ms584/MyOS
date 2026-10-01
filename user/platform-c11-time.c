/* Standalone target SDK clock wrappers; no host libc or kernel image changes. */
#include <time.h>
#include <errno.h>
int nanosleep(const struct timespec *request,struct timespec *remaining)
{
    int rc=clock_nanosleep(CLOCK_MONOTONIC,0,request,remaining);
    if(rc){errno=rc;return -1;}return 0;
}
int timespec_get(struct timespec *value,int base)
{return base==TIME_UTC&&clock_gettime(CLOCK_REALTIME,value)==0?base:0;}
/* Mesa's c11/time.h maps its extended bases to this distinct symbol.
 * Unsupported CPU-time/raw clocks are reported unavailable, not fabricated. */
int c23_timespec_get(struct timespec *value,int base)
{
    if(base!=1&&base!=2)return 0;
    return clock_gettime(base==1?CLOCK_REALTIME:CLOCK_MONOTONIC,value)==0?base:0;
}
