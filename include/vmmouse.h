#ifndef MYOS_VMMOUSE_H
#define MYOS_VMMOUSE_H
#include "os.h"
/* Optional QEMU/VMware virtual device. PS/2 remains the default/fallback. */
struct vmmouse_event { uint16_t x,y;uint8_t buttons; };
bool vmmouse_enable(void);
void vmmouse_disable(void);
bool vmmouse_enabled(void);
int vmmouse_read(struct vmmouse_event *event);
#endif
