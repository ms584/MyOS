#include "graphics.h"
#include "mouse.h"

static struct gfx_mode screen;
static bool enabled;

bool gfx_probe(struct gfx_mode *mode)
{
    if(!mode || !*(volatile uint8_t *)0x7004)return false;
    const uint8_t *info=(const uint8_t *)0xb200;
    uint32_t width=*(const uint16_t *)(info+18);
    uint32_t height=*(const uint16_t *)(info+20);
    uint32_t pitch_bytes=*(const uint16_t *)(info+16);
    uint32_t physical=*(const uint32_t *)(info+40);
    uint8_t bpp=info[25];
    if(bpp!=32 || width<320 || width>1280 || height<200 || height>800 ||
       width/8>160 || height/16>50 || pitch_bytes%4 ||
       pitch_bytes<width*4 || pitch_bytes>16384 || physical<0x100000)
        return false;
    uint64_t last=(uint64_t)physical+(uint64_t)(height-1)*pitch_bytes+
                  (uint64_t)width*4;
    if(last>(1ull<<32))return false; /* Bootstrap identity map covers 4 GiB. */
    *mode=(struct gfx_mode){width,height,pitch_bytes/4,(volatile uint32_t *)(uintptr_t)physical};
    return true;
}
void gfx_init(void)
{
    enabled=gfx_probe(&screen);
}
bool gfx_enabled(void){return enabled;}

