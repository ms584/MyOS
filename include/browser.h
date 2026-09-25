#ifndef MYOS_BROWSER_H
#define MYOS_BROWSER_H
#include "graphics.h"
#define BROWSER_WIDTH 600
#define BROWSER_HEIGHT 420
void browser_begin(void);
void browser_close(void);
void browser_draw(struct gfx_surface *scene,int x,int y);
bool browser_click(int x,int y);
bool browser_key(int key);
#endif
