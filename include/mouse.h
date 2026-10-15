#ifndef MYOS_MOUSE_H
#define MYOS_MOUSE_H
#include "os.h"

struct mouse_event {
    int16_t x,y;
    int16_t dx,dy;
    uint8_t buttons,changed;
};

bool mouse_init(uint16_t width,uint16_t height);
bool mouse_ready(void);
void mouse_usb_attach(void);
void mouse_usb_report(uint8_t,int,int);
bool mouse_absolute_enable(void);
void mouse_relative_enable(void);
void mouse_irq(void);
bool mouse_get(struct mouse_event *event);
void mouse_position(int *x,int *y,uint8_t *buttons);
void mouse_flush(void);
uint32_t mouse_dropped(void);
#endif
