/* Native Ring 3 fixture; Starboard declarations come from the pinned header. */
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "window.h"
#include "native-ui.h"
#ifndef MODE
#define MODE 0
#endif
_Static_assert(sizeof(SbWindowOptions)==24&&offsetof(SbWindowOptions,name)==16,"pinned options");
_Static_assert(sizeof(SbWindowSize)==12,"pinned size");
extern int MyOSUiQuery(void *,struct native_ui_info *);
extern int MyOSUiPresent(void *,const struct native_ui_present *);
extern int MyOSUiPoll(void *,struct native_ui_event *,int);
extern int *__errno_location(void);
extern int64_t write(int,const void *,size_t);
extern int pthread_create(uint64_t *,const void *,void *(*)(void *),void *);
extern int pthread_join(uint64_t,void **);
extern void *mmap(void *,size_t,int,int,int,int64_t);
extern int mprotect(void *,size_t,int),munmap(void *,size_t);
static int64_t call(uint64_t number,uint64_t a,uint64_t b,uint64_t c)
{
    register uint64_t result __asm__("rax")=number;
    __asm__ volatile("int $0x80":"+a"(result):"D"(a),"S"(b),"d"(c):"memory","cc");
    return (int64_t)result;
}
static void say(const char *s){size_t n=0;while(s[n])++n;write(1,s,n);}
static void pause_ticks(uint64_t n)
{uint64_t end=(uint64_t)call(13,0,0,0)+n;while((uint64_t)call(13,0,0,0)<end)call(4,0,0,0);}
static uint32_t pixels[96*64];
static SbWindow window;
static volatile unsigned waiting;
static int waited_result,waited_error;
static void *waiter(void *unused)
{
    (void)unused;struct native_ui_event event;
    __atomic_store_n(&waiting,1,__ATOMIC_RELEASE);
    waited_result=MyOSUiPoll(window,&event,-1);waited_error=*__errno_location();return 0;
}
#define REQUIRE(condition,code) do{if(!(condition)){say("Native UI FAIL: " #condition "\n");return code;}}while(0)
uint64_t _start(void)
{
    struct {uint64_t before;SbWindowOptions options;uint64_t after;} guarded={0};
    guarded.before=guarded.after=0xabcdef0123456789ull;
    SbWindowSetDefaultOptions(&guarded.options);
    REQUIRE(guarded.before==guarded.after&&guarded.before==0xabcdef0123456789ull,1);
    REQUIRE(!guarded.options.windowed&&guarded.options.size.video_pixel_ratio==1.0f&&guarded.options.name,2);
    struct native_ui_info info;
    if(MODE==3){
        REQUIRE(MyOSUiQuery(0,&info)==-1&&*__errno_location()==95,3);
        REQUIRE(!SbWindowCreate(0)&&!SbWindowCreate(&guarded.options),4);
        say("Native window text-mode unavailable PASS\n");return 55;
    }
    REQUIRE(!MyOSUiQuery(0,&info)&&info.version==1&&info.width==800&&info.height==600&&!info.handle,5);
    REQUIRE(guarded.options.size.width==(int)info.width&&guarded.options.size.height==(int)info.height,6);
    guarded.options.windowed=true;REQUIRE(!SbWindowCreate(&guarded.options),7);
    guarded.options.windowed=false;guarded.options.size.width++;
    REQUIRE(!SbWindowCreate(&guarded.options),8);guarded.options.size.width--;
    guarded.options.size.video_pixel_ratio=2.0f; /* Header says request ratio is ignored. */
    window=SbWindowCreate(&guarded.options);REQUIRE(window&&!SbWindowCreate(0),9);
    SbWindowSize size={-1,-1,-1.0f};
    REQUIRE(SbWindowGetSize(window,&size)&&size.width==800&&size.height==600&&size.video_pixel_ratio==1.0f,10);
    REQUIRE(SbWindowGetPlatformHandle(window)==window&&SbWindowGetDiagonalSizeInInches(window)==0.0f,11);
    REQUIRE(!SbWindowDestroy(0)&&!SbWindowGetSize(0,&size)&&size.width==800,12);
    REQUIRE(!SbWindowGetSize((SbWindow)((uintptr_t)window+(1ull<<32)),&size)&&size.width==800,13);
    REQUIRE(!MyOSUiQuery(window,&info)&&info.handle==(uintptr_t)window&&!info.dropped,14);
    for(unsigned y=0;y<64;++y)for(unsigned x=0;x<96;++x)
        pixels[y*96+x]=(y<32?(x<48?0xff0000ffu:0xff00ff00u):(x<48?0xffff0000u:0xff00ffffu));
    struct native_ui_present request={(uintptr_t)pixels,96,64,96*4,NATIVE_UI_RGBA8,688,520};
    REQUIRE(!MyOSUiPresent(window,&request),15);
    /* A valid first row followed by an unreadable second row must change NO pixel. */
    unsigned char *mapping=mmap(0,8192,3,34,-1,0);
    REQUIRE(mapping!=(void *)(uintptr_t)-1,16);
    for(unsigned i=0;i<64;++i)mapping[4096-64+i]=255;
    REQUIRE(!mprotect(mapping+4096,4096,0),17);
    struct native_ui_present bad={(uintptr_t)(mapping+4096-64),16,2,64,NATIVE_UI_RGBA8,688,520};
    REQUIRE(MyOSUiPresent(window,&bad)==-1&&*__errno_location()==14,18);
    REQUIRE(!munmap(mapping,8192),19);
    uint32_t xrgb=0xab123456;
    struct native_ui_present rgb={(uintptr_t)&xrgb,1,1,8,NATIVE_UI_XRGB8888,783,583};
    REQUIRE(!MyOSUiPresent(window,&rgb),43);
    bad=request;bad.pixels=UINT64_MAX-20;
    REQUIRE(MyOSUiPresent(window,&bad)==-1&&*__errno_location()==14,20);
    bad=request;bad.stride=1;
    REQUIRE(MyOSUiPresent(window,&bad)==-1&&*__errno_location()==22,21);
    bad=request;bad.x=-1;
    REQUIRE(MyOSUiPresent(window,&bad)==-1&&*__errno_location()==22,22);
    bad=request;bad.format=99;
    REQUIRE(MyOSUiPresent(window,&bad)==-1&&*__errno_location()==22,23);
    bad=request;bad.x=750;
    REQUIRE(MyOSUiPresent(window,&bad)==-1&&*__errno_location()==22,44);
    REQUIRE(MyOSUiPresent((void *)((uintptr_t)window+1),&request)==-1&&*__errno_location()==9,24);
    REQUIRE(MyOSUiQuery(window,(void *)0x10000)==-1&&*__errno_location()==14,25);
    pause_ticks(20);struct native_ui_event event;
    while(MyOSUiPoll(window,&event,0)==1){}
    for(unsigned i=0;i<sizeof(event);++i)((unsigned char *)&event)[i]=0xa5;
    uint64_t start=(uint64_t)call(13,0,0,0);
    REQUIRE(MyOSUiPoll(window,&event,40)==0&&(uint64_t)call(13,0,0,0)-start>=4,26);
    for(unsigned i=0;i<sizeof(event);++i)REQUIRE(((unsigned char *)&event)[i]==0xa5,45);
    REQUIRE(MyOSUiPoll(window,0,0)==-1&&*__errno_location()==14,46);
    REQUIRE(MyOSUiPoll(window,&event,-2)==-1&&*__errno_location()==22,27);
    if(MODE==1){
        uint64_t thread;waiting=0;
        REQUIRE(!pthread_create(&thread,0,waiter,0),28);
        while(!__atomic_load_n(&waiting,__ATOMIC_ACQUIRE))call(4,0,0,0);
        pause_ticks(5);say("Native window cancel ready\n");
        for(;;)call(4,0,0,0);
    }
    if(MODE==2){
        say("Native window fault ready\n");
        do{REQUIRE(MyOSUiPoll(window,&event,-1)==1,29);}while(event.character!='b');
        __asm__ volatile("ud2");return 30;
    }
    say("Native window frame ready\n");
    unsigned seen=0;
    while(seen!=63){
        REQUIRE(MyOSUiPoll(window,&event,-1)==1,31);
        REQUIRE(event.tick&&event.type>=1&&event.type<=3,32);
        if(event.type==NATIVE_UI_KEY_DOWN&&event.scan==30){
            REQUIRE(event.character=='A'&&(event.modifiers&NATIVE_UI_SHIFT),33);seen|=1;
        }
        if(event.type==NATIVE_UI_KEY_UP&&event.scan==30){REQUIRE(!event.character,34);seen|=2;}
        if(event.scan==0x150&&event.type==NATIVE_UI_KEY_DOWN){REQUIRE(!event.character,35);seen|=4;}
        if(event.scan==0x150&&event.type==NATIVE_UI_KEY_UP)seen|=8;
        if(event.type==NATIVE_UI_POINTER){
            REQUIRE(event.x>=0&&event.x<800&&event.y>=0&&event.y<600,36);
            if((event.changed&1)&&(event.buttons&1))seen|=16;
            if((event.changed&1)&&!(event.buttons&1))seen|=32;
        }
    }
    say("Native window real input checked\n");
    while(MyOSUiPoll(window,&event,0)==1){}
    uint64_t thread;waiting=0;
    REQUIRE(!pthread_create(&thread,0,waiter,0),37);
    while(!__atomic_load_n(&waiting,__ATOMIC_ACQUIRE))call(4,0,0,0);
    pause_ticks(5);SbWindow stale=window;
    REQUIRE(SbWindowDestroy(window)&&!pthread_join(thread,0)&&waited_result==-1&&waited_error==9,38);
    REQUIRE(!SbWindowDestroy(stale)&&!SbWindowGetSize(stale,&size)&&size.width==800,39);
    REQUIRE(!SbWindowGetPlatformHandle(stale),40);
    window=SbWindowCreate(0);REQUIRE(window&&window!=stale,41);
    REQUIRE(MyOSUiPoll(stale,&event,0)==-1&&*__errno_location()==9&&SbWindowDestroy(window),42);
    say("Native window ABI/present/input/wait/close/reuse PASS\n");return 55;
}
