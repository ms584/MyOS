/* Native Starboard main-thread events. No host event loop or fake renderer. */
#include "starboard-event-abi.h"
#include "native-ui.h"
#include "starboard-sabi.h"
#define EVENT_GLOBAL 0x803f0106b0ull /* main TLS bytes 1712..1743 */
#define EVENT_LIMIT 128u
struct pending { uint32_t id;int64_t deadline;void (*callback)(void *);void *context; };
struct loop {
    struct pending pending[EVENT_LIMIT];
    void (*handler)(const struct sb_event *);
    uint64_t thread,window,pressed[8];
    uint32_t dropped,buttons;
    int stop,status,conceal,reveal,blur,focus,stage,input_turn,pointer_x,pointer_y;
};
struct globals { volatile uint64_t lock;struct loop *loop;uint64_t next_id,phase; };
extern void *calloc(size_t,size_t);extern void free(void *);
extern uint64_t pthread_self(void);
extern int *__errno_location(void);
extern void *MyOSUiWindow(void);
extern int MyOSUiPoll(void *,struct native_ui_event *,int);
extern int MyOSUiQuery(void *,struct native_ui_info *);
extern int MyOSUiSetVisible(bool);
extern volatile uint64_t *MyOSUiSequence(void);
extern void MyOSUiWake(void);
extern bool SbWindowDestroy(void *);
static struct globals *globals(void){return (void *)(uintptr_t)EVENT_GLOBAL;}
static int64_t call(uint64_t number,uint64_t a,uint64_t b,uint64_t c)
{
    register uint64_t result __asm__("rax")=number;
    __asm__ volatile("int $0x80":"+a"(result):"D"(a),"S"(b),"d"(c):"memory","cc");return (int64_t)result;
}
static void lock(void)
{while(__atomic_exchange_n(&globals()->lock,1,__ATOMIC_ACQUIRE))call(4,0,0,0);}
static void unlock(void){__atomic_store_n(&globals()->lock,0,__ATOMIC_RELEASE);}
static int64_t now(void)
{uint64_t value=(uint64_t)call(27,1,0,0)/1000;return value>INT64_MAX?INT64_MAX:(int64_t)value;}
static struct loop *prepare(void)
{
    if(globals()->phase==2)return 0;
    if(!globals()->loop)globals()->loop=calloc(1,sizeof(struct loop));
    return globals()->loop;
}
uint32_t SbEventSchedule(void (*callback)(void *),void *context,int64_t delay)
{
    if(!callback)return 0;
    lock();struct loop *loop=prepare();uint32_t id=0;
    if(loop&&globals()->next_id<UINT32_MAX){
        for(unsigned i=0;i<EVENT_LIMIT;++i)if(!loop->pending[i].id){
            int64_t instant=now();if(delay<0)delay=0;
            int64_t deadline=delay>INT64_MAX-instant?INT64_MAX:instant+delay;
            id=(uint32_t)++globals()->next_id;
            loop->pending[i]=(struct pending){id,deadline,callback,context};break;
        }
    }
    unlock();if(id)MyOSUiWake();return id;
}
void SbEventCancel(uint32_t id)
{
    if(!id)return;lock();struct loop *loop=globals()->loop;
    if(loop)for(unsigned i=0;i<EVENT_LIMIT;++i)if(loop->pending[i].id==id){loop->pending[i].id=0;break;}
    unlock();MyOSUiWake();
}
void SbSystemRequestStop(int status)
{
    lock();struct loop *loop=globals()->loop;
    if(globals()->phase==1&&loop&&!loop->stop){loop->stop=1;loop->status=status;}
    unlock();MyOSUiWake();
}
void SbSystemRequestConceal(void)
{
    lock();struct loop *loop=prepare();if(loop)loop->conceal=1;
    unlock();MyOSUiWake();
}
void SbSystemRequestReveal(void)
{
    lock();if(globals()->phase==1&&globals()->loop)globals()->loop->reveal=1;
    unlock();MyOSUiWake();
}
void SbSystemRequestBlur(void)
{
    lock();if(globals()->phase==1&&globals()->loop)globals()->loop->blur=1;
    unlock();MyOSUiWake();
}
void SbSystemRequestFocus(void)
{
    lock();if(globals()->phase==1&&globals()->loop)globals()->loop->focus=1;
    unlock();MyOSUiWake();
}
bool SbSystemSupportsResume(void){return false;} /* No frozen-process resume facility. */
static void dispatch(struct loop *loop,int type,void *data,int64_t timestamp)
{struct sb_event event={type,timestamp,data};loop->handler(&event);}
/* Set-1 scan identity -> pinned SbKey (Windows virtual keys), NOT ASCII text. */
static unsigned key(unsigned scan,int *location)
{
    static const uint8_t plain[128]={
        [1]=27,[2]='1',[3]='2',[4]='3',[5]='4',[6]='5',[7]='6',[8]='7',[9]='8',[10]='9',[11]='0',
        [12]=0xbd,[13]=0xbb,[14]=8,[15]=9,[16]='Q',[17]='W',[18]='E',[19]='R',[20]='T',[21]='Y',
        [22]='U',[23]='I',[24]='O',[25]='P',[26]=0xdb,[27]=0xdd,[28]=13,[29]=17,[30]='A',[31]='S',
        [32]='D',[33]='F',[34]='G',[35]='H',[36]='J',[37]='K',[38]='L',[39]=0xba,[40]=0xde,[41]=0xc0,
        [42]=16,[43]=0xdc,[44]='Z',[45]='X',[46]='C',[47]='V',[48]='B',[49]='N',[50]='M',[51]=0xbc,
        [52]=0xbe,[53]=0xbf,[54]=16,[55]=0x6a,[56]=18,[57]=32,[58]=20,
        [59]=0x70,[60]=0x71,[61]=0x72,[62]=0x73,[63]=0x74,[64]=0x75,[65]=0x76,[66]=0x77,[67]=0x78,
        [68]=0x79,[69]=0x90,[70]=0x91,[71]=0x67,[72]=0x68,[73]=0x69,[74]=0x6d,[75]=0x64,[76]=0x65,
        [77]=0x66,[78]=0x6b,[79]=0x61,[80]=0x62,[81]=0x63,[82]=0x60,[83]=0x6e,[86]=0xe2,[87]=0x7a,[88]=0x7b
    };
    *location=0;
    if(scan&0x100){
        switch(scan&127){
        case 28:return 13;case 29:*location=2;return 17;case 53:return 0x6f;
        case 56:*location=2;return 18;case 71:return 0x24;case 72:return 0x26;case 73:return 0x21;
        case 75:return 0x25;case 77:return 0x27;case 79:return 0x23;case 80:return 0x28;case 81:return 0x22;
        case 82:return 0x2d;case 83:return 0x2e;case 91:*location=1;return 0x5b;
        case 92:*location=2;return 0x5c;case 93:return 0x5d;default:return 0;
        }
    }
    if(scan>=128)return 0;
    if(scan==42||scan==29||scan==56)*location=1;
    if(scan==54)*location=2;return plain[scan];
}
static unsigned modifiers(unsigned raw,unsigned buttons)
{
    return (raw&NATIVE_UI_ALT?1:0)|(raw&NATIVE_UI_CONTROL?2:0)|
        (raw&NATIVE_UI_META?4:0)|(raw&NATIVE_UI_SHIFT?8:0)|
        ((buttons&7)<<4);
}
static void input(struct loop *loop,void *window,const struct native_ui_event *raw)
{
    union { uint32_t bits;float value; } nan={0x7fc00000};
    struct sb_input_data data={0};data.window=window;
    data.pressure=nan.value;data.tilt=(struct sb_vector){nan.value,nan.value};
    data.key_modifiers=modifiers(raw->modifiers,raw->buttons);
    int64_t timestamp=raw->tick>(uint64_t)INT64_MAX/10000?INT64_MAX:(int64_t)(raw->tick*10000);
    if(raw->type==NATIVE_UI_KEY_DOWN||raw->type==NATIVE_UI_KEY_UP){
        data.device_type=2;data.device_id=1;data.key=(int)key(raw->scan,&data.key_location);
        if(!data.key)return;
        data.type=raw->type==NATIVE_UI_KEY_DOWN?1:2;data.character=(int32_t)raw->character;
        data.key_modifiers=modifiers(raw->modifiers,loop->buttons);
        if(raw->scan<512){uint64_t mask=1ull<<(raw->scan&63);
            if(data.type==1)loop->pressed[raw->scan/64]|=mask;else loop->pressed[raw->scan/64]&=~mask;}
        dispatch(loop,SB_INPUT,&data,timestamp);return;
    }
    data.device_type=3;data.device_id=2;
    data.position=(struct sb_vector){(float)raw->x,(float)raw->y};
    loop->pointer_x=raw->x;loop->pointer_y=raw->y;
    loop->buttons=raw->buttons;
    if(raw->dx||raw->dy){data.type=0;data.delta=(struct sb_vector){(float)raw->dx,(float)raw->dy};
        dispatch(loop,SB_INPUT,&data,timestamp);data.delta=(struct sb_vector){0,0};}
    for(unsigned i=0;i<3&&MyOSUiWindow()==window;++i)if(raw->changed&(1u<<i)){
        /* Pinned X11 adapter: Mouse1=left, Mouse2=middle, Mouse3=right.
         * PS/2 bits instead encode left/right/middle. Modifier bits stay L/R/M. */
        static const uint16_t mouse_key[3]={0x7000,0x7002,0x7001};
        data.type=raw->buttons&(1u<<i)?1:2;data.key=mouse_key[i];
        dispatch(loop,SB_INPUT,&data,timestamp);
    }
}
static void release_input(struct loop *loop)
{
    void *window=MyOSUiWindow();uint64_t instant=(uint64_t)now()/10000;
    for(unsigned i=0;window&&i<512&&MyOSUiWindow()==window;++i)if(loop->pressed[i/64]&(1ull<<(i&63))){
        struct native_ui_event event={.type=NATIVE_UI_KEY_UP,.scan=i,.tick=instant};input(loop,window,&event);
    }
    if(loop->buttons&&window&&MyOSUiWindow()==window){struct native_ui_event event={.type=NATIVE_UI_POINTER,
        .x=loop->pointer_x,.y=loop->pointer_y,.changed=loop->buttons,.tick=instant};input(loop,window,&event);}
    for(unsigned i=0;i<8;++i)loop->pressed[i]=0;loop->buttons=0;
}
static int transition(struct loop *loop,bool stop)
{
    if(loop->stage==SB_START){release_input(loop);dispatch(loop,SB_BLUR,0,now());loop->stage=SB_BLUR;}
    if(loop->stage==SB_BLUR){
        if(MyOSUiSetVisible(false))return -1;
        dispatch(loop,SB_CONCEAL,0,now());loop->stage=SB_CONCEAL;
    }
    if(stop){dispatch(loop,SB_FREEZE,0,now());loop->stage=SB_FREEZE;
        dispatch(loop,SB_STOP,0,now());loop->stage=SB_STOP;}
    return 0;
}
static void drain_input(void)
{
    void *window=MyOSUiWindow();struct native_ui_event event;
    /* Discard blurred input, bounded even if interrupts keep producing it. */
    for(unsigned i=0;window&&i<128&&MyOSUiPoll(window,&event,0)==1;++i){}
}
static int run(int argc,char **argv,void (*handler)(const struct sb_event *),const char *link)
{
    if(!handler||argc<0||argc>64||(argc&&!argv)){*__errno_location()=22;return -1;}
    lock();if(globals()->phase){unlock();*__errno_location()=16;return -1;}
    struct loop *loop=prepare();
    if(!loop){unlock();*__errno_location()=12;return -1;}
    int preload=loop->conceal;loop->conceal=0;
    loop->handler=handler;loop->thread=pthread_self();loop->stage=preload?SB_CONCEAL:SB_START;
    globals()->phase=1;unlock();
    int result=0;
    if(MyOSUiSetVisible(!preload)){result=95;goto cleanup;}
    struct sb_start_data start={argv,argc,link};
    dispatch(loop,preload?SB_PRELOAD:SB_START,preload?0:&start,now());
    for(;;){
        uint64_t sequence=__atomic_load_n(MyOSUiSequence(),__ATOMIC_ACQUIRE);
        struct pending pending={0};int stop,status,action=0;int64_t deadline=INT64_MAX,instant=now();
        lock();stop=loop->stop;status=loop->status;
        /* Consume one request at a time. In particular conceal must not erase
         * a reveal queued by another thread or by a lifecycle callback. */
        if(loop->conceal){action=SB_CONCEAL;loop->conceal=0;}
        else if(loop->reveal){action=SB_REVEAL;loop->reveal=0;}
        else if(loop->blur){action=SB_BLUR;loop->blur=0;}
        else if(loop->focus){action=SB_FOCUS;loop->focus=0;}
        unsigned chosen=EVENT_LIMIT;
        for(unsigned i=0;i<EVENT_LIMIT;++i){struct pending *item=&loop->pending[i];
            if(item->id&&(chosen==EVENT_LIMIT||item->deadline<deadline||
                         (item->deadline==deadline&&item->id<loop->pending[chosen].id))){chosen=i;deadline=item->deadline;}}
        if(!stop&&!action&&!loop->input_turn&&chosen<EVENT_LIMIT&&deadline<=instant){
            pending=loop->pending[chosen];loop->pending[chosen].id=0;}
        unlock();
        if(stop){result=status;if(transition(loop,true))result=95;break;}
        if(action==SB_CONCEAL){if(transition(loop,false)){result=95;break;}continue;}
        if(action==SB_REVEAL||action==SB_FOCUS){
            if(loop->stage==SB_CONCEAL){
                if(MyOSUiSetVisible(true)){result=95;break;}
                dispatch(loop,SB_REVEAL,0,now());loop->stage=SB_BLUR;
            }
            if(action==SB_FOCUS&&loop->stage==SB_BLUR){
                drain_input();dispatch(loop,SB_FOCUS,0,now());loop->stage=SB_START;
            }
            continue;
        }
        if(action==SB_BLUR){
            if(loop->stage==SB_START){release_input(loop);dispatch(loop,SB_BLUR,0,now());loop->stage=SB_BLUR;}
            continue;
        }
        if(pending.id){pending.callback(pending.context);loop->input_turn=1;continue;}
        void *window=MyOSUiWindow();
        if(window!= (void *)(uintptr_t)loop->window){
            loop->window=(uintptr_t)window;loop->dropped=0;
            for(unsigned i=0;i<8;++i)loop->pressed[i]=0;loop->buttons=0;
        }
        if(window&&loop->stage==SB_START){
            struct native_ui_info info;
            if(!MyOSUiQuery(window,&info)&&info.dropped!=loop->dropped){release_input(loop);loop->dropped=info.dropped;}
            struct native_ui_event event;int rc=MyOSUiPoll(window,&event,0);
            if(rc==1){input(loop,window,&event);loop->input_turn=0;continue;}
        }
        /* An endless chain of immediately scheduled work must not starve
         * focused input, nor may continuous input starve overdue callbacks. */
        if(loop->input_turn&&deadline<=instant){loop->input_turn=0;continue;}
        uint64_t wake=0;
        if(deadline!=INT64_MAX){
            uint64_t tick=(uint64_t)call(13,0,0,0),delta=deadline>instant?(uint64_t)(deadline-instant):0;
            uint64_t add=delta/10000+(delta%10000!=0);
            wake=add>UINT64_MAX-1-tick?UINT64_MAX-1:tick+add;
            if(!wake)wake=1;
        }
        call(20,(uintptr_t)MyOSUiSequence(),sequence,wake);
    }
cleanup:;
    void *window=MyOSUiWindow();if(window)SbWindowDestroy(window);MyOSUiSetVisible(true);
    lock();globals()->phase=2;globals()->loop=0;unlock();free(loop);return result;
}
int SbRunStarboardMain(int argc,char **argv,void (*handler)(const struct sb_event *))
{return run(argc,argv,handler,0);}
int MyOSStarboardRun(int argc,char **argv,void (*handler)(const struct sb_event *),const char *link)
{return run(argc,argv,handler,link);}
int MyOSStarboardValidate(const char *(*get_sabi)(void))
{
    static const char expected[]=MYOS_STARBOARD_SABI;
    const char *actual=get_sabi?get_sabi():0;
    if(actual){unsigned i=0;for(;i<sizeof(expected)&&actual[i]==expected[i];++i){}
        if(i==sizeof(expected)){const char ok[]="Starboard SABI: pinned x64 API 18 matched\n";
            call(1,(uintptr_t)ok,sizeof(ok)-1,0);return 0;}}
    const char bad[]="Starboard SABI: mismatch; constructors not executed\n";
    call(1,(uintptr_t)bad,sizeof(bad)-1,0);return 95;
}
