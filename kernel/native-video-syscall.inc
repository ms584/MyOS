/* Included after user-copy validators, before syscall_dispatch. */
#include "native-video-abi.h"
static int64_t native_video_call(struct process *p,struct frame *f){
 if(!f->rdi&&!f->rsi&&!f->rdx)return gfx_enabled()?(int64_t)MYOS_NATIVE_VIDEO_ABI:(int64_t)-95;
 if(!f->rdi||!native_ui_owns(p))return -9;
 struct native_ui_info info;int rc=native_ui_query(p,f->rdi,&info);if(rc)return rc;
 struct native_video_request request;
 if(f->rdx!=sizeof(request))return -22;
 if(!copy_from_user(p,(char *)&request,f->rsi,sizeof(request)))return -14;
 if(request.flags||request.reserved)return -22;
 if(request.action==NATIVE_VIDEO_CLEAR){
  if(request.pixels||request.width||request.height||request.stride||request.x||request.y||
     request.display_width||request.display_height||request.timestamp)return -22;
  return native_ui_video_clear(p,f->rdi);
 }
 if(request.action!=NATIVE_VIDEO_SET||!request.pixels||!request.width||!request.height||
    request.width>NATIVE_VIDEO_MAX_WIDTH||request.height>NATIVE_VIDEO_MAX_HEIGHT||
    request.stride!=request.width*4||request.display_width<=0||request.display_height<=0||
    request.display_width>4096||request.display_height>4096||
    request.x < -8192||request.x>8192||request.y < -8192||request.y>8192||request.timestamp<0)return -22;
 size_t bytes=(size_t)request.width*request.height*4;
 if(!readable_range(p,request.pixels,bytes))return -14;
 uint32_t *pixels=kmalloc(bytes);if(!pixels)return -12;
 if(!copy_from_user(p,(char *)pixels,request.pixels,bytes)){kfree(pixels);return -14;}
 for(size_t i=0;i<bytes/4;++i){
  uint32_t rgba=pixels[i];
  pixels[i]=((rgba&255)<<16)|(rgba&0xff00)|((rgba>>16)&255);
 }
 rc=native_ui_video_set(p,f->rdi,pixels,&request);
 /* Failed validation never transfers ownership. Display errors after commit
  * retain the frame and return through normal UI cleanup. */
 if(rc==-9||rc==-22)kfree(pixels);
 return rc;
}
