#include <time.h>
#include <stdint.h>
#include <limits.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#ifdef __cplusplus
#include <ctime>
extern "C" {
#endif
void *mmap(void *,size_t,int,int,int,long);
int munmap(void *,size_t);
#ifdef __cplusplus
}
#endif
#include "calendar-vectors.h"
static void require(int good){if(!good){const char fail[]="native calendar assertion failed\n";write(1,fail,sizeof(fail)-1);abort();}}
static void calendars(void) {
    for(size_t i=0;i<sizeof(vectors)/sizeof(vectors[0]);++i) {
        const struct calendar_vector *v=&vectors[i];time_t t=v->seconds;
        struct tm tm;char text[96];
        errno=741;require(gmtime_r(&t,&tm)==&tm && errno==741);
        require(tm.tm_year==v->year-1900 && tm.tm_mon==v->month-1 && tm.tm_mday==v->day);
        require(tm.tm_hour==13 && tm.tm_min==27 && tm.tm_sec==39 && tm.tm_isdst==0 && tm.tm_gmtoff==0);
        require(localtime_r(&t,&tm)==&tm && mktime(&tm)==t && errno==741);
        require(strftime(text,sizeof(text),"%F %T|%j|%w|%G-%V-%u|%Z|%z",&tm)==strlen(v->formatted));
        require(!strcmp(text,v->formatted) && errno==741);
        for(size_t n=0;n<=strlen(v->formatted)+1;++n) {
            unsigned char buf[128];memset(buf,0x59,sizeof(buf));
            size_t got=strftime((char *)buf+1,n,"%F %T|%j|%w|%G-%V-%u|%Z|%z",&tm);
            require(n>strlen(v->formatted)?got==strlen(v->formatted):got==0);
            require(buf[0]==0x59);
            for(size_t k=n+1;k<sizeof(buf);++k)require(buf[k]==0x59);
        }
        tm.tm_mon+=12;--tm.tm_year;tm.tm_sec-=60;++tm.tm_min;tm.tm_isdst=-1;
        require(mktime(&tm)==t && tm.tm_isdst==0 && tm.tm_year==v->year-1900 && tm.tm_sec==39);
    }
    time_t zero=0,minus=-1;struct tm tm;
    require(gmtime_r(&minus,&tm)==&tm && mktime(&tm)==minus);
    require(!strcmp(ctime(&zero),"Thu Jan  1 00:00:00 1970\n"));
    require(gmtime(&zero)==localtime(&zero));
    require(!strcmp(asctime(gmtime(&zero)),"Thu Jan  1 00:00:00 1970\n"));
    require(difftime(INT64_MAX,INT64_MAX-1)==1.0 && difftime(INT64_MIN+1,INT64_MIN)==1.0);
    require(difftime(INT64_MIN,INT64_MAX)==-18446744073709551616.0);
    require(difftime(INT64_MAX,INT64_MIN)==18446744073709551616.0);
#ifdef __cplusplus
    require(std::difftime(7,3)==4 && std::mktime(&tm)==minus);
#endif
    errno=0;require(clock()==(clock_t)-1 && errno==ENOSYS); /* No fake CPU time. */
    memset(&tm,0,sizeof(tm));tm.tm_year=INT_MAX;tm.tm_mon=INT_MAX;
    struct tm before;memcpy(&before,&tm,sizeof(tm));errno=0;
    require(mktime(&tm)==(time_t)-1 && errno==EOVERFLOW && !memcmp(&tm,&before,sizeof(tm)));
    memset(&tm,0,sizeof(tm));tm.tm_year=70;tm.tm_mday=INT_MIN;
    time_t day=mktime(&tm);require(day!=(time_t)-1 && tm.tm_year < 70);
    char text[26];memset(text,0x55,sizeof(text));tm.tm_year=INT_MAX;
    errno=0;require(!asctime_r(&tm,text) && errno==EOVERFLOW && text[0]==0x55);
}
static void reentrant(void) {
    time_t t=0;struct tm tm;char b[96];
    require(gmtime_r(&t,&tm)==&tm && mktime(&tm)==0);
    require(ctime_r(&t,b)==b && !strcmp(b,"Thu Jan  1 00:00:00 1970\n"));
    require(strftime(b,sizeof(b),"%a|%A|%b|%B|%c|%x|%X|%r|%h",&tm)>0);
    require(!strcmp(b,"Thu|Thursday|Jan|January|Thu Jan  1 00:00:00 1970|01/01/70|00:00:00|12:00:00 AM|Jan"));
    require(strftime(b,sizeof(b),"%C %d %e %H %I %j %m %M %p %R %s %S %u %U %W %V %w %y %Y %% %n%t",&tm)>0);
    require(!strcmp(b,"19 01  1 00 12 001 01 00 AM 00:00 0 00 4 00 00 01 4 70 1970 % \n\t"));
    require(strftime(b,sizeof(b),"%EY-%Om-%Od|%6Y|%_d|%-d|%04Y",&tm)>0);
    require(!strcmp(b,"1970-01-01|001970| 1|1|1970"));
    require(strftime(b,sizeof(b),"%Q",&tm)==0 && b[0]==0);
}
static void guards(void) {
    char *p=(char *)mmap(0,8192,3,0x22,-1,0);require(p!=(char *)-1);
    require(munmap(p+4096,4096)==0);time_t t=0;struct tm tm;require(gmtime_r(&t,&tm)==&tm);
    require(ctime_r(&t,p+4096-26)==p+4096-26 && p[4095]==0);
    require(strftime(p+4096-11,11,"%F",&tm)==10 && p[4095]==0);
    require(strftime(p+4096,0,"%F",&tm)==0);
    memcpy(p+4093,"%F",3);char b[16];require(strftime(b,sizeof(b),p+4093,&tm)==10);
    require(munmap(p,4096)==0);
}
static void *worker(void *v){for(int i=0;i<40;++i)reentrant();return v;}
#ifdef __cplusplus
extern "C"
#endif
int _start(void) {
    calendars();reentrant();guards();pthread_t threads[8];
    for(unsigned long i=0;i<8;++i)require(pthread_create(&threads[i],0,worker,(void *)(i+1))==0);
    for(unsigned long i=0;i<8;++i){void *v=0;require(pthread_join(threads[i],&v)==0 && v==(void *)(i+1));}
    const char done[]="native UTC calendar/strftime/bounds/8workers PASS\n";
    require(write(1,done,sizeof(done)-1)==(ssize_t)(sizeof(done)-1));return 101;
}