struct gfx_surface *gfx_surface_create(uint32_t width,uint32_t height)
{
    if(!width || !height || width>1280 || height>800)return 0;
    size_t count=(size_t)width*height;
    if(count>(SIZE_MAX-sizeof(struct gfx_surface))/sizeof(uint32_t))return 0;
    size_t bytes=sizeof(struct gfx_surface)+count*sizeof(uint32_t);
    struct gfx_surface *surface=kmalloc(bytes);
    if(!surface)return 0;
    *surface=(struct gfx_surface){width,height,width,(uint32_t *)(surface+1)};
    memset(surface->pixels,0,count*sizeof(uint32_t));
    return surface;
}
void gfx_surface_destroy(struct gfx_surface *surface){kfree(surface);}
void gfx_clear(struct gfx_surface *surface,uint32_t color)
{
    if(!surface)return;
    color&=0xffffff;
    for(uint32_t y=0;y<surface->height;++y)
        for(uint32_t x=0;x<surface->width;++x)
            surface->pixels[(size_t)y*surface->stride+x]=color;
}
void gfx_pixel(struct gfx_surface *surface,int x,int y,uint32_t color)
{
    if(!surface || x<0 || y<0 || (uint32_t)x>=surface->width ||
       (uint32_t)y>=surface->height)return;
    surface->pixels[(size_t)y*surface->stride+(uint32_t)x]=color&0xffffff;
}
void gfx_rect(struct gfx_surface *surface,int x,int y,int width,int height,uint32_t color)
{
    if(!surface || width<=0 || height<=0)return;
    int64_t left=x,top=y,right=left+width,bottom=top+height;
    if(left<0)left=0;if(top<0)top=0;
    if(right>surface->width)right=surface->width;
    if(bottom>surface->height)bottom=surface->height;
    if(left>=right || top>=bottom)return;
    color&=0xffffff;
    for(int64_t row=top;row<bottom;++row)
        for(int64_t col=left;col<right;++col)
            surface->pixels[(size_t)row*surface->stride+(size_t)col]=color;
}
void gfx_text(struct gfx_surface *surface,int x,int y,const char *text,uint32_t color)
{
    if(!surface || !text)return;
    int64_t cursor=x,row=y;color&=0xffffff;
    for(;*text;++text){
        if(*text=='\n'){cursor=x;row+=16;continue;}
        const uint8_t *glyph=(const uint8_t *)0xa000+(uint8_t)*text*16;
        for(int dy=0;dy<16;++dy){
            int64_t py=row+dy;if(py<0 || py>=surface->height)continue;
            uint8_t bits=glyph[dy];
            for(int dx=0;dx<8;++dx){
                int64_t px=cursor+dx;
                if((bits&(0x80u>>dx)) && px>=0 && px<surface->width)
                    surface->pixels[(size_t)py*surface->stride+(size_t)px]=color;
            }
        }
        cursor+=8;
        if(cursor>(int64_t)surface->width+8 && x>=0)break;
    }
}
bool gfx_present(const struct gfx_surface *surface,int x,int y)
{
    if(!enabled || !surface || !surface->pixels)return false;
    int64_t sx=0,sy=0,dx=x,dy=y;
    if(dx<0){sx=-dx;dx=0;}if(dy<0){sy=-dy;dy=0;}
    int64_t width=(int64_t)surface->width-sx;
    int64_t height=(int64_t)surface->height-sy;
    if(width>(int64_t)screen.width-dx)width=(int64_t)screen.width-dx;
    if(height>(int64_t)screen.height-dy)height=(int64_t)screen.height-dy;
    if(width<=0 || height<=0)return true;
    uint64_t flags=irq_save();
    for(int64_t row=0;row<height;++row){
        const uint32_t *source=surface->pixels+(size_t)(sy+row)*surface->stride+(size_t)sx;
        volatile uint32_t *target=screen.pixels+(size_t)(dy+row)*screen.pitch+(size_t)dx;
        for(int64_t col=0;col<width;++col)target[col]=source[col]&0xffffff;
    }
    irq_restore(flags);return true;
}
/* Restore a small changed region from the off-screen scene. */
void gfx_restore_region(const struct gfx_surface *scene,int x,int y,int width,int height)
{
    if(!enabled || !scene)return;
    int left=x<0?0:x,top=y<0?0:y;
    int right=x+width>(int)screen.width?(int)screen.width:x+width;
    int bottom=y+height>(int)screen.height?(int)screen.height:y+height;
    if(right>(int)scene->width)right=scene->width;
    if(bottom>(int)scene->height)bottom=scene->height;
    if(left>=right || top>=bottom)return;
    uint64_t flags=irq_save();
    for(int row=top;row<bottom;++row)
        for(int col=left;col<right;++col)
            screen.pixels[(size_t)row*screen.pitch+col]=
                scene->pixels[(size_t)row*scene->stride+col];
    irq_restore(flags);
}
void gfx_cursor_draw(int x,int y,uint8_t buttons)
{
    if(!enabled)return;
    uint32_t fill=(buttons&1)?0xf5b642:0xffffff;
    uint64_t flags=irq_save();
    for(int row=0;row<16;++row){
        int width=row<9?2+row/2:row<12?5:3;
        for(int col=0;col<width;++col){
            int px=x+col,py=y+row;
            if(px<0 || py<0 || px>=(int)screen.width || py>=(int)screen.height)continue;
            screen.pixels[(size_t)py*screen.pitch+px]=
                col==0 || col==width-1 || row==0?0x101010:fill;
        }
    }
    irq_restore(flags);
}
static void demo_buttons(struct gfx_surface *scene,uint8_t buttons)
{
    gfx_rect(scene,56,392,480,20,0x101a2e);
    if(buttons&1)gfx_text(scene,56,392,"Left button down",0xf5b642);
    else if(buttons&2)gfx_text(scene,56,392,"Right button down",0xf5b642);
    else if(buttons&4)gfx_text(scene,56,392,"Middle button down",0xf5b642);
    else gfx_text(scene,56,392,"Mouse buttons released",0xbbeeff);
}
static void marker(const char *text)
{
    while(*text)serial_put(*text++);
    serial_put('\n');
}
void gfx_test(void)
{
    struct gfx_surface *s=gfx_surface_create(16,16);
    check(s!=0,"graphics small surface allocation");
    gfx_clear(s,0x102030);
    gfx_rect(s,-4,-4,8,8,0xaabbcc);
    check(s->pixels[0]==0xaabbcc && s->pixels[3]==0xaabbcc &&
          s->pixels[4]==0x102030 && s->pixels[4*s->stride]==0x102030,
          "graphics clipped rectangle");
    gfx_pixel(s,15,15,0xff0011);
    gfx_pixel(s,-1,3,0xffffff);
    check(s->pixels[15*16+15]==0xff0011 && s->pixels[3*16]==0xaabbcc,
          "graphics clipped pixel");
    gfx_text(s,14,14,"A",0xffffff);
    gfx_rect(s,0,0,16,16,0x445566);
    check(s->pixels[0]==0x445566,"graphics redraw");
    gfx_surface_destroy(s);
    print("PASS: graphics surface, clipping, font and redraw\n");
}
static void demo_frame(struct gfx_surface *s,bool alternate)
{
    gfx_clear(s,0x101a2e);
    gfx_rect(s,0,0,(int)s->width,96,alternate?0x623b72:0x233b62);
    gfx_rect(s,0,0,8,(int)s->height,0x23c4cf);
    gfx_text(s,32,28,"MyOS Graphics Foundation",0xffffff);
    gfx_text(s,32,58,"Phase 15  |  PS/2 mouse + redraw",0xbbeeff);
    gfx_rect(s,32,132,(int)s->width-64,176,0x1c2b43);
    gfx_text(s,56,154,"Clipped shapes and bitmap font",0xffffff);
    gfx_rect(s,56,196,120,64,0x23c4cf);
    gfx_rect(s,192,196,120,64,0xf5b642);
    gfx_rect(s,-20,340,64,48,0xe85d75);
    gfx_text(s,56,344,"Move/click mouse. R redraws; another key exits.",0xffffff);
    demo_buttons(s,0);
    gfx_rect(s,(int)s->width-40,(int)s->height-30,90,80,0x23c4cf);
}
void gfx_demo(void)
{
    if(!enabled){print("Graphics requires framebuffer mode\n");return;}
    struct gfx_surface *s=gfx_surface_create(screen.width,screen.height);
    if(!s){print("Graphics surface allocation failed\n");return;}
    bool alternate=false;
    int cursor_x=0,cursor_y=0;uint8_t buttons=0;
    mouse_flush();mouse_position(&cursor_x,&cursor_y,&buttons);
    demo_frame(s,alternate);demo_buttons(s,buttons);
    check(gfx_present(s,0,0),"graphics present");
    if(mouse_ready())gfx_cursor_draw(cursor_x,cursor_y,buttons);
    marker("GRAPHICS DEMO READY");
    for(;;){
        struct mouse_event event;
        uint64_t flags=irq_save();
        bool moved=mouse_get(&event);
        int key=keyboard_get();
        if(!moved && key<0){__asm__ volatile("sti; hlt":::"memory");continue;}
        irq_restore(flags);
        if(moved){
            gfx_restore_region(s,cursor_x,cursor_y,12,16);
            cursor_x=event.x;cursor_y=event.y;
            if(event.changed){
                buttons=event.buttons;demo_buttons(s,buttons);
                gfx_restore_region(s,56,392,480,20); /* Update only the button legend. */
                if((event.changed&1) && (buttons&1))marker("MOUSE LEFT DOWN");
            }
            gfx_cursor_draw(cursor_x,cursor_y,buttons);
            marker("MOUSE EVENT");
        }
        if(key>=0){
            if(key!='r' && key!='R')break;
            alternate=!alternate;
            demo_frame(s,alternate);demo_buttons(s,buttons);
            check(gfx_present(s,0,0),"graphics present");
            if(mouse_ready())gfx_cursor_draw(cursor_x,cursor_y,buttons);
            marker("GRAPHICS REDRAW");
        }
    }
    gfx_surface_destroy(s);
    console_clear();
    print("Graphics demo closed\n");
}
