#include "boot.h"
#include "os.h"
#include "graphics.h"
static unsigned row,col,width,height,pitch,pixels_w,pixels_h;
static volatile uint32_t *fb;
static char cells[160*50];
static bool graphics;
void serial_put(char c){for(unsigned n=0;n<100000;++n)if(inb(0x3fd)&32){outb(0x3f8,(uint8_t)c);break;}}
static void cell(unsigned x,unsigned y,char c){
    cells[y*width+x]=c;
    if(!graphics){((volatile uint16_t *)0xb8000)[y*width+x]=0x0f00|(uint8_t)c;return;}
    const uint8_t *font=(const uint8_t *)boot_state.font+(uint8_t)c*16;
    for(unsigned dy=0;dy<16;++dy)for(unsigned dx=0;dx<8;++dx)
        fb[(y*16+dy)*pitch+x*8+dx]=(font[dy]&(0x80>>dx))?0xffffff:0;
}
static void cursor(void){if(!graphics){unsigned p=row*width+col;outb(0x3d4,15);outb(0x3d5,p);outb(0x3d4,14);outb(0x3d5,p>>8);}}
void console_clear(void){
    uint64_t f=irq_save();
    if(graphics)for(unsigned y=0;y<pixels_h;++y)for(unsigned x=0;x<pixels_w;++x)fb[y*pitch+x]=0;
    for(unsigned y=0;y<height;++y)for(unsigned x=0;x<width;++x)cell(x,y,' ');
    row=col=0;cursor();irq_restore(f);
}
void console_init(void){
    outb(0x3f9,0);outb(0x3fb,0x80);outb(0x3f8,1);outb(0x3f9,0);outb(0x3fb,3);outb(0x3fa,0xc7);outb(0x3fc,0x0b);
    gfx_init();graphics=gfx_enabled();
    if(graphics){
        struct gfx_mode mode;check(gfx_probe(&mode),"validated framebuffer mode");
        pixels_w=mode.width;pixels_h=mode.height;pitch=mode.pitch;fb=mode.pixels;
        width=pixels_w/8;height=pixels_h/16;
    }else{width=80;height=25;}
    console_clear();
    if(graphics){print("Console: framebuffer ");decimal(pixels_w);print("x");decimal(pixels_h);print("x32\n");}
    else print("Console: VGA text 80x25\n");
}
void putchar(char c){
    uint64_t f=irq_save();serial_put(c);
    if(c=='\n'){col=0;++row;}
    else if(c=='\r')col=0;
    else if(c=='\b'){if(col||row){if(!col){--row;col=width;}--col;cell(col,row,' ');}}
    else {cell(col,row,c);if(++col==width){col=0;++row;}}
    if(row>=height){
        for(unsigned y=1;y<height;++y)for(unsigned x=0;x<width;++x)cell(x,y-1,cells[y*width+x]);
        for(unsigned x=0;x<width;++x)cell(x,height-1,' ');
        row=height-1;
    }
    cursor();irq_restore(f);
}
void print(const char *s){uint64_t f=irq_save();while(*s)putchar(*s++);irq_restore(f);}
void hex(uint64_t n){print("0x");for(int i=60;i>=0;i-=4)putchar("0123456789ABCDEF"[(n>>i)&15]);}
void decimal(uint64_t n){char b[21];unsigned i=0;do{b[i++]=(char)('0'+n%10);n/=10;}while(n);while(i)putchar(b[--i]);}
