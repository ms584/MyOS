#ifndef MYOS_NATIVE_VIDEO_ABI_H
#define MYOS_NATIVE_VIDEO_ABI_H
#include <stdint.h>
#define MYOS_NATIVE_VIDEO_SYSCALL 60u
#define MYOS_NATIVE_VIDEO_ABI 1u
enum {NATIVE_VIDEO_SET=1,NATIVE_VIDEO_CLEAR=2};
#define NATIVE_VIDEO_MAX_WIDTH 160u
#define NATIVE_VIDEO_MAX_HEIGHT 90u
/* Private owned video underlay, not a Linux ABI. RGBA source is opaque.
 * Destination coordinates are in native display pixels; graphics RGBA above
 * the underlay is premultiplied. One bounded underlay belongs to the UI owner.
 * Query: zero arguments -> version1, or a negative error.
 * Set/clear: RDI=current UI handle,RSI=request,RDX=sizeof(request).
 * Set copies and validates the entire frame before changing the display.
 * Clear requires every other request field zero and resets timestamp history. */
struct native_video_request {
 uint32_t action,flags;
 uint64_t pixels;
 uint32_t width,height,stride,reserved;
 int32_t x,y,display_width,display_height;
 int64_t timestamp;
};
_Static_assert(sizeof(struct native_video_request)==56,"native video ABI");
struct process;
int native_ui_video_set(struct process *,uint64_t,uint32_t *,
                       const struct native_video_request *);
int native_ui_video_clear(struct process *,uint64_t);
#endif
