#include "util/os_time.h"
#include "util/timespec.h"
#include "util/cnd_monotonic.h"
#include <errno.h>
#include <limits.h>
#include <string.h>
static void message(const char *text)
{
    size_t length=strlen(text);
    register uint64_t a __asm__("rax")=1,d __asm__("rdi")=(uintptr_t)text;
    register uint64_t s __asm__("rsi")=length;
    __asm__ volatile("int $0x80":"+a"(a):"D"(d),"S"(s):"rdx","memory","cc");
}
#define CHECK(x) do{if(!(x)){message("Mesa time failed: " #x "\n");__builtin_trap();}}while(0)
/* A valid int deliberately NOT eight-byte aligned, with adjacent guards. */
static struct {int before;volatile int word;int after;} guarded __attribute__((aligned(8)))={17,1,29};
static int clear_word(void *unused)
{
    (void)unused;os_time_sleep(30000);__atomic_store_n(&guarded.word,0,__ATOMIC_RELEASE);return 31;
}
static void calendar(void)
{
    const struct {time_t seconds;int year,month,day,hour,min,sec,wday,yday;} cases[]={
        {0,70,0,1,0,0,0,4,0},{-1,69,11,31,23,59,59,3,364},
        {951782400,100,1,29,0,0,0,2,59},
        {-2203891200L,0,2,1,0,0,0,4,59},
        {4107542400L,200,2,1,0,0,0,1,59},
        {2147483647,138,0,19,3,14,7,2,18}
    };
    for(unsigned i=0;i<sizeof(cases)/sizeof(cases[0]);++i){
        struct tm t;errno=57;
        CHECK(gmtime_r(&cases[i].seconds,&t)==&t&&errno==57);
        CHECK(t.tm_year==cases[i].year&&t.tm_mon==cases[i].month&&t.tm_mday==cases[i].day);
        CHECK(t.tm_hour==cases[i].hour&&t.tm_min==cases[i].min&&t.tm_sec==cases[i].sec);
        CHECK(t.tm_wday==cases[i].wday&&t.tm_yday==cases[i].yday);
        CHECK(!t.tm_isdst&&!t.tm_gmtoff&&t.tm_zone[0]=='U');
        CHECK(os_localtime(&cases[i].seconds,&t)==&t&&t.tm_year==cases[i].year);
    }
    struct tm t;memset(&t,0x55,sizeof(t));time_t huge=INT64_MAX;
    CHECK(!gmtime_r(&huge,&t)&&errno==EOVERFLOW&&t.tm_sec==0x55555555);
    huge=INT64_MIN;CHECK(!gmtime_r(&huge,&t)&&errno==EOVERFLOW&&t.tm_sec==0x55555555);
    CHECK(!gmtime_r(0,&t)&&errno==EINVAL);
    CHECK(!localtime_r(&huge,0)&&errno==EINVAL);
}
#ifdef TEST_TIME_CANCEL
static unsigned ready;
static int blocked(void *unused)
{
    (void)unused;__atomic_add_fetch(&ready,1,__ATOMIC_RELEASE);
    os_wait_until_zero(&guarded.word,OS_TIMEOUT_INFINITE);return 0;
}
#endif
int _start(void)
{
#ifdef TEST_TIME_CANCEL
    thrd_t workers[8];
    for(unsigned i=0;i<8;++i)CHECK(thrd_create(&workers[i],blocked,0)==thrd_success);
    while(__atomic_load_n(&ready,__ATOMIC_ACQUIRE)!=8)os_time_sleep(1000);
    message("native Mesa time cancel ready\n");
    int result;CHECK(thrd_join(workers[0],&result)==thrd_success);return 76;
#else
    calendar();
    struct timespec a={7,500000000},b={0,500000000},sum;
    timespec_add(&sum,&a,&b);CHECK(sum.tv_sec==8&&sum.tv_nsec==0);
    struct timespec realtime,monotonic;
    CHECK(timespec_get(&realtime,TIME_UTC)==TIME_UTC);
    CHECK(timespec_get(&monotonic,TIME_MONOTONIC)==TIME_MONOTONIC);
    CHECK(realtime.tv_sec>1000000000&&monotonic.tv_sec<1000000000);
    int64_t start=os_time_get_nano();os_time_sleep(30000);
    CHECK(os_time_get_nano()-start>=30000000);
    os_time_sleep(0);os_time_sleep(-1);
    CHECK(os_time_get_absolute_timeout(OS_TIMEOUT_INFINITE)==-1);
    CHECK(os_time_get_absolute_timeout(INT64_MAX)==-1);
    CHECK(os_time_get_absolute_timeout(1000000000)>os_time_get_nano());
    CHECK(!os_wait_until_zero(&guarded.word,0));
    CHECK(!os_wait_until_zero_abs_timeout(&guarded.word,0));
    start=os_time_get_nano();CHECK(!os_wait_until_zero(&guarded.word,1000000));
    CHECK(os_time_get_nano()-start>=1000000);
    for(unsigned i=0;i<3;++i){
        __atomic_store_n(&guarded.word,1,__ATOMIC_RELEASE);thrd_t worker;
        CHECK(thrd_create(&worker,clear_word,0)==thrd_success);
        CHECK(os_wait_until_zero(&guarded.word,i==1?OS_TIMEOUT_INFINITE:1000000000));
        int result;CHECK(thrd_join(worker,&result)==thrd_success&&result==31);
        CHECK(guarded.before==17&&guarded.after==29);
    }
    CHECK(os_wait_until_zero_abs_timeout(&guarded.word,0));
    struct u_cnd_monotonic cond;mtx_t mutex;
    CHECK(mtx_init(&mutex,mtx_plain)==thrd_success);
    CHECK(u_cnd_monotonic_init(&cond)==thrd_success);
    CHECK(mtx_lock(&mutex)==thrd_success);
    CHECK(timespec_get(&a,TIME_MONOTONIC)==TIME_MONOTONIC);
    b=(struct timespec){0,50000000};timespec_add(&sum,&a,&b);
    start=os_time_get_nano();
    CHECK(u_cnd_monotonic_timedwait(&cond,&mutex,&sum)==thrd_timedout);
    CHECK(os_time_get_nano()-start>=40000000);
    CHECK(mtx_trylock(&mutex)==thrd_busy);
    sum=(struct timespec){0,0};
    CHECK(u_cnd_monotonic_timedwait(&cond,&mutex,&sum)==thrd_timedout);
    sum.tv_nsec=1000000000;
    CHECK(u_cnd_monotonic_timedwait(&cond,&mutex,&sum)==thrd_error);
    CHECK(mtx_unlock(&mutex)==thrd_success);
    u_cnd_monotonic_destroy(&cond);mtx_destroy(&mutex);
    message("native Mesa clocks/calendar/32-bit waits PASS\n");return 75;
#endif
}
