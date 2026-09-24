#ifndef MYOS_WIDGETS_H
#define MYOS_WIDGETS_H
#include "graphics.h"

struct ui_rect {int x,y,width,height;};
bool ui_hit(struct ui_rect rect,int x,int y);
void ui_label(struct gfx_surface *scene,int x,int y,const char *text,uint32_t color);
void ui_button(struct gfx_surface *scene,struct ui_rect rect,const char *text,
               uint32_t fill,uint32_t ink);
void ui_field(struct gfx_surface *scene,struct ui_rect rect,const char *text,
              uint32_t fill,uint32_t ink,uint32_t border);

#endif
