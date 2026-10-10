/* Pinned x64 Starboard window ABI plus private MyOS software-winsys helpers.
 * Compiled with SSE2 for float return/fields; kernel code stays general-regs-only. */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "native-ui.h"

#define UI_STATE_BASE 0x803f010690ull /* main shared TLS page, bytes 1680..1711 */
#define SYS_TICKS 13u
#define SYS_YIELD 4u
#define SYS_WAIT 20u
#define SYS_UI_QUERY 42u
#define SYS_UI_ACQUIRE 43u
#define SYS_UI_PRESENT 44u
#define SYS_UI_RELEASE 45u
#define SYS_UI_EVENT 46u
#define SYS_UI_VISIBLE 47u
struct SbWindowSize { int width,height;float video_pixel_ratio; };
struct SbWindowOptions { struct SbWindowSize size;bool windowed;const char *name; };
_Static_assert(sizeof(struct SbWindowSize)==12,"pinned window size ABI");
_Static_assert(sizeof(struct SbWindowOptions)==24&&offsetof(struct SbWindowOptions,name)==16,
               "pinned window options ABI");
struct window_state { volatile uint64_t lock,sequence;uint64_t window,concealed; };
int *__errno_location(void);
static struct window_state *state(void){return (struct window_state *)(uintptr_t)UI_STATE_BASE;}
static int64_t syscall3(uint64_t number,uint64_t first,uint64_t second,uint64_t third)
{
    register uint64_t a __asm__("rax")=number;
    register uint64_t d __asm__("rdi")=first;
    register uint64_t s __asm__("rsi")=second;
    register uint64_t x __asm__("rdx")=third;
    __asm__ volatile("int $0x80":"+a"(a):"D"(d),"S"(s),"d"(x):"memory","cc");return (int64_t)a;
}
static void lock(void)
{while(__atomic_exchange_n(&state()->lock,1,__ATOMIC_ACQUIRE))syscall3(SYS_YIELD,0,0,0);}
static void unlock(void){__atomic_store_n(&state()->lock,0,__ATOMIC_RELEASE);}
static int error(int64_t result)
{if(result<0){*__errno_location()=(int)-result;return -1;}return (int)result;}
int MyOSUiQuery(void *window,struct native_ui_info *info)
{return error(syscall3(SYS_UI_QUERY,(uintptr_t)window,(uintptr_t)info,0));}
void SbWindowSetDefaultOptions(struct SbWindowOptions *options)
{
    if(!options)return;
    for(unsigned i=0;i<sizeof(*options);++i)((unsigned char *)options)[i]=0;
    struct native_ui_info info;
    if(syscall3(SYS_UI_QUERY,0,(uintptr_t)&info,0)>=0){
        options->size.width=(int)info.width;options->size.height=(int)info.height;
    }
    options->size.video_pixel_ratio=1.0f;options->name="MyOS";
}
/* PRIVATE: numeric-only requested dimensions and outcome; no titles/URLs. */
static void diagnostic_window_request(const struct SbWindowOptions *options,int64_t result)
{
    char text[160];unsigned n=0;
    const char *label="DIAG window-create width height windowed existing result=";
    while(*label)text[n++]=*label++;
    uint64_t values[5]={options?(uint64_t)(int64_t)options->size.width:0,
        options?(uint64_t)(int64_t)options->size.height:0,
        options?options->windowed:0,state()->window,(uint64_t)result};
    for(unsigned j=0;j<5;++j){text[n++]=' ';text[n++]='0';text[n++]='x';
        for(int i=60;i>=0;i-=4)text[n++]="0123456789abcdef"[(values[j]>>i)&15];}
    text[n++]='\n';syscall3(1,(uintptr_t)text,n,0);
}
void *SbWindowCreate(const struct SbWindowOptions *options)
{
    struct native_ui_info info;
    if(syscall3(SYS_UI_QUERY,0,(uintptr_t)&info,0)<0)return 0;
    /* Boot mode is fixed: unsupported requested resolutions/windowed mode
     * cannot be silently advertised as successful. */
    if(options&&(options->windowed||options->size.width!=(int)info.width||
                options->size.height!=(int)info.height)){diagnostic_window_request(options,-95);return 0;}
    lock();int64_t handle=syscall3(SYS_UI_ACQUIRE,(uintptr_t)&state()->sequence,(uintptr_t)&info,0);
    if(handle>=0&&state()->concealed&&syscall3(SYS_UI_VISIBLE,(uint64_t)handle,0,0)<0){
        syscall3(SYS_UI_RELEASE,(uint64_t)handle,0,0);handle=-95;
    }
    if(handle>=0)state()->window=(uint64_t)handle;unlock();
    diagnostic_window_request(options,handle);return handle<0?0:(void *)(uintptr_t)handle;
}
bool SbWindowDestroy(void *window)
{
    if(!window)return false;
    lock();int64_t result=syscall3(SYS_UI_RELEASE,(uintptr_t)window,0,0);
    if(!result)state()->window=0;unlock();return result==0;
}
bool SbWindowGetSize(void *window,struct SbWindowSize *size)
{
    if(!window||!size)return false;struct native_ui_info info;
    if(syscall3(SYS_UI_QUERY,(uintptr_t)window,(uintptr_t)&info,0)<0)return false;
    struct SbWindowSize value={(int)info.width,(int)info.height,1.0f};*size=value;return true;
}
void *SbWindowGetPlatformHandle(void *window)
{
    if(!window)return 0;struct native_ui_info info;
    return syscall3(SYS_UI_QUERY,(uintptr_t)window,(uintptr_t)&info,0)<0?0:window;
}
float SbWindowGetDiagonalSizeInInches(void *window)
{(void)window;return 0.0f;} /* Physical diagonal is unknown, not a guessed value. */
int MyOSUiPresent(void *window,const struct native_ui_present *request)
{return error(syscall3(SYS_UI_PRESENT,(uintptr_t)window,(uintptr_t)request,sizeof(*request)));}
int MyOSUiPoll(void *window,struct native_ui_event *event,int timeout_ms)
{
    if(timeout_ms<-1){*__errno_location()=22;return -1;}
    uint64_t now=(uint64_t)syscall3(SYS_TICKS,0,0,0);
    uint64_t duration=timeout_ms>0?((uint64_t)timeout_ms+9)/10:0;
    uint64_t deadline=duration?(now>UINT64_MAX-duration?UINT64_MAX:now+duration):0;
    for(;;){
        uint64_t sequence=__atomic_load_n(&state()->sequence,__ATOMIC_ACQUIRE);
        int64_t rc=syscall3(SYS_UI_EVENT,(uintptr_t)window,(uintptr_t)event,sizeof(*event));
        if(rc!=0)return error(rc);
        if(!timeout_ms||(duration&&(uint64_t)syscall3(SYS_TICKS,0,0,0)>=deadline))return 0;
        rc=syscall3(SYS_WAIT,(uintptr_t)&state()->sequence,sequence,deadline);
        if(rc<0&&rc!=-11&&rc!=-110)return error(rc);
    }
}
/* Private event-pump integration; one sequence combines IRQ input and
 * scheduled work. Read sequence before checking BOTH queues, then wait. */
volatile uint64_t *MyOSUiSequence(void){return &state()->sequence;}
void MyOSUiWake(void)
{
    __atomic_add_fetch(&state()->sequence,1,__ATOMIC_RELEASE);
    /* SYS_WAKE count zero wakes nobody; UINT32_MAX broadcasts to waiters. */
    syscall3(21,(uintptr_t)&state()->sequence,UINT32_MAX,0);
}
void *MyOSUiWindow(void)
{
    lock();void *window=(void *)(uintptr_t)state()->window;unlock();return window;
}
int MyOSUiSetVisible(bool visible)
{
    lock();int64_t result=state()->window?syscall3(SYS_UI_VISIBLE,state()->window,visible,0):0;
    if(!result)state()->concealed=!visible;unlock();MyOSUiWake();return error(result);
}
