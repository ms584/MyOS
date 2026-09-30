/* Ring 3 event ABI fixture, compiled against the pinned upstream declarations. */
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "event.h"
#include "input.h"
#include "native-ui.h"
#ifndef MODE
#define MODE 0
#endif
_Static_assert(sizeof(SbEvent)==24&&offsetof(SbEvent,timestamp)==8,"event ABI");
_Static_assert(sizeof(SbEventStartData)==24&&offsetof(SbEventStartData,link)==16,"start ABI");
_Static_assert(sizeof(SbInputData)==88&&offsetof(SbInputData,input_text)==72,"input ABI");
_Static_assert(sizeof(SbEventId)==4&&kSbEventTypeInput==9,"enum ABI");
extern void SbSystemRequestStop(int),SbSystemRequestConceal(void),SbSystemRequestReveal(void);
extern void SbSystemRequestFocus(void),SbSystemRequestBlur(void);
extern bool SbSystemSupportsResume(void);
extern int MyOSUiPresent(void *,const struct native_ui_present *);
extern void *MyOSUiWindow(void);
extern uint64_t pthread_self(void);
extern int pthread_create(uint64_t *,const void *,void *(*)(void *),void *),pthread_join(uint64_t,void **);
extern int *__errno_location(void);
extern int64_t write(int,const void *,size_t);
static int64_t call(uint64_t n,uint64_t a,uint64_t b,uint64_t c)
{register uint64_t r __asm__("rax")=n;__asm__ volatile("int $0x80":"+a"(r):"D"(a),"S"(b),"d"(c):"memory","cc");return (int64_t)r;}
static void say(const char *s){size_t n=0;while(s[n])++n;write(1,s,n);}
#define CHECK(c) do{if(!(c)){say("Native events FAIL: " #c "\n");call(3,99,0,0);for(;;)call(4,0,0,0);}}while(0)
static int64_t micros(void){return call(27,1,0,0)/1000;}
static uint64_t main_thread,worker_thread;
static SbWindow window;
static uint32_t pixels[96*64];
static unsigned lifecycle[24],life_count,seen,fifo,negative_called,timer_called,worker_called,chain_count;
static unsigned concealed,focuses,c_down,c_up,worker_done,loop_done;
static int64_t timer_start;
static SbEventId cancelled;
static volatile uint64_t never_wake;
static void handler(const SbEvent *event);
static void main_only(void){CHECK(pthread_self()==main_thread);}
static void paint(uint32_t color)
{
    for(unsigned i=0;i<96*64;++i)pixels[i]=color;
    struct native_ui_present request={(uintptr_t)pixels,96,64,96*4,NATIVE_UI_RGBA8,688,520};
    CHECK(!MyOSUiPresent(window,&request));
}
static void forbidden(void *context){(void)context;CHECK(0);}
static void negative(void *context){main_only();CHECK(context==(void *)0x1234);++negative_called;}
static void ordered(void *context){main_only();CHECK((uintptr_t)context==fifo+1);++fifo;}
static void timed(void *context){(void)context;main_only();CHECK(micros()-timer_start>=80000);++timer_called;}
static void chain(void *context)
{
    (void)context;main_only();++chain_count;
    if(seen!=131071)CHECK(SbEventSchedule(chain,0,0));
}
static void worker_callback(void *context){main_only();CHECK(context==(void *)0x5678);++worker_called;}
static void *worker(void *context)
{
    (void)context;CHECK(pthread_self()!=main_thread);
    uint64_t end=(uint64_t)call(13,0,0,0)+12;
    while((uint64_t)call(13,0,0,0)<end)call(4,0,0,0);
    SbEventId id=SbEventSchedule(forbidden,0,100000);CHECK(id);SbEventCancel(id);
    CHECK(SbEventSchedule(worker_callback,(void *)0x5678,0));
    __atomic_store_n(&worker_done,1,__ATOMIC_RELEASE);return 0;
}
static void *blocked_worker(void *context)
{(void)context;for(;;)call(20,(uintptr_t)&never_wake,0,0);return 0;}
static void stop(void *context){(void)context;main_only();if(MODE==5)say("Native events stop callback\n");
    SbSystemRequestStop(56);SbSystemRequestStop(89);}
