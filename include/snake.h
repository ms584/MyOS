#ifndef MYOS_SNAKE_H
#define MYOS_SNAKE_H
#include "graphics.h"

void snake_init(void);
void snake_begin(void);
bool snake_update(uint64_t now);
bool snake_key(int key);
void snake_click(int x,int y);
void snake_draw(struct gfx_surface *scene,int x,int y);

#endif
