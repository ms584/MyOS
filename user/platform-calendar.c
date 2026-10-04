/* UTC guest calendar profile; no host timezone/locale or Linux syscall code. */
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <limits.h>
int __myos_secs_to_tm(long long,struct tm *);
long long __myos_tm_to_secs(const struct tm *);
struct tm *gmtime_r(const time_t *timer,struct tm *output)
{
    if(!timer||!output){errno=EINVAL;return 0;}
    struct tm converted={0};
    if(__myos_secs_to_tm(*timer,&converted)<0){errno=EOVERFLOW;return 0;}
    converted.tm_isdst=0;converted.tm_gmtoff=0;converted.tm_zone="UTC";
    *output=converted;return output;
}
struct tm *localtime_r(const time_t *timer,struct tm *output)
{return gmtime_r(timer,output);}
time_t time(time_t *output)
{
    struct timespec now;
    if(clock_gettime(CLOCK_REALTIME,&now))return (time_t)-1;
    if(output)*output=now.tv_sec;
    return now.tv_sec;
}
clock_t clock(void) { errno=ENOSYS;return (clock_t)-1; }
double difftime(time_t end,time_t begin)
{
    /* Preserve small differences near INT64 limits without signed overflow. */
    if(end>=begin)return (double)((uint64_t)end-(uint64_t)begin);
    return -(double)((uint64_t)begin-(uint64_t)end);
}
time_t mktime(struct tm *input)
{
    if(!input){errno=EINVAL;return (time_t)-1;}
    time_t seconds=(time_t)__myos_tm_to_secs(input);
    struct tm normalized;
    if(!gmtime_r(&seconds,&normalized))return (time_t)-1;
    *input=normalized;return seconds;
}
/* ISO C permits these traditional interfaces to share static storage. Use
 * the _r interfaces for independent worker buffers; no TLS is fabricated. */
static struct tm shared_tm;
static char shared_text[26];
struct tm *gmtime(const time_t *input){return gmtime_r(input,&shared_tm);}
struct tm *localtime(const time_t *input){return localtime_r(input,&shared_tm);}
char *asctime_r(const struct tm *input,char *out)
{
    static const char days[][4]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
    static const char months[][4]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
    if(!input||!out){errno=EINVAL;return 0;}
    if((unsigned)input->tm_wday>6||(unsigned)input->tm_mon>11||
       input->tm_year < -1900 || input->tm_year > 8099 ||
       input->tm_mday<1||input->tm_mday>31||input->tm_hour<0||input->tm_hour>23||
       input->tm_min<0||input->tm_min>59||input->tm_sec<0||input->tm_sec>60){errno=EOVERFLOW;return 0;}
    char text[26];
    int n=snprintf(text,sizeof(text),"%.3s %.3s%3d %.2d:%.2d:%.2d %d\n",
                   days[input->tm_wday],months[input->tm_mon],input->tm_mday,
                   input->tm_hour,input->tm_min,input->tm_sec,1900+input->tm_year);
    if(n<0||n>=26){errno=EOVERFLOW;return 0;}
    for(int i=0;i<=n;++i)out[i]=text[i];
    return out;
}
char *asctime(const struct tm *input){return asctime_r(input,shared_text);}
char *ctime_r(const time_t *input,char *out)
{
    struct tm converted;if(!localtime_r(input,&converted))return 0;
    return asctime_r(&converted,out);
}
char *ctime(const time_t *input){return ctime_r(input,shared_text);}
