#ifndef MYOS_GRAPHICS_H
#define MYOS_GRAPHICS_H
#include "os.h"

struct gfx_mode {
    uint32_t width,height,pitch;
    volatile uint32_t *pixels;
};
struct gfx_surface {
    uint32_t width,height,stride;
    uint32_t *pixels;
};

/* Probe is safe before memory_init. A bad VBE structure is rejected. */
bool gfx_probe(struct gfx_mode *mode);
bool gfx_enabled(void);
void gfx_init(void);
struct gfx_surface *gfx_surface_create(uint32_t width,uint32_t height);
void gfx_surface_destroy(struct gfx_surface *surface);
void gfx_clear(struct gfx_surface *surface,uint32_t color);
void gfx_pixel(struct gfx_surface *surface,int x,int y,uint32_t color);
void gfx_rect(struct gfx_surface *surface,int x,int y,int width,int height,uint32_t color);
void gfx_text(struct gfx_surface *surface,int x,int y,const char *text,uint32_t color);
bool gfx_present(const struct gfx_surface *surface,int x,int y);
void gfx_restore_region(const struct gfx_surface *scene,int x,int y,int width,int height);
void gfx_cursor_draw(int x,int y,uint8_t buttons);
void gfx_test(void);
void gfx_demo(void);
#endif
