/* Real native launcher boundary fixture, not the Cobalt engine. */
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "event.h"
#include "input.h"
#include "native-ui.h"
#include "starboard-sabi.h"
#ifndef MODE
#define MODE 0
#endif
extern int64_t write(int,const void *,size_t);
extern uint64_t pthread_self(void);
extern int pthread_getattr_np(uint64_t,void *),pthread_attr_getstack(const void *,void **,size_t *);
extern int getrlimit(int,void *);
extern void SbSystemRequestStop(int);
static int64_t call(uint64_t n,uint64_t a,uint64_t b,uint64_t c)
{register uint64_t r __asm__("rax")=n;__asm__ volatile("int $0x80":"+a"(r):"D"(a),"S"(b),"d"(c):"memory","cc");return (int64_t)r;}
static void say(const char *s){size_t n=0;while(s[n])++n;write(1,s,n);}
#define CHECK(c) do{if(!(c)){say("Starboard launcher FAIL: " #c "\n");call(3,99,0,0);for(;;)call(4,0,0,0);}}while(0)
static int equal(const char *a,const char *b){if(!a||!b)return a==b;while(*a&&*a==*b){++a;++b;}return *a==*b;}
static unsigned initialized,finalized,life;
static const char **arguments;
static SbWindow window;
static volatile uint64_t wait_word;
__attribute__((visibility("default"))) const char *GetEvergreenSabiString(void)
{
    say("Starboard launcher getter\n");
    if(MODE==1)return "wrong-sabi";
    if(MODE==2)__asm__ volatile("ud2");
    if(MODE==7)return (const char *)(uintptr_t)0x800fe04000ull;
    return MYOS_STARBOARD_SABI;
}
void init_first(void){CHECK(!initialized);initialized=1;say("Starboard launcher init\n");}
static void ctor(void){CHECK(initialized==1);initialized=2;say("Starboard launcher constructor\n");}
__attribute__((section(".init_array"),used)) static void (*const ctor_slot)(void)=ctor;
static void fini(void){CHECK(initialized==2&&life==5);++finalized;say("Starboard launcher destructor\n");}
__attribute__((section(".fini_array"),used)) static void (*const fini_slot)(void)=fini;
void fini_last(void){CHECK(finalized==1);say("Starboard launcher finalizer PASS\n");}
static void forbidden(void *p){(void)p;CHECK(0);}
__attribute__((noinline)) static void stack_probe(unsigned depth)
{
    volatile unsigned char page[4096];
    for(unsigned i=0;i<sizeof(page);++i)page[i]=(unsigned char)(i+depth);
    if(depth)stack_probe(depth-1);
    for(unsigned i=0;i<sizeof(page);++i)CHECK(page[i]==(unsigned char)(i+depth));
}
__attribute__((visibility("default"))) void SbEventHandle(const SbEvent *event)
{
    CHECK(initialized==2&&event);
    if(event->type==kSbEventTypeInput){
        const SbInputData *input=event->data;CHECK(input&&input->window==window);
        if(input->key==kSbKeyA&&input->type==kSbInputEventTypePress){
            say("Starboard launcher real input PASS\n");SbSystemRequestStop(57);}
        return;
    }
    const unsigned expected[]={1,2,4,6,8};CHECK(life<5&&(unsigned)event->type==expected[life++]);
    if(event->type==kSbEventTypeStart){
        SbEventStartData *start=event->data;CHECK(start&&start->argument_values);
        CHECK(start->argument_count==1||start->argument_count==2);arguments=(const char **)start->argument_values;
        CHECK(arguments[start->argument_count]==0&&arguments[0][0]);
        if(start->argument_count==2){CHECK(equal(arguments[1],"--url=https://myos.test/a?b=1&c=2"));
            CHECK(equal(start->link,"https://myos.test/a?b=1&c=2"));}
        else CHECK(!start->link);
        uint64_t info[4],attr[7],limit[2];void *base=0;size_t length=0;
        CHECK(!call(48,(uintptr_t)info,0,0)&&info[1]==2*1024*1024&&info[3]==4096&&info[2]+4096==info[0]);
        CHECK(!pthread_getattr_np(pthread_self(),attr)&&!pthread_attr_getstack(attr,&base,&length));
        CHECK((uintptr_t)base==info[0]&&length==info[1]);
        CHECK(!getrlimit(3,limit)&&limit[0]==length&&limit[1]==length);
        CHECK(call(48,0,0,0)==-14&&call(48,(uintptr_t)info,1,0)==-22);
        CHECK(call(48,(uintptr_t)arguments,0,0)==-14); /* Read-only owned startup. */
        CHECK(call(48,(uintptr_t)info,0,1)==-22&&call(48,UINT64_MAX-15,0,0)==-14);
        /* The writable first half of an invalid output must not be changed. */
        uint64_t *edge=(void *)(uintptr_t)(info[0]+info[1]-16);edge[0]=0x12345678;edge[1]=0x87654321;
        CHECK(call(48,(uintptr_t)edge,0,0)==-14&&edge[0]==0x12345678&&edge[1]==0x87654321);
        stack_probe(63);say("Starboard launcher 256KiB stack/args PASS\n");
        if(MODE==3)*(volatile unsigned char *)(uintptr_t)info[2]=1;
        if(MODE==4)((void (*)(void))base)(); /* NX stack fault. */
        if(MODE==5){say("Starboard launcher cancel ready\n");for(;;)call(20,(uintptr_t)&wait_word,0,0);}
        if(MODE==6){window=SbWindowCreate(0);CHECK(window);say("Starboard launcher input ready\n");}
        else SbSystemRequestStop(57);
        CHECK(SbEventSchedule(forbidden,0,INT64_MAX));
    }
    if(event->type==kSbEventTypeStop){CHECK(arguments&&arguments[0][0]);say("Starboard launcher lifecycle PASS\n");}
}
