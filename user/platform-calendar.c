/* UTC guest calendar profile; no host timezone/locale or Linux syscall code. */
#include <time.h>
#include <errno.h>
int __myos_secs_to_tm(long long,struct tm *);
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
