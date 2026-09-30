#include "native-ui.h"
#include "graphics.h"
#include "process.h"
#include "mouse.h"

#define UI_EVENTS 128u
static struct process *owner;
static struct gfx_surface *saved,*foreground;
static bool visible;
static uint64_t generation,next_generation,notify_address;
static unsigned head,tail,count;
static uint32_t dropped,modifiers;
static struct native_ui_event *events;

bool native_ui_owns(struct process *process){return process&&owner==process;}
uint32_t native_ui_modifiers(void){return modifiers;}
void native_ui_set_modifiers(uint32_t value){modifiers=value;}
static bool matches(struct process *process,uint64_t handle)
{return native_ui_owns(process)&&handle&&handle==generation;}
int native_ui_query(struct process *process,uint64_t handle,struct native_ui_info *info)
{
    if(handle&&!matches(process,handle))return -9;
    struct gfx_mode mode;if(!gfx_enabled()||!gfx_probe(&mode))return -95;
    *info=(struct native_ui_info){NATIVE_UI_ABI,mode.width,mode.height,
                                handle?dropped:0,handle?generation:0};return 0;
}
int64_t native_ui_acquire(struct process *process,uint64_t notification,struct native_ui_info *info)
{
    int rc=native_ui_query(process,0,info);if(rc)return rc;
    if(owner)return -16;
    if(next_generation==INT64_MAX)return -28;
    struct gfx_surface *snapshot=gfx_surface_create(info->width,info->height);
    if(!snapshot)return -12;
    struct gfx_surface *frame=gfx_surface_create(info->width,info->height);
    if(!frame){gfx_surface_destroy(snapshot);return -12;}
    struct native_ui_event *queue=kmalloc(UI_EVENTS*sizeof(*queue));
    if(!queue){gfx_surface_destroy(frame);gfx_surface_destroy(snapshot);return -12;}
    if(!gfx_capture(snapshot)){kfree(queue);gfx_surface_destroy(frame);gfx_surface_destroy(snapshot);return -95;}
    saved=snapshot;foreground=frame;events=queue;visible=true;
    owner=process;generation=++next_generation;notify_address=notification;
    head=tail=count=0;dropped=0;info->handle=generation;return (int64_t)generation;
}
static void release(void)
{
    /* Wake borrowed waiters before invalidating ownership; no IRQ may retain
     * this process or a notification VA after its pages are reclaimed. */
    if(owner&&notify_address)process_notify(owner,notify_address);
    owner=0;generation=notify_address=0;head=tail=count=0;dropped=0;
    if(saved){gfx_present(saved,0,0);gfx_surface_destroy(saved);saved=0;}
    if(foreground){gfx_surface_destroy(foreground);foreground=0;}visible=false;
    if(events){kfree(events);events=0;}
    /* Focused app input must not become commands/clicks after it closes. */
    while(keyboard_get()>=0){}mouse_flush();
}
int native_ui_release(struct process *process,uint64_t handle)
{
    if(!matches(process,handle))return -9;
    release();return 0;
}
void native_ui_release_owner(struct process *process)
{if(native_ui_owns(process))release();}
void native_ui_emit(const struct native_ui_event *event)
{
    if(!owner||!visible)return;
    if(count==UI_EVENTS){tail=(tail+1)%UI_EVENTS;--count;if(dropped!=UINT32_MAX)++dropped;}
    events[head]=*event;events[head].tick=ticks;
    head=(head+1)%UI_EVENTS;++count;
    if(notify_address)process_notify(owner,notify_address);
}
int native_ui_poll(struct process *process,uint64_t handle,struct native_ui_event *event)
{
    if(!matches(process,handle))return -9;
    if(!count)return 0;
    *event=events[tail];tail=(tail+1)%UI_EVENTS;--count;return 1;
}
int native_ui_visible(struct process *process,uint64_t handle,bool show)
{
    if(!matches(process,handle))return -9;
    if(visible==show)return 0;
    if(!show&&!gfx_capture(foreground))return -95;
    if(!gfx_present(show?foreground:saved,0,0))return -95;
    visible=show;head=tail=count=0;process_notify(owner,notify_address);
    while(keyboard_get()>=0){}mouse_flush();return 0;
}
int native_ui_draw(struct process *process,uint64_t handle,const struct gfx_surface *row,int x,int y)
{
    if(!matches(process,handle))return -9;
    if(visible)return gfx_present(row,x,y)?0:-95;
    if(!row||!row->pixels||!row->width||row->height!=1||x<0||y<0||
       (uint64_t)x+row->width>foreground->width||(uint32_t)y>=foreground->height)return -22;
    for(unsigned i=0;i<row->width;++i)
        foreground->pixels[(size_t)y*foreground->stride+(unsigned)x+i]=row->pixels[i]&0xffffff;
    return 0;
}