static void *infinite_worker(void *context)
{
    (void)context;uint64_t end=(uint64_t)call(13,0,0,0)+12;
    while((uint64_t)call(13,0,0,0)<end)call(4,0,0,0);
    CHECK(SbEventSchedule(stop,0,0)); /* Main is waiting with no deadline/work. */
    say("Native events worker scheduled stop\n");
    while(!__atomic_load_n(&loop_done,__ATOMIC_ACQUIRE)){
        SbEventId id=SbEventSchedule(forbidden,0,INT64_MAX);if(id)SbEventCancel(id);call(4,0,0,0);
    }
    CHECK(!SbEventSchedule(forbidden,0,0));SbEventCancel(UINT32_MAX);return 0;
}
static void reveal(void *context){(void)context;main_only();SbSystemRequestReveal();}
static void focus(void *context){(void)context;main_only();SbSystemRequestFocus();}
static void ready(void *context)
{
    (void)context;main_only();CHECK(negative_called==1&&fifo==3&&timer_called==1&&worker_called==1);
    CHECK(__atomic_load_n(&worker_done,__ATOMIC_ACQUIRE));CHECK(!pthread_join(worker_thread,0));
    CHECK(SbEventSchedule(chain,0,0));say("Native events frame ready\n");
}
static void input(const SbInputData *data)
{
    CHECK(data&&data->window==window&&!data->input_text&&!data->is_composing);
    CHECK(data->pressure!=data->pressure&&data->tilt.x!=data->tilt.x&&data->tilt.y!=data->tilt.y);
    CHECK(data->size.x==0&&data->size.y==0);
    unsigned press=data->type==kSbInputEventTypePress;
    if(MODE==2&&data->key==kSbKeyB&&press){__asm__ volatile("ud2");}
    if(MODE==4||MODE==6){CHECK(data->key!=kSbKeyB&&data->key!=kSbKeyX);return;}
    if(MODE!=0)return;
    CHECK(data->key!=kSbKeyB&&data->key!=kSbKeyX); /* Hidden/blurred input cannot be replayed. */
    if(data->device_type==kSbInputDeviceTypeKeyboard){
        CHECK(data->device_id==1&&data->position.x==0&&data->position.y==0);
        CHECK(data->delta.x==0&&data->delta.y==0);
        CHECK(data->type==kSbInputEventTypePress||data->type==kSbInputEventTypeUnpress);
        if(data->key==kSbKeyA){
            if(press){CHECK(data->character=='A'&&(data->key_modifiers&kSbKeyModifiersShift));seen|=1;}
            else{CHECK(data->character==0);seen|=2;}
        }
        if(data->key==kSbKeyDown){CHECK(!data->character);seen|=press?4:8;}
        if(data->key==kSbKeyControl){CHECK(data->key_location==kSbKeyLocationRight);
            CHECK(!press||(data->key_modifiers&kSbKeyModifiersCtrl));seen|=press?16:32;}
        if(data->key==kSbKeyMenu){CHECK(data->key_location==kSbKeyLocationRight);
            CHECK(!press||(data->key_modifiers&kSbKeyModifiersAlt));seen|=press?64:128;}
        if(data->key==kSbKeyRwin){CHECK(data->key_location==kSbKeyLocationRight);
            CHECK(!press||(data->key_modifiers&kSbKeyModifiersMeta));seen|=press?256:512;}
        if(data->key==kSbKeyC){CHECK(focuses==1);if(press)++c_down;else ++c_up;}
    }else{
        if(MODE==5){say("Native events teardown event\n");}
        CHECK(data->device_type==kSbInputDeviceTypeMouse&&data->device_id==2);
        CHECK(data->position.x>=0&&data->position.x<800&&data->position.y>=0&&data->position.y<600);
        if(data->type==kSbInputEventTypeMove){CHECK(data->delta.x||data->delta.y);seen|=65536;}
        else{
            CHECK(data->delta.x==0&&data->delta.y==0);
            CHECK(data->key>=kSbKeyMouse1&&data->key<=kSbKeyMouse3);
            unsigned index=(unsigned)data->key-kSbKeyMouse1;
            const unsigned masks[]={kSbKeyModifiersPointerButtonLeft,kSbKeyModifiersPointerButtonMiddle,
                                    kSbKeyModifiersPointerButtonRight};
            CHECK((!!(data->key_modifiers&masks[index]))==press);
            seen|=1u<<(10+2*index+!press);
        }
    }
    if(seen==131071&&!concealed){
        concealed=1;CHECK(chain_count);say("Native events real input checked\n");SbSystemRequestConceal();
    }
}
static void handler(const SbEvent *event)
{
    main_only();CHECK(event&&event->timestamp>=0);
    if(event->type==kSbEventTypeInput){input(event->data);return;}
    CHECK(life_count<24);lifecycle[life_count++]=(unsigned)event->type;
    if(event->type==kSbEventTypeStart||event->type==kSbEventTypePreload){
        if(MODE==3){CHECK(event->type==kSbEventTypePreload&&!event->data);}
        else{
            CHECK(event->type==kSbEventTypeStart&&event->data);
            SbEventStartData *start=event->data;
            CHECK(start->argument_count==2&&start->argument_values&&start->argument_values[0][0]=='e');
            CHECK(start->argument_values[1][0]=='h'&&!start->link);
        }
        CHECK(SbRunStarboardMain(0,0,handler)==-1&&*__errno_location()==16);
        CHECK(!SbSystemSupportsResume());window=SbWindowCreate(0);CHECK(window);
        CHECK(call(47,(uintptr_t)window,2,0)==-22);
        CHECK(call(47,(uintptr_t)window,1,1)==-22);
        CHECK(call(47,(uintptr_t)window+1,1,0)==-9);
        paint(0xff0000ffu);
        if(MODE==1||MODE==2){
            CHECK(!pthread_create(&worker_thread,0,blocked_worker,0));
            CHECK(SbEventSchedule(forbidden,0,INT64_MAX));
            say(MODE==1?"Native events cancel ready\n":"Native events fault ready\n");return;
        }
        if(MODE==3){say("Native events preload ready\n");CHECK(SbEventSchedule(reveal,0,1500000));return;}
        if(MODE==4){SbSystemRequestConceal();SbSystemRequestReveal();return;}
        if(MODE==5){CHECK(!pthread_create(&worker_thread,0,infinite_worker,0));
            say("Native events infinite ready\n");return;}
        if(MODE==6){SbSystemRequestBlur();SbSystemRequestBlur();return;}
        SbEventCancel(cancelled);CHECK(!pthread_create(&worker_thread,0,worker,0));
        CHECK(SbEventSchedule(ready,0,250000));
    }else{
        CHECK(!event->data);
        if(MODE==6&&event->type==kSbEventTypeBlur&&life_count==2){
            say("Native events explicit blurred ready\n");CHECK(SbEventSchedule(focus,0,1500000));
        }
        if(event->type==kSbEventTypeConceal&&MODE==0&&life_count==3){
            paint(0xff00ff00u);say("Native events hidden ready\n");CHECK(SbEventSchedule(reveal,0,1500000));
        }
        if(event->type==kSbEventTypeReveal){
            say("Native events reveal blurred ready\n");CHECK(SbEventSchedule(focus,0,1500000));
        }
        if(event->type==kSbEventTypeFocus){
            ++focuses;say("Native events focused ready\n");CHECK(SbEventSchedule(stop,0,1000000));
        }
        if(event->type==kSbEventTypeFreeze){CHECK(SbWindowDestroy(window));window=0;}
    }
}
uint64_t _start(void)
{
    main_thread=pthread_self();CHECK(!SbEventSchedule(0,0,0));SbEventCancel(0);SbEventCancel(UINT32_MAX);
    CHECK(SbRunStarboardMain(0,0,0)==-1&&*__errno_location()==22);
    CHECK(SbRunStarboardMain(-1,0,handler)==-1&&*__errno_location()==22);
    CHECK(SbRunStarboardMain(1,0,handler)==-1&&*__errno_location()==22);
    if(MODE==0){
        SbEventId ids[128];for(unsigned i=0;i<128;++i){ids[i]=SbEventSchedule(forbidden,0,INT64_MAX);CHECK(ids[i]);
            CHECK(!i||ids[i]>ids[i-1]);}
        CHECK(!SbEventSchedule(forbidden,0,INT64_MAX));SbEventCancel(ids[0]);
        SbEventId newer=SbEventSchedule(forbidden,0,INT64_MAX);CHECK(newer>ids[127]);
        SbEventCancel(ids[0]);CHECK(!SbEventSchedule(forbidden,0,INT64_MAX));SbEventCancel(newer);
        for(unsigned i=1;i<128;++i)SbEventCancel(ids[i]);
        CHECK(SbEventSchedule(negative,(void *)0x1234,-1));
        for(unsigned i=1;i<=3;++i)CHECK(SbEventSchedule(ordered,(void *)(uintptr_t)i,0));
        cancelled=SbEventSchedule(forbidden,0,0);CHECK(cancelled);
        timer_start=micros();CHECK(SbEventSchedule(timed,0,80000));
    }
    if(MODE==3)SbSystemRequestConceal();
    char *argv[]={"events","https://fixture.invalid/",0};
    CHECK(SbRunStarboardMain(2,argv,handler)==56);
    if(MODE==5)say("Native events loop returned\n");
    __atomic_store_n(&loop_done,1,__ATOMIC_RELEASE);
    if(MODE==5)CHECK(!pthread_join(worker_thread,0));
    const unsigned normal[]={1,2,4,5,3,2,4,6,8},preloaded[]={0,5,3,2,4,6,8},simple[]={1,2,4,6,8};
    const unsigned blurred[]={1,2,3,2,4,6,8};
    CHECK(life_count==(MODE==5?5u:MODE==3||MODE==6?7u:9u));
    for(unsigned i=0;i<life_count;++i)
        CHECK(lifecycle[i]==(MODE==5?simple[i]:MODE==6?blurred[i]:MODE==3?preloaded[i]:normal[i]));
    if(MODE==0)CHECK(c_down==1&&c_up==1); /* Stop synthesized release of the held key. */
    CHECK(!MyOSUiWindow()&&!SbEventSchedule(forbidden,0,0));SbEventCancel(cancelled);
    CHECK(SbRunStarboardMain(0,0,handler)==-1&&*__errno_location()==16);
    say("Native events ABI/schedule/cancel/main-thread/lifecycle/input PASS\n");return 56;
}
