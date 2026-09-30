/* Host state-machine test supplements, never replaces, native VRAM/input tests. */
#include "native-ui.h"
#include "graphics.h"
#include "process.h"
#include <assert.h>
#include <stdlib.h>
volatile uint64_t ticks;
static unsigned allocated,presented,flushed,notified;
static bool enabled=true,allocation_failure,capture_failure,present_failure;
static uint32_t last_pixel;
static unsigned creation_count,fail_at;
static unsigned queues;static bool queue_failure;
void *kmalloc(size_t size)
{assert(size==128*48);if(queue_failure)return 0;void *p=malloc(size);assert(p);++queues;return p;}
void kfree(void *p){assert(p&&queues);--queues;free(p);}
bool gfx_enabled(void){return enabled;}
bool gfx_probe(struct gfx_mode *mode)
{*mode=(struct gfx_mode){800,600,3200,0};return true;}
struct gfx_surface *gfx_surface_create(uint32_t w,uint32_t h)
{
    if(allocation_failure||++creation_count==fail_at)return 0;
    struct gfx_surface *surface=malloc(sizeof(*surface));assert(surface);
    uint32_t *pixels=calloc((size_t)w*h,sizeof(*pixels));assert(pixels);
    *surface=(struct gfx_surface){w,h,w,pixels};++allocated;return surface;
}
void gfx_surface_destroy(struct gfx_surface *surface)
{assert(surface&&allocated);--allocated;free(surface->pixels);free(surface);}
bool gfx_capture(struct gfx_surface *surface)
{
    assert(surface);if(capture_failure)return false;
    for(size_t i=0;i<(size_t)surface->width*surface->height;++i)surface->pixels[i]=0x445566;
    return true;
}
bool gfx_present(const struct gfx_surface *surface,int x,int y)
{assert(surface&&!x&&!y);if(present_failure)return false;
 last_pixel=surface->pixels[4*surface->stride+3];++presented;return true;}
int keyboard_get(void){return -1;}
void mouse_flush(void){++flushed;}
bool process_notify(struct process *process,uint64_t address)
{assert(process&&address);++*(uint64_t *)(uintptr_t)address;++notified;return true;}
int main(void)
{
    struct process process={0},other={0};struct native_ui_info info;
    uint64_t sequence=0;
    enabled=false;assert(native_ui_acquire(&process,(uintptr_t)&sequence,&info)==-95);
    enabled=true;allocation_failure=true;
    assert(native_ui_acquire(&process,(uintptr_t)&sequence,&info)==-12&&!allocated);
    allocation_failure=false;capture_failure=true;
    assert(native_ui_acquire(&process,(uintptr_t)&sequence,&info)==-95&&!allocated&&!queues);
    capture_failure=false;
    fail_at=creation_count+2;
    assert(native_ui_acquire(&process,(uintptr_t)&sequence,&info)==-12&&!allocated);
    fail_at=0;
    queue_failure=true;
    assert(native_ui_acquire(&process,(uintptr_t)&sequence,&info)==-12&&!allocated&&!queues);
    queue_failure=false;
    int64_t handle=native_ui_acquire(&process,(uintptr_t)&sequence,&info);
    assert(handle>0&&info.handle==(uint64_t)handle&&allocated==2);
    assert(native_ui_owns(&process)&&!native_ui_owns(&other));
    assert(native_ui_acquire(&other,(uintptr_t)&sequence,&info)==-16);
    struct native_ui_event event={0},out;
    assert(native_ui_query(&other,(uint64_t)handle,&info)==-9);
    assert(native_ui_poll(&other,(uint64_t)handle,&out)==-9);
    assert(native_ui_release(&other,(uint64_t)handle)==-9);
    native_ui_release_owner(&other);assert(allocated==2);
    for(unsigned i=0;i<140;++i){event.scan=i;ticks=i+1;native_ui_emit(&event);}
    assert(sequence==140&&notified==140);
    assert(!native_ui_query(&process,(uint64_t)handle,&info)&&info.dropped==12);
    assert(!native_ui_query(&other,0,&info)&&!info.handle&&!info.dropped);
    for(unsigned i=12;i<140;++i){
        assert(native_ui_poll(&process,(uint64_t)handle,&out)==1);
        assert(out.scan==i&&out.tick==i+1);
    }
    assert(!native_ui_poll(&process,(uint64_t)handle,&out));
    assert(native_ui_visible(&other,(uint64_t)handle,false)==-9);
    capture_failure=true;assert(native_ui_visible(&process,(uint64_t)handle,false)==-95&&sequence==140);
    capture_failure=false;
    assert(!native_ui_visible(&process,(uint64_t)handle,false));
    assert(sequence==141);
    native_ui_emit(&event);assert(sequence==141&&!native_ui_poll(&process,(uint64_t)handle,&out));
    uint32_t value=0xab123456;struct gfx_surface row={1,1,1,&value};
    assert(!native_ui_draw(&process,(uint64_t)handle,&row,3,4));
    assert(native_ui_draw(&other,(uint64_t)handle,&row,3,4)==-9);
    assert(native_ui_draw(&process,(uint64_t)handle,&row,800,4)==-22);
    present_failure=true;assert(native_ui_visible(&process,(uint64_t)handle,true)==-95&&sequence==141);
    native_ui_emit(&event);assert(!native_ui_poll(&process,(uint64_t)handle,&out));present_failure=false;
    assert(!native_ui_visible(&process,(uint64_t)handle,true)&&sequence==142);
    assert(last_pixel==0x123456);
    assert(!native_ui_visible(&process,(uint64_t)handle,true)&&sequence==142);
    assert(!native_ui_release(&process,(uint64_t)handle));
    assert(last_pixel==0x445566);
    assert(!allocated&&!queues&&presented==3&&flushed==3&&sequence==143);
    unsigned count=notified;native_ui_emit(&event);assert(notified==count);
    int64_t replacement=native_ui_acquire(&process,(uintptr_t)&sequence,&info);
    assert(replacement>handle&&!info.dropped);
    assert(native_ui_query(&process,(uint64_t)handle,&info)==-9);
    assert(native_ui_poll(&process,(uint64_t)handle,&out)==-9);
    assert(native_ui_release(&process,(uint64_t)handle)==-9);
    native_ui_release_owner(&process);native_ui_release_owner(&process);
    assert(!allocated&&!queues&&presented==4&&flushed==4);
    native_ui_set_modifiers(NATIVE_UI_CONTROL|NATIVE_UI_META);
    assert(native_ui_modifiers()==(NATIVE_UI_CONTROL|NATIVE_UI_META));return 0;
}
