#ifndef MYOS_CALCULATOR_H
#define MYOS_CALCULATOR_H
#include "graphics.h"

void calculator_begin(void);
void calculator_draw(struct gfx_surface *scene,int x,int y);
void calculator_click(int x,int y);
bool calculator_key(int key);

#endif
