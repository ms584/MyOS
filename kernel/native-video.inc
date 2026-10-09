/* Included by native-ui.c after native UI definitions. */
#include "native-video-abi.h"
struct video_layer {
 uint32_t *pixels,width,height;
 int x,y,width_out,height_out;
 int64_t timestamp;
};
static struct video_layer video_layer;
static uint32_t composite_pixels[64];
static void video_dispose(void){
 if(video_layer.pixels)kfree(video_layer.pixels);
 video_layer=(struct video_layer){0};
}
static int present_base_row(unsigned x,unsigned y,unsigned width){
 if(!visible)return 0;
 struct gfx_surface row={0,1,0,composite_pixels};
 while(width){
  unsigned n=width>64?64:width;
  for(unsigned i=0;i<n;++i){
   unsigned px=x+i;uint32_t base=foreground->pixels[(size_t)y*foreground->stride+px];
   int64_t vx=(int64_t)px-video_layer.x,vy=(int64_t)y-video_layer.y;
   uint32_t color=base;
   if(video_layer.pixels&&vx>=0&&vy>=0&&vx<video_layer.width_out&&vy<video_layer.height_out){
    unsigned sx=(unsigned)(vx*video_layer.width/video_layer.width_out);
    unsigned sy=(unsigned)(vy*video_layer.height/video_layer.height_out);
    uint32_t behind=video_layer.pixels[(size_t)sy*video_layer.width+sx];
    unsigned inverse=255-(base>>24),r=((base>>16)&255)+(((behind>>16)&255)*inverse+127)/255;
    unsigned g=((base>>8)&255)+(((behind>>8)&255)*inverse+127)/255;
    unsigned b=(base&255)+((behind&255)*inverse+127)/255;
    color=((r>255?255:r)<<16)|((g>255?255:g)<<8)|(b>255?255:b);
   }
   composite_pixels[i]=color&0xffffff;
  }
  row.width=row.stride=n;if(!gfx_present(&row,(int)x,(int)y))return -95;
  x+=n;width-=n;
 }
 return 0;
}
static int redraw_video_rect(int x,int y,int width,int height){
 int64_t left=x,top=y,right=(int64_t)x+width,bottom=(int64_t)y+height;
 if(left<0)left=0;if(top<0)top=0;
 if(right>foreground->width)right=foreground->width;
 if(bottom>foreground->height)bottom=foreground->height;
 if(right<=left||bottom<=top)return 0;
 for(int64_t row=top;row<bottom;++row){
  int rc=present_base_row((unsigned)left,(unsigned)row,(unsigned)(right-left));if(rc)return rc;
 }
 return 0;
}
int native_ui_video_set(struct process *process,uint64_t handle,uint32_t *pixels,
                       const struct native_video_request *request){
 if(!matches(process,handle))return -9;
 if(!pixels||!request)return -22;
 if(video_layer.pixels&&request->timestamp<video_layer.timestamp)return -22;
 struct video_layer old=video_layer;
 video_layer=(struct video_layer){pixels,request->width,request->height,
  request->x,request->y,request->display_width,request->display_height,request->timestamp};
 /* Ownership transfers here. Present failure still leaves a valid bounded
  * retained frame; caller must not free it after this commit point. */
 if(old.pixels)kfree(old.pixels);
 bool moved=old.x!=video_layer.x||old.y!=video_layer.y||
  old.width_out!=video_layer.width_out||old.height_out!=video_layer.height_out;
 int rc=old.pixels&&moved?redraw_video_rect(old.x,old.y,old.width_out,old.height_out):0;
 int next=redraw_video_rect(video_layer.x,video_layer.y,video_layer.width_out,video_layer.height_out);
 return rc?rc:next;
}
int native_ui_video_clear(struct process *process,uint64_t handle){
 if(!matches(process,handle))return -9;
 struct video_layer old=video_layer;video_dispose();
 return old.pixels?redraw_video_rect(old.x,old.y,old.width_out,old.height_out):0;
}
