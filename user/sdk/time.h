#ifndef MYOS_SDK_TIME_H
#define MYOS_SDK_TIME_H
typedef long time_t;
struct timespec { time_t tv_sec;long tv_nsec; };
/* Current guest local timezone is UTC; no TZ database/DST override API. */
struct tm {
    int tm_sec,tm_min,tm_hour,tm_mday,tm_mon,tm_year,tm_wday,tm_yday,tm_isdst;
    long tm_gmtoff;
    const char *tm_zone;
};
#define CLOCK_REALTIME 0
#define CLOCK_MONOTONIC 1
#define TIME_UTC 1
#define TIMER_ABSTIME 1
#ifdef __cplusplus
extern "C" {
#endif
int clock_gettime(int,struct timespec *);
int clock_nanosleep(int,int,const struct timespec *,struct timespec *);
int nanosleep(const struct timespec *,struct timespec *);
int timespec_get(struct timespec *,int);
time_t time(time_t *);
struct tm *gmtime_r(const time_t *,struct tm *);
struct tm *localtime_r(const time_t *,struct tm *);
#ifdef __cplusplus
}
static_assert(sizeof(struct timespec)==16,"target timespec");
static_assert(sizeof(struct tm)==56,"target UTC calendar ABI");
#else
_Static_assert(sizeof(struct timespec)==16,"target timespec");
_Static_assert(sizeof(struct tm)==56,"target UTC calendar ABI");
#endif
#endif
