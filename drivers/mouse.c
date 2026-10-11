#include "mouse.h"
#include "native-ui.h"
#include "vmmouse.h"

#define MOUSE_QUEUE 64u
static struct mouse_event events[MOUSE_QUEUE];
static volatile unsigned head,tail;
static volatile uint32_t dropped;
static uint8_t packet[3],packet_index,buttons;
static uint16_t bound_width,bound_height;
static int x_position,y_position;
static bool ready;

static bool input_ready(void)
{
    for(unsigned n=0;n<100000;++n)if(!(inb(0x64)&2))return true;
    return false;
}
static bool output_ready(void)
{
    for(unsigned n=0;n<100000;++n)if(inb(0x64)&1)return true;
    return false;
}
static bool controller(uint8_t command)
{
    if(!input_ready())return false;
    outb(0x64,command);return true;
}
static bool device(uint8_t command)
{
    if(!controller(0xd4) || !input_ready())return false;
    outb(0x60,command);
    return output_ready() && inb(0x60)==0xfa;
}
bool mouse_init(uint16_t width,uint16_t height)
{
    if(!width || !height)return false;
    bound_width=width;bound_height=height;
    x_position=width/2;y_position=height/2;
    if(!controller(0xa8) || !controller(0x20) || !output_ready())return false;
    uint8_t configuration=inb(0x60);
    if(!controller(0x60) || !input_ready())return false;
    outb(0x60,(configuration|3u)&~0x20u);
    if(!device(0xf6) || !device(0xf4))return false;
    /* IRQ12 travels through the slave PIC and the master cascade (IRQ2). */
    outb(0xa1,inb(0xa1)&~0x10u);
    outb(0x21,inb(0x21)&~0x04u);
    ready=true;
    return true;
}
bool mouse_ready(void){return ready;}
bool mouse_absolute_enable(void)
{
    uint64_t flags=irq_save();
    bool result=ready&&vmmouse_enable();
    if(result){packet_index=0;tail=head;}
    irq_restore(flags);return result;
}
void mouse_relative_enable(void)
{
    uint64_t flags=irq_save();vmmouse_disable();packet_index=0;tail=head;irq_restore(flags);
}
static void enqueue(struct mouse_event event)
{
    unsigned next=(head+1)%MOUSE_QUEUE;
    if(next==tail){tail=(tail+1)%MOUSE_QUEUE;++dropped;}
    events[head]=event;
    __asm__ volatile("":::"memory");
    head=next;
}
static void report_position(int nx,int ny,uint8_t current)
{
    if(nx<0)nx=0;if(nx>=bound_width)nx=bound_width-1;
    if(ny<0)ny=0;if(ny>=bound_height)ny=bound_height-1;
    uint8_t changed=current^buttons;
    struct mouse_event event={nx,ny,nx-x_position,ny-y_position,current,changed};
    x_position=nx;y_position=ny;buttons=current;
    if(event.dx || event.dy || changed){
        enqueue(event);
        struct native_ui_event native={NATIVE_UI_POINTER,0,0,native_ui_modifiers(),
            event.x,event.y,event.dx,event.dy,event.buttons,event.changed,0};
        native_ui_emit(&native);
    }
}
void mouse_irq(void)
{
    uint8_t status=inb(0x64);
    if(!(status&1))return;
    uint8_t value=inb(0x60);
    if(!(status&0x20))return;
    if(!packet_index){if(!(value&8))return;packet[0]=value;packet_index=1;return;}
    packet[packet_index++]=value;
    if(packet_index<3)return;
    packet_index=0;
    if(vmmouse_enabled()){
        /* PS/2 bytes notify us; positions come from the bounded virtual queue. */
        for(unsigned n=0;n<256;++n){
            struct vmmouse_event event;
            if(vmmouse_read(&event)!=1)break;
            int nx=((uint32_t)event.x*(bound_width-1)+32767u)/65535u;
            int ny=((uint32_t)event.y*(bound_height-1)+32767u)/65535u;
            report_position(nx,ny,event.buttons);
        }
        return;
    }
    if(packet[0]&0xc0)return; /* PS/2 overflow: discard this packet. */
    int dx=(int8_t)packet[1],dy=-(int8_t)packet[2];
    report_position(x_position+dx,y_position+dy,packet[0]&7u);
}

bool mouse_get(struct mouse_event *event)
{
    if(!event)return false;
    uint64_t flags=irq_save();
    bool available=tail!=head;
    if(available){*event=events[tail];tail=(tail+1)%MOUSE_QUEUE;}
    irq_restore(flags);return available;
}
void mouse_position(int *x,int *y,uint8_t *pressed)
{
    uint64_t flags=irq_save();
    if(x)*x=x_position;if(y)*y=y_position;if(pressed)*pressed=buttons;
    irq_restore(flags);
}
void mouse_flush(void)
{
    uint64_t flags=irq_save();tail=head;irq_restore(flags);
}
uint32_t mouse_dropped(void)
{
    uint64_t flags=irq_save();uint32_t count=dropped;irq_restore(flags);return count;
}
