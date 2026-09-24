#include "widgets.h"

bool ui_hit(struct ui_rect r,int x,int y)
{
    return r.width>0 && r.height>0 && x>=r.x && y>=r.y &&
           x-r.x<r.width && y-r.y<r.height;
}
void ui_label(struct gfx_surface *scene,int x,int y,const char *text,uint32_t color)
{
    gfx_text(scene,x,y,text,color);
}
void ui_button(struct gfx_surface *scene,struct ui_rect r,const char *text,
               uint32_t fill,uint32_t ink)
{
    gfx_rect(scene,r.x,r.y,r.width,r.height,0x97a9bd);
    gfx_rect(scene,r.x+2,r.y+2,r.width-4,r.height-4,fill);
    gfx_text(scene,r.x+10,r.y+(r.height-16)/2,text,ink);
}
void ui_field(struct gfx_surface *scene,struct ui_rect r,const char *text,
              uint32_t fill,uint32_t ink,uint32_t border)
{
    gfx_rect(scene,r.x,r.y,r.width,r.height,border);
    gfx_rect(scene,r.x+2,r.y+2,r.width-4,r.height-4,fill);
    gfx_text(scene,r.x+8,r.y+(r.height-16)/2,text,ink);
}
