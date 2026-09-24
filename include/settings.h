#ifndef MYOS_SETTINGS_H
#define MYOS_SETTINGS_H
#include "graphics.h"

void settings_init(void);
void settings_begin(void);
void settings_cancel(void);
void settings_draw(struct gfx_surface *scene,int x,int y);
void settings_click(int x,int y);
bool settings_key(int key);
bool settings_editing(void);
uint32_t settings_background_color(void);
uint32_t settings_bar_color(void);
uint32_t settings_text_color(void);
uint32_t settings_accent_color(void);
void settings_print(void);

#endif
