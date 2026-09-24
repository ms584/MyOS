#include "desktop.h"
#include "calculator.h"
#include "graphics.h"
#include "mouse.h"
#include "settings.h"
#include "snake.h"
#include "widgets.h"

#define WINDOW_LIMIT 6
#define WINDOW_WIDTH 320
#define WINDOW_HEIGHT 210
#define TITLE_HEIGHT 32

struct window {
    int x,y,id;
};
struct desktop {
    struct gfx_surface *scene;
    struct window *windows[WINDOW_LIMIT];
    int order[WINDOW_LIMIT],count,active,dragging,drag_x,drag_y;
    int cursor_x,cursor_y;
    uint8_t buttons;
};

static const char *titles[WINDOW_LIMIT]={"Demo A","Demo B","Demo C","Settings","Calculator","Snake"};
static const uint32_t bodies[WINDOW_LIMIT]={0x20324b,0x3a2b43,0x2a3c39,0x20324b,0x20324b,0x20324b};

static void marker(const char *name,int id)
{
    while(*name)serial_put(*name++);
    if(id>=0){serial_put(' ');serial_put((char)('1'+id));}
    serial_put('\n');
}
static int clamp(int value,int low,int high)
{
    if(value<low)return low;
    if(value>high)return high;
    return value;
}
static bool inside(int x,int y,int left,int top,int width,int height)
{
    return x>=left && y>=top && x<left+width && y<top+height;
}
/* The three finished apps stay accessible even when demo windows exist. */
static struct ui_rect app_button(const struct desktop *d,int id)
{
    int left=(int)d->scene->width-298;
    if(id==3)return (struct ui_rect){left,12,86,30};
    if(id==4)return (struct ui_rect){left+94,12,100,30};
    return (struct ui_rect){left+202,12,78,30};
}
static void top(struct desktop *d,int id)
{
    int place=-1;
    for(int i=0;i<d->count;++i)if(d->order[i]==id){place=i;break;}
    check(place>=0,"desktop z-order");
    for(int i=place;i<d->count-1;++i)d->order[i]=d->order[i+1];
    d->order[d->count-1]=id;
    d->active=id;
}
static bool open_window(struct desktop *d,int id)
{
    if(id<0 || id>=WINDOW_LIMIT)return false;
    if(d->windows[id]){top(d,id);marker("WINDOW FOCUS",id);return true;}
    struct window *w=kmalloc(sizeof(*w));
    if(!w){marker("WINDOW OOM",id);return false;}
    int width=d->scene->width,height=d->scene->height;
    *w=(struct window){clamp(230+id*50,0,width-WINDOW_WIDTH),
                       clamp(130+id*50,54,height-WINDOW_HEIGHT-36),id};
    d->windows[id]=w;
    d->order[d->count++]=id;
    d->active=id;
    if(id==3)settings_begin();
    if(id==4)calculator_begin();
    if(id==5)snake_begin();
    marker("WINDOW OPEN",id);
    return true;
}
static void close_window(struct desktop *d,int id)
{
    if(id<0 || id>=WINDOW_LIMIT || !d->windows[id])return;
    if(id==3)settings_cancel();
    kfree(d->windows[id]);d->windows[id]=0;
    int place=-1;
    for(int i=0;i<d->count;++i)if(d->order[i]==id){place=i;break;}
    check(place>=0,"desktop close z-order");
    for(int i=place;i<d->count-1;++i)d->order[i]=d->order[i+1];
    --d->count;
    d->active=d->count?d->order[d->count-1]:-1;
    d->dragging=-1;
    marker("WINDOW CLOSE",id);
}
static void draw_window(struct desktop *d,const struct window *w)
{
    struct gfx_surface *s=d->scene;
    int x=w->x,y=w->y,id=w->id;
    gfx_rect(s,x+5,y+6,WINDOW_WIDTH,WINDOW_HEIGHT,0x0a101d);
    gfx_rect(s,x,y,WINDOW_WIDTH,WINDOW_HEIGHT,0x92aac4);
    gfx_rect(s,x+2,y+2,WINDOW_WIDTH-4,WINDOW_HEIGHT-4,bodies[id]);
    gfx_rect(s,x+2,y+2,WINDOW_WIDTH-4,TITLE_HEIGHT-2,
             d->active==id?settings_accent_color():0x39475d);
    gfx_text(s,x+12,y+8,titles[id],0xffffff);
    gfx_rect(s,x+WINDOW_WIDTH-29,y+5,24,22,0xd65c69);
    gfx_text(s,x+WINDOW_WIDTH-21,y+7,"X",0xffffff);
    if(id==3){settings_draw(s,x,y);return;}
    if(id==4){calculator_draw(s,x,y);return;}
    if(id==5){snake_draw(s,x,y);return;}
    gfx_text(s,x+18,y+53,"Phase 16 window placeholder",0xe1ecfa);
    gfx_text(s,x+18,y+82,"Drag the title. Click X to close.",0xb8cee7);
    gfx_rect(s,x+18,y+118,WINDOW_WIDTH-36,54,
             id==0?0x277c91:id==1?0x83566f:0x52816d);
    gfx_text(s,x+30,y+137,"Desktop windowing test",0xffffff);
}
static void redraw(struct desktop *d)
{
    struct gfx_surface *s=d->scene;
    gfx_clear(s,settings_background_color());
    gfx_rect(s,0,0,(int)s->width,54,settings_bar_color());
    gfx_text(s,24,18,"MyOS Desktop 1.0",settings_text_color());
    gfx_text(s,(int)s->width-342,20,"Apps",settings_text_color());
    for(int id=3;id<WINDOW_LIMIT;++id)
        ui_button(s,app_button(d,id),titles[id],
                  d->windows[id]?0x2c6672:0x2f4768,0xffffff);
    gfx_text(s,24,64,"Launcher",settings_text_color());
    for(int id=0;id<WINDOW_LIMIT;++id){
        int y=86+id*56;
        gfx_rect(s,24,y,160,44,d->windows[id]?0x2c6672:0x2f4768);
        gfx_text(s,42,y+13,titles[id],0xffffff);
    }
    for(int i=0;i<d->count;++i)draw_window(d,d->windows[d->order[i]]);
    gfx_rect(s,0,(int)s->height-36,(int)s->width,36,0x192842);
    gfx_text(s,24,(int)s->height-26,"Apps above | 4-6 keys | Drag | X close | Q exit",0xffffff);
    check(gfx_present(s,0,0),"desktop present");
    gfx_cursor_draw(d->cursor_x,d->cursor_y,d->buttons);
    marker("DESKTOP FRAME",-1);
}
static int hit_window(const struct desktop *d,int x,int y)
{
    for(int i=d->count-1;i>=0;--i){
        int id=d->order[i];const struct window *w=d->windows[id];
        if(inside(x,y,w->x,w->y,WINDOW_WIDTH,WINDOW_HEIGHT))return id;
    }
    return -1;
}
static bool click(struct desktop *d,int x,int y)
{
    for(int app=3;app<WINDOW_LIMIT;++app)
        if(ui_hit(app_button(d,app),x,y))return open_window(d,app);
    int id=hit_window(d,x,y);
    if(id>=0){
        struct window *w=d->windows[id];
        if(inside(x,y,w->x+WINDOW_WIDTH-29,w->y+5,24,22)){
            close_window(d,id);return true;
        }
        top(d,id);marker("WINDOW FOCUS",id);
        if(inside(x,y,w->x,w->y,WINDOW_WIDTH,TITLE_HEIGHT)){
            d->dragging=id;d->drag_x=x-w->x;d->drag_y=y-w->y;
        }else if(id==3)settings_click(x-w->x,y-w->y);
        else if(id==4)calculator_click(x-w->x,y-w->y);
        else if(id==5)snake_click(x-w->x,y-w->y);
        return true;
    }
    for(int i=0;i<WINDOW_LIMIT;++i)
        if(inside(x,y,24,86+i*56,160,44))return open_window(d,i);
    d->active=-1;return true;
}
static bool handle_mouse(struct desktop *d,struct mouse_event event)
{
    int old_x=d->cursor_x,old_y=d->cursor_y;
    d->cursor_x=event.x;d->cursor_y=event.y;d->buttons=event.buttons;
    bool dirty=false;
    if((event.changed&1) && (event.buttons&1))dirty=click(d,event.x,event.y);
    if((event.changed&1) && !(event.buttons&1))d->dragging=-1;
    if((event.buttons&1) && d->dragging>=0){
        int id=d->dragging;struct window *w=d->windows[id];
        if(w){
            int x=clamp(event.x-d->drag_x,0,(int)d->scene->width-WINDOW_WIDTH);
            int y=clamp(event.y-d->drag_y,54,(int)d->scene->height-WINDOW_HEIGHT-36);
            if(x!=w->x || y!=w->y){w->x=x;w->y=y;dirty=true;marker("WINDOW DRAG",id);}
        }
    }
    if(dirty)redraw(d);
    else{
        gfx_restore_region(d->scene,old_x,old_y,12,16);
        gfx_cursor_draw(d->cursor_x,d->cursor_y,d->buttons);
    }
    return dirty;
}
void desktop_run(void)
{
    struct gfx_mode mode;
    if(!gfx_enabled() || !gfx_probe(&mode) || mode.width<640 || mode.height<480){
        print("Desktop requires framebuffer >= 640x480\n");return;
    }
    struct desktop d={0};d.active=d.dragging=-1;
    d.scene=gfx_surface_create(mode.width,mode.height);
    if(!d.scene){print("Desktop surface allocation failed\n");return;}
    settings_cancel();
    mouse_flush();mouse_position(&d.cursor_x,&d.cursor_y,&d.buttons);
    redraw(&d);marker("DESKTOP READY",-1);
    for(;;){
        /* PIT ticks advance Snake even while another window has focus. */
        if(d.windows[5] && snake_update(ticks))redraw(&d);
        struct mouse_event event;
        uint64_t flags=irq_save();
        bool has_mouse=mouse_get(&event);
        int key=keyboard_get();
        if(!has_mouse && key<0){__asm__ volatile("sti; hlt":::"memory");continue;}
        irq_restore(flags);
        if(has_mouse)handle_mouse(&d,event);
        if(d.active==3 && settings_editing() && settings_key(key)){redraw(&d);continue;}
        if(key=='q' || key=='Q' || key==27)break;
        if(d.active==4 && calculator_key(key)){redraw(&d);continue;}
        if(d.active==5 && snake_key(key)){redraw(&d);continue;}
        if(key>='1' && key<='6' && open_window(&d,key-'1'))redraw(&d);
    }
    settings_cancel();
    for(int id=0;id<WINDOW_LIMIT;++id)if(d.windows[id])kfree(d.windows[id]);
    gfx_surface_destroy(d.scene);
    console_clear();print("Desktop closed\n");marker("DESKTOP CLOSED",-1);
}
