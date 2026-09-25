#include "security.h"

/* CMOS RTC is used as the wall-clock anchor. Refuse implausible dates rather
 * than validating certificates at an invented build time. */
struct date {unsigned year,month,day,hour,minute,second;};
static uint64_t epoch_at_boot,tick_at_boot;
static bool clock_ok,rng_ok;
static uint8_t rtc(unsigned reg){outb(0x70,(uint8_t)(0x80|reg));return inb(0x71);}
static unsigned bcd(uint8_t n){return (unsigned)(n&15)+10u*(n>>4);}
static bool read_date(struct date *d){
    uint64_t flags=irq_save();
    for(unsigned retry=0;retry<100000;++retry)if(!(rtc(0x0a)&0x80))break;
    if(rtc(0x0a)&0x80){outb(0x70,0);irq_restore(flags);return false;}
    uint8_t status=rtc(0x0b),second=rtc(0),minute=rtc(2),hour=rtc(4);
    uint8_t day=rtc(7),month=rtc(8),year=rtc(9),century=rtc(0x32);
    outb(0x70,0);irq_restore(flags);
    if(!(status&4)){
        second=(uint8_t)bcd(second);minute=(uint8_t)bcd(minute);
        hour=(uint8_t)((hour&0x80)|bcd(hour&0x7f));day=(uint8_t)bcd(day);
        month=(uint8_t)bcd(month);year=(uint8_t)bcd(year);
        century=(uint8_t)bcd(century);
    }
    unsigned h=hour&0x7f;
    if(!(status&2)){if(h<1||h>12)return false;h=(h%12)+((hour&0x80)?12:0);}
    unsigned full_year=century==20?(unsigned)century*100+year:2000u+year;
    *d=(struct date){full_year,month,day,h,minute,second};
    return true;
}
static bool leap(unsigned year){return !(year%4) && ((year%100)||!(year%400));}
static unsigned month_days(unsigned year,unsigned month){
    static const uint8_t days[12]={31,28,31,30,31,30,31,31,30,31,30,31};
    return days[month-1]+(month==2&&leap(year));
}
static bool to_epoch(struct date d,uint64_t *out){
    if(d.year<2024||d.year>2099||d.month<1||d.month>12||d.day<1||
       d.day>month_days(d.year,d.month)||d.hour>23||d.minute>59||d.second>59)return false;
    uint64_t days=0;
    for(unsigned y=1970;y<d.year;++y)days+=leap(y)?366u:365u;
    for(unsigned m=1;m<d.month;++m)days+=month_days(d.year,m);
    days+=d.day-1;
    *out=days*86400u+d.hour*3600u+d.minute*60u+d.second;
    return true;
}
static bool random64(uint64_t *value){
    unsigned char success=0;
    for(unsigned i=0;i<16&&!success;++i)
        __asm__ volatile("rdrand %0; setc %1":"=r"(*value),"=qm"(success)::"cc");
    return success!=0;
}
bool security_random(void *out,size_t size){
    if(!rng_ok||!out)return false;
    uint8_t *bytes=out;uint64_t previous=0;bool have_previous=false;
    while(size){
        uint64_t sample;
        if(!random64(&sample)||(have_previous&&sample==previous))return false;
        previous=sample;have_previous=true;
        size_t count=size<8?size:8;
        memcpy(bytes,&sample,count);bytes+=count;size-=count;
    }
    return true;
}
void security_init(void){
    struct date first,second;uint64_t when;
    clock_ok=false;
    for(unsigned attempt=0;attempt<5;++attempt)
        if(read_date(&first)&&read_date(&second)&&
           first.year==second.year&&first.month==second.month&&first.day==second.day&&
           first.hour==second.hour&&first.minute==second.minute&&
           first.second==second.second&&to_epoch(first,&when)){
            epoch_at_boot=when;tick_at_boot=ticks;clock_ok=true;break;
        }
    uint32_t a,b,c,d;
    __asm__ volatile("cpuid":"=a"(a),"=b"(b),"=c"(c),"=d"(d):"a"(1),"c"(0));
    rng_ok=(c&(1u<<30))!=0;
    if(rng_ok){uint64_t probe[2];rng_ok=security_random(probe,sizeof(probe));}
}
bool security_time_ready(void){return clock_ok;}
bool security_entropy_ready(void){return rng_ok;}
uint64_t security_unix_time(void){return clock_ok?epoch_at_boot+(ticks-tick_at_boot)/100u:0;}
void security_status(void){
    print("Security: UTC ");
    if(clock_ok)decimal(security_unix_time());else print("unavailable");
    print(", RDRAND ");print(rng_ok?"ready":"unavailable");
    print(", HTTPS ");print(clock_ok&&rng_ok?"prerequisites ready":"blocked");
    print("\n");
}
