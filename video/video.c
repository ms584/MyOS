#include "video.h"
#include "audio.h"
#include "fs.h"
#include "graphics.h"

#define VIDEO_HEADER 24u
#define VIDEO_FRAMES 60u
#define VIDEO_COLORS 16u
#define VIDEO_PIXELS (160u*120u)

struct clip {
    uint8_t *data;
    size_t size;
    unsigned width,height,fps,frames,colors;
    uint32_t palette[VIDEO_COLORS],offsets[VIDEO_FRAMES];
    uint16_t lengths[VIDEO_FRAMES];
    const uint8_t *sound;
    unsigned samples;
};
static struct fs_handle handles[FS_HANDLES];
static unsigned u16(const uint8_t *p){return (unsigned)p[0]|((unsigned)p[1]<<8);}
static uint32_t u32(const uint8_t *p){return (uint32_t)u16(p)|((uint32_t)u16(p+2)<<16);}
static bool magic(const uint8_t *p){return p[0]=='M'&&p[1]=='Y'&&p[2]=='V'&&p[3]=='1';}
static void trace(const char *s){while(*s)serial_put(*s++);}
static void trace_number(uint64_t n){
    char digits[21];unsigned count=0;
    do{digits[count++]=(char)('0'+n%10);n/=10;}while(n);
    while(count)serial_put(digits[--count]);
}
static void trace_frame(unsigned index,uint64_t now){
    trace("VIDEO FRAME ");trace_number(index);trace(" tick ");trace_number(now);serial_put('\n');
}
static bool load(struct clip *video,const char *path){
    struct fs_entry info;
    if(fs_stat(path,&info)<0||info.type!=FS_FILE||
       info.size<VIDEO_HEADER||info.size>FS_FILE_MAX)return false;
    video->size=(size_t)info.size;
    video->data=kmalloc(video->size);
    if(!video->data)return false;
    int fd=fs_open(handles,path,FS_READ);
    if(fd<0)return false;
    int64_t got=fs_read(handles,fd,video->data,video->size);
    (void)fs_close(handles,fd);
    if(got!=(int64_t)video->size)return false;
    const uint8_t *data=video->data;
    if(!magic(data))return false;
    video->width=u16(data+4);video->height=u16(data+6);
    video->fps=u16(data+8);video->frames=u16(data+10);
    video->samples=u32(data+16);video->colors=u16(data+20);
    if(!video->width||video->width>160||!video->height||video->height>120||
       video->fps<5||video->fps>25||100%video->fps||
       !video->frames||video->frames>VIDEO_FRAMES||
       u32(data+12)!=11025||!video->colors||video->colors>VIDEO_COLORS||
       u16(data+22)!=0||video->samples!=
       (uint64_t)video->frames*11025/video->fps||
       video->samples>65536)return false;
    size_t table=VIDEO_HEADER+(size_t)video->colors*3;
    size_t offset=table+(size_t)video->frames*2;
    if(offset>video->size)return false;
    for(unsigned i=0;i<video->colors;++i){
        size_t at=VIDEO_HEADER+(size_t)i*3;
        video->palette[i]=((uint32_t)data[at]<<16)|((uint32_t)data[at+1]<<8)|data[at+2];
    }
    size_t pixels=(size_t)video->width*video->height;
    for(unsigned i=0;i<video->frames;++i){
        unsigned length=u16(data+table+(size_t)i*2);
        if(!length||length%2||length>video->size-offset)return false;
        video->offsets[i]=(uint32_t)offset;video->lengths[i]=(uint16_t)length;
        size_t decoded=0;
        for(size_t at=offset;at<offset+length;at+=2){
            unsigned run=data[at],color=data[at+1];
            if(!run||color>=video->colors||run>pixels-decoded)return false;
            decoded+=run;
        }
        if(decoded!=pixels)return false;
        offset+=length;
    }
    if(video->size-offset!=video->samples)return false;
    video->sound=data+offset;
    return true;
}
static void decode(const struct clip *video,unsigned index,uint8_t *pixels){
    const uint8_t *src=video->data+video->offsets[index];
    size_t at=0;
    for(size_t i=0;i<video->lengths[index];i+=2)
        for(unsigned n=0;n<src[i];++n)pixels[at++]=src[i+1];
}
static void paint(const struct clip *video,const uint8_t *pixels,
                  struct gfx_surface *scene,unsigned scale,int left,int top,
                  unsigned frame,bool paused){
    for(unsigned y=0;y<video->height;++y)
        for(unsigned x=0;x<video->width;++x){
            uint32_t color=video->palette[pixels[(size_t)y*video->width+x]];
            for(unsigned sy=0;sy<scale;++sy){
                uint32_t *dst=scene->pixels+(size_t)(top+(int)(y*scale+sy))*scene->stride+
                              left+(int)(x*scale);
                for(unsigned sx=0;sx<scale;++sx)dst[sx]=color;
            }
        }
    gfx_rect(scene,left,top+(int)(video->height*scale)+16,
             (int)(video->width*scale),12,0x293b56);
    gfx_rect(scene,left,top+(int)(video->height*scale)+16,
             (int)(((uint64_t)(frame+1)*video->width*scale)/video->frames),12,0x36c6bc);
    gfx_rect(scene,left,top+(int)(video->height*scale)+40,
             (int)(video->width*scale),20,0x101a2e);
    gfx_text(scene,left,top+(int)(video->height*scale)+40,
             paused?"Paused | Space resume | Esc exit":
                    "Space pause | Esc exit",0xffffff);
    (void)gfx_present(scene,0,0);
}
bool video_play(const char *path){
    struct gfx_mode mode;
    if(!gfx_enabled()||!gfx_probe(&mode)||mode.width<640||mode.height<480){
        print("Video requires framebuffer >= 640x480\n");return false;
    }
    if(!audio_ready()){print("Video requires SB16 audio\n");return false;}
    struct clip video={0};
    if(!load(&video,path)){
        kfree(video.data);print("Video: invalid or unavailable MYV1 clip\n");return false;
    }
    size_t count=(size_t)video.width*video.height;
    uint8_t *pixels=kmalloc(count);
    struct gfx_surface *scene=gfx_surface_create(mode.width,mode.height);
    if(!pixels||!scene){
        kfree(pixels);gfx_surface_destroy(scene);kfree(video.data);
        print("Video: out of memory\n");return false;
    }
    unsigned scale=(mode.width-80)/video.width;
    unsigned height_scale=(mode.height-160)/video.height;
    if(scale>height_scale)scale=height_scale;
    if(scale>4)scale=4;
    if(!scale){
        kfree(pixels);gfx_surface_destroy(scene);kfree(video.data);
        print("Video: display too small\n");return false;
    }
    int left=((int)mode.width-(int)(video.width*scale))/2;
    int top=((int)mode.height-(int)(video.height*scale))/2-20;
    gfx_clear(scene,0x101a2e);
    gfx_text(scene,left,top-35,"MyOS Video | MYV1 RLE + PCM 11025 Hz",0xffffff);
    print("Video: MYV1 ");decimal(video.width);print("x");decimal(video.height);
    print(" ");decimal(video.fps);print(" fps, ");decimal(video.frames);
    print(" frames, PCM 11025 Hz\n");
    /* The clip owns SB16 playback: discard any earlier tone/WAV queue. */
    audio_stop();
    if(!audio_queue(video.sound,video.samples)){
        kfree(pixels);gfx_surface_destroy(scene);kfree(video.data);
        print("Video: audio queue unavailable\n");return false;
    }
    uint64_t start=ticks,elapsed=0;
    unsigned interval=100/video.fps,duration=video.frames*interval;
    unsigned shown=0,dropped=0;
    int previous=-1;
    bool paused=false,aborted=false;
    trace("VIDEO START tick ");trace_number(start);serial_put('\n');
    for(;;){
        int key=keyboard_get();
        if(key==27||key=='q'||key=='Q'){aborted=true;break;}
        if(key==' '){
            if(!paused){
                elapsed=ticks-start;
                if(elapsed>=duration)break;
                audio_stop();paused=true;trace("VIDEO PAUSED\n");
            }else{
                uint64_t offset=elapsed*11025/100;
                if(offset>=video.samples||
                   !audio_queue(video.sound+offset,video.samples-(unsigned)offset)){
                    aborted=true;break;
                }
                start=ticks-elapsed;paused=false;trace("VIDEO RESUMED\n");
            }
        }
        if(!paused)elapsed=ticks-start;
        if(elapsed>=duration)break;
        unsigned frame=(unsigned)(elapsed/interval);
        if((int)frame!=previous){
            if(previous>=0&&frame>(unsigned)previous+1)dropped+=frame-(unsigned)previous-1;
            decode(&video,frame,pixels);
            paint(&video,pixels,scene,scale,left,top,frame,paused);
            previous=(int)frame;++shown;
            trace_frame(frame,ticks);
        }else if(key==' '&&paused){
            paint(&video,pixels,scene,scale,left,top,frame,true);
        }
        __asm__ volatile("sti; hlt":::"memory");
    }
    audio_stop();
    gfx_surface_destroy(scene);kfree(pixels);kfree(video.data);
    console_clear();
    print(aborted?"Video stopped: ":"Video complete: ");
    print("displayed ");decimal(shown);print(" frames, dropped ");decimal(dropped);print("\n");
    trace(aborted?"VIDEO STOPPED\n":"VIDEO COMPLETE\n");
    return !aborted;
}
