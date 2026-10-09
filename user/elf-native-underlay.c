/* Ring3 native underlay contract: real framebuffer evidence is checked by QEMU. */
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "native-ui.h"
#include "native-video-abi.h"
extern void *SbWindowCreate(const void *);
extern bool SbWindowDestroy(void *);
extern int MyOSUiPresent(void *,const struct native_ui_present *);
extern int MyOSUiPoll(void *,struct native_ui_event *,int);
extern int64_t write(int,const void *,size_t);
extern void *mmap(void *,size_t,int,int,int,int64_t);
extern int mprotect(void *,size_t,int),munmap(void *,size_t);
#ifndef MODE
#define MODE 0
#endif
static int64_t call(uint64_t n,uint64_t a,uint64_t b,uint64_t c){
 register uint64_t r __asm__("rax")=n;
 __asm__ volatile("int $0x80":"+a"(r):"D"(a),"S"(b),"d"(c):"memory","cc");return (int64_t)r;
}
static void say(const char *s){size_t n=0;while(s[n])++n;write(1,s,n);}
#define CHECK(x) do{if(!(x)){say("Underlay FAIL: " #x "\n");return __LINE__;}}while(0)
static int step(void *w,const char *s){
 say(s);struct native_ui_event e;
 for(;;){int r=MyOSUiPoll(w,&e,-1);if(r<0)return r;if(r==1&&e.type==NATIVE_UI_KEY_DOWN&&e.character=='n')return 0;}
}
static uint32_t base[8*8],video[2*2]={0xff0000ff,0xff00ff00,0xffff0000,0xff00ffff};
static int64_t present(uint64_t w,struct native_video_request *r){return call(MYOS_NATIVE_VIDEO_SYSCALL,w,(uintptr_t)r,sizeof(*r));}
uint64_t _start(void){
 CHECK(call(MYOS_NATIVE_VIDEO_SYSCALL,0,0,0)==MYOS_NATIVE_VIDEO_ABI);
 struct native_video_request r={.action=NATIVE_VIDEO_SET,.pixels=(uintptr_t)video,.width=2,.height=2,.stride=8,.x=680,.y=520,.display_width=8,.display_height=8,.timestamp=10};
 CHECK(present(1,&r)==-9);
 void *window=SbWindowCreate(0);uint64_t w=(uintptr_t)window;CHECK(w);
 CHECK(call(MYOS_NATIVE_VIDEO_SYSCALL,w,0,sizeof(r))==-14);
 CHECK(call(MYOS_NATIVE_VIDEO_SYSCALL,w,(uintptr_t)&r,sizeof(r)-1)==-22);
 CHECK(present(w+1,&r)==-9);
 for(unsigned y=0;y<8;++y)for(unsigned x=0;x<8;++x)base[y*8+x]=x<2?0xff332211:x<4?0x80000040:0;
 struct native_ui_present b={(uintptr_t)base,8,8,32,NATIVE_UI_RGBA8,680,520};
 CHECK(!MyOSUiPresent(window,&b));CHECK(!present(w,&r));
 struct native_video_request bad=r;bad.flags=1;CHECK(present(w,&bad)==-22);
 bad=r;bad.reserved=1;CHECK(present(w,&bad)==-22);
 bad=r;bad.timestamp=9;CHECK(present(w,&bad)==-22);
 bad=r;bad.width=161;CHECK(present(w,&bad)==-22);
 bad=r;bad.pixels=UINT64_MAX-3;CHECK(present(w,&bad)==-14);
 bad=r;bad.stride=9;CHECK(present(w,&bad)==-22);
 bad=r;bad.display_height=4097;CHECK(present(w,&bad)==-22);
 unsigned char *mapping=mmap(0,8192,3,34,-1,0);CHECK(mapping!=(void *)(uintptr_t)-1);
 CHECK(!mprotect(mapping+4096,4096,0));
 bad=r;bad.pixels=(uintptr_t)(mapping+4096-8);CHECK(present(w,&bad)==-14);
 CHECK(!munmap(mapping,8192));
 CHECK(!step(window,"Underlay blend ready\n"));
 if(MODE==1){say("Underlay cancel ready\n");for(;;)call(4,0,0,0);}
 if(MODE==2){say("Underlay fault ready\n");__asm__ volatile("ud2");}
 struct native_video_request clear={.action=NATIVE_VIDEO_CLEAR};
 CHECK(!present(w,&clear));CHECK(!step(window,"Underlay clear ready\n"));
 r.timestamp=1;r.x=684;CHECK(!present(w,&r));CHECK(!step(window,"Underlay move ready\n"));
 CHECK(call(47,w,0,0)==0);r.timestamp=2;CHECK(!present(w,&r));CHECK(call(47,w,1,0)==0);
 CHECK(!step(window,"Underlay show ready\n"));
 CHECK(SbWindowDestroy(window));CHECK(present(w,&clear)==-9);
 window=SbWindowCreate(0);CHECK(window&&(uintptr_t)window!=w);
 CHECK(!step(window,"Underlay reacquire ready\n"));CHECK(SbWindowDestroy(window));
 say("Underlay validation/blend/clear/move/hide/show/reuse PASS\n");return 55;
}
