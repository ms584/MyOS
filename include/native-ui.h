#ifndef MYOS_NATIVE_UI_H
#define MYOS_NATIVE_UI_H
#include <stdint.h>
#include <stdbool.h>
/* Private MyOS presentation/input ABI; not Linux or SbInputData. */
#define NATIVE_UI_ABI 1u
#define NATIVE_UI_RGBA8 1u
#define NATIVE_UI_XRGB8888 2u
#define NATIVE_UI_KEY_DOWN 1u
#define NATIVE_UI_KEY_UP 2u
#define NATIVE_UI_POINTER 3u
#define NATIVE_UI_SHIFT 1u
#define NATIVE_UI_CONTROL 2u
#define NATIVE_UI_ALT 4u
#define NATIVE_UI_META 8u
#define NATIVE_UI_CAPS 16u
struct native_ui_info { uint32_t version,width,height,dropped;uint64_t handle; };
struct native_ui_present {
    uint64_t pixels;
    uint32_t width,height,stride,format;
    int32_t x,y;
};
struct native_ui_event {
    uint32_t type,scan,character,modifiers;
    int32_t x,y,dx,dy;
    uint32_t buttons,changed;
    uint64_t tick;
};
_Static_assert(sizeof(struct native_ui_info)==24,"native UI info ABI");
_Static_assert(sizeof(struct native_ui_present)==32,"native UI present ABI");
_Static_assert(sizeof(struct native_ui_event)==48,"native UI event ABI");
struct process;
struct gfx_surface;
int native_ui_query(struct process *,uint64_t,struct native_ui_info *);
int64_t native_ui_acquire(struct process *,uint64_t,struct native_ui_info *);
int native_ui_release(struct process *,uint64_t);
int native_ui_visible(struct process *,uint64_t,bool);
int native_ui_draw(struct process *,uint64_t,const struct gfx_surface *,int,int);
void native_ui_release_owner(struct process *);
int native_ui_poll(struct process *,uint64_t,struct native_ui_event *);
bool native_ui_owns(struct process *);
void native_ui_emit(const struct native_ui_event *);
uint32_t native_ui_modifiers(void);
void native_ui_set_modifiers(uint32_t);
#endif
