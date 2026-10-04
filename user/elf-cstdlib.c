#include <stdlib.h>
#include <wchar.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#ifdef __cplusplus
extern "C" {
#endif
void *mmap(void *,size_t,int,int,int,long);
int munmap(void *,size_t);
#ifdef __cplusplus
}
#endif
#ifdef __cplusplus
#include <cstdlib>
static_assert(sizeof(div_t)==8 && sizeof(ldiv_t)==16 && sizeof(lldiv_t)==16);
#else
_Static_assert(sizeof(div_t)==8 && sizeof(ldiv_t)==16 && sizeof(lldiv_t)==16,"LP64 div ABI");
#endif
static void require(int value) { if(!value) { write(1,"native Cstdlib assertion failed\n",31);abort(); } }
static const char text[]="A\xe0\xb8\x81\xe2\x82\xac\xf0\x9f\x98\x80";
static const wchar_t points[]={65,0xe01,0x20ac,0x1f600,0};
static void conversions(void) {
    wchar_t w[8],wc;
    char b[32];
    errno=713;require(mblen(0,0)==0 && mbtowc(0,0,0)==0 && errno==713);
    require(mblen("",1)==0 && mbtowc(&wc,"",1)==0 && wc==0);
    size_t off=0;
    for(int i=0;i<4;++i) {
        int n=i==0?1:i==3?4:3;
        errno=714;
        require(mblen(text+off,n)==n && mbtowc(&wc,text+off,n)==n && wc==points[i] && errno==714);
        mbstate_t state={0,0};
        require(wcrtomb(b,points[i],&state)==(size_t)n && !memcmp(b,text+off,n) && mbsinit(&state));
        if(n>1){errno=0;require(mbtowc(&wc,text+off,n-1)==-1 && errno==EILSEQ);}
        state=(mbstate_t){0,0};
        for(int j=0;j<n;++j) {
            size_t got=mbrtowc(&wc,text+off+j,1,&state);
            require(got==(j==n-1?(size_t)1:(size_t)-2));
        }
        require(wc==points[i] && mbsinit(&state));
        off+=(size_t)n;
    }
    require(off==11 && mbstowcs(0,text,0)==4 && wcstombs(0,points,0)==11);
    for(int n=0;n<=5;++n) {
        for(int i=0;i<8;++i)w[i]=0x556677;
        size_t got=mbstowcs(w,text,(size_t)n);
        require(got==(size_t)(n<4?n:4));
        for(size_t i=0;i<got;++i)require(w[i]==points[i]);
        require(n>4 ? w[4]==0 && w[5]==0x556677 : w[n]==0x556677);
    }
    memset(b,0x55,sizeof(b));require(wcstombs(b,points,11)==11 && b[11]==0x55 && !memcmp(b,text,11));
    require(wcstombs(b,points,12)==11 && b[11]==0);
    const wchar_t *wp=points;
    memset(b,0x55,sizeof(b));require(wcsrtombs(b,&wp,8,0)==7 && wp==points+3 && b[7]==0x55);
    require(wcsrtombs(b+7,&wp,4,0)==4 && wp==points+4);
    require(wcsrtombs(b+11,&wp,1,0)==0 && wp==0 && !memcmp(b,text,12));
    const char *sp=text;
    require(mbsrtowcs(w,&sp,2,0)==2 && sp==text+4);
    require(mbsrtowcs(w+2,&sp,3,0)==2 && sp==0 && w[4]==0);
    sp=text;require(mbsrtowcs(0,&sp,0,0)==4 && sp==text);
    wp=points;require(wcsrtombs(0,&wp,0,0)==11 && wp==points);
    const char *bad[]={"\xc0\x80","\xed\xa0\x80","\xf4\x90\x80\x80","\x80","\xe2\x82"};
    for(int i=0;i<5;++i){errno=0;require(mbstowcs(w,bad[i],8)==(size_t)-1 && errno==EILSEQ);}
    wchar_t badw[]={0xd800,0};errno=0;
    require(wcstombs(b,badw,sizeof(b))==(size_t)-1 && errno==EILSEQ);
    errno=0;require(wcrtomb(b,(wchar_t)0x110000,0)==(size_t)-1 && errno==EILSEQ);
    require(wcrtomb(0,0,0)==1);
    errno=715;require(system(0)==0 && errno==715);
    require(system("not-executed")==-1 && errno==ENOSYS);
}
static void divisions(void) {
    for(int a=-31;a<=31;++a)for(int b=-7;b<=7;++b)if(b) {
        div_t d=div(a,b);ldiv_t l=ldiv(a,b);lldiv_t ll=lldiv(a,b);
        require(d.quot==a/b && d.rem==a%b);
        require(l.quot==d.quot && l.rem==d.rem && ll.quot==d.quot && ll.rem==d.rem);
        require(d.quot*b+d.rem==a);
    }
    ldiv_t l=ldiv(INT64_MIN,2);require(l.quot==INT64_MIN/2 && !l.rem);
    lldiv_t ll=lldiv(INT64_MAX,-2);require(ll.quot==INT64_MAX/-2 && ll.rem==1);
#ifdef __cplusplus
    require(std::div(-17L,5L).quot==-3 && std::div(-17,5).rem==-2);
#endif
}
static void guards(void) {
    char *p=(char *)mmap(0,8192,3,0x22,-1,0);
    require(p!=(char *)-1 && munmap(p+4096,4096)==0);
    wchar_t wc,out[4];
    memcpy(p+4092,"\xe2\x82\xac",4);
    require(mbtowc(&wc,p+4092,3)==3 && wc==0x20ac);
    require(mbstowcs(out,p+4092,2)==1 && out[0]==0x20ac && out[1]==0);
    require(mbstowcs(0,p+4092,0)==1);
    memcpy(p+4092,"abc",4);require(mbstowcs(0,p+4092,0)==3);
    require(mbtowc(&wc,p+4095,1)==0 && wc==0);
    p[4094]=(char)0xe2;p[4095]=(char)0x82;
    errno=0;require(mbtowc(&wc,p+4094,2)==-1 && errno==EILSEQ);
    mbstate_t st={0,0};require(mbrtowc(&wc,p+4094,2,&st)==(size_t)-2);
    require(mbstowcs((wchar_t *)(p+4092),"A",1)==1 && *(wchar_t *)(p+4092)==65);
    const wchar_t euro[]={0x20ac,0};
    require(wcstombs(p+4093,euro,3)==3 && !memcmp(p+4093,"\xe2\x82\xac",3));
    require(wcstombs(p+4092,euro,4)==3 && p[4095]==0);
    require(munmap(p,4096)==0);
}
static void *worker(void *value) { for(int i=0;i<20;++i){divisions();conversions();}return value; }
#ifdef __cplusplus
extern "C"
#endif
int _start(void) {
    divisions();conversions();guards();
    pthread_t threads[8];
    for(unsigned long i=0;i<8;++i)require(pthread_create(&threads[i],0,worker,(void *)(i+1))==0);
    for(unsigned long i=0;i<8;++i){void *v=0;require(pthread_join(threads[i],&v)==0 && v==(void *)(i+1));}
    const char done[]="native sealed Cstdlib div/UTF8/bounds/8workers PASS\n";
    require(write(1,done,sizeof(done)-1)==(ssize_t)(sizeof(done)-1));return 98;
}
