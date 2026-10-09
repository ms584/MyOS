#ifndef MYOS_MEDIA_VIDEO_OUTPUT_H
#define MYOS_MEDIA_VIDEO_OUTPUT_H
#include "native-video-abi.h"
static int64_t media_video_call(uint64_t handle,const struct native_video_request *request){
 register uint64_t a __asm__("rax")=MYOS_NATIVE_VIDEO_SYSCALL;
 __asm__ volatile("int $0x80":"+a"(a):"D"(handle),"S"((uintptr_t)request),
  "d"((uint64_t)(request?sizeof(*request):0)):"memory","cc");return (int64_t)a;
}
static int media_video_supported(void){return media_video_call(0,0)==MYOS_NATIVE_VIDEO_ABI;}
static int media_video_clear(uint64_t window){
 struct native_video_request r={.action=NATIVE_VIDEO_CLEAR};return (int)media_video_call(window,&r);
}
static int media_video_set(uint64_t window,const unsigned char *rgba,int width,int height,
                          int x,int y,int dw,int dh,int64_t timestamp){
 struct native_video_request r={NATIVE_VIDEO_SET,0,(uintptr_t)rgba,
  (uint32_t)width,(uint32_t)height,(uint32_t)width*4,0,x,y,dw,dh,timestamp};
 return (int)media_video_call(window,&r);
}
#endif
