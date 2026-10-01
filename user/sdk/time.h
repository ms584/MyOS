#ifndef MYOS_SDK_TIME_H
#define MYOS_SDK_TIME_H
typedef long time_t;
struct timespec { time_t tv_sec;long tv_nsec; };
#define CLOCK_REALTIME 0
#define CLOCK_MONOTONIC 1
#define TIME_UTC 1
#ifdef __cplusplus
extern "C" {
#endif
int clock_gettime(int,struct timespec *);
int clock_nanosleep(int,int,const struct timespec *,struct timespec *);
int nanosleep(const struct timespec *,struct timespec *);
int timespec_get(struct timespec *,int);
#ifdef __cplusplus
}
static_assert(sizeof(struct timespec)==16,"target timespec");
#else
_Static_assert(sizeof(struct timespec)==16,"target timespec");
#endif
#endif
