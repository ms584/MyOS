#include "syscall.h"
#include "mouse.h"
#include "audio.h"
#include "tcp.h"
extern void syscall_entry(void);
struct gate {uint16_t low,selector;uint8_t ist,flags;uint16_t mid;uint32_t high,zero;} __attribute__((packed));
struct descriptor {uint16_t limit;uint64_t base;} __attribute__((packed));
static struct gate idt[256];
extern void *isr_table[48];
volatile uint64_t ticks;
static void wait_io(void){outb(0x80,0);}
void interrupts_init(void){
    for(unsigned i=0;i<48;++i){uint64_t a=(uint64_t)isr_table[i];idt[i]=(struct gate){a,0x18,0,0x8e,a>>16,a>>32,0};}
    uint64_t a = (uint64_t)syscall_entry;
    idt[128] = (struct gate){a,0x18,0,0xee,a>>16,a>>32,0};
    struct descriptor d={sizeof(idt)-1,(uint64_t)idt};__asm__ volatile("lidt %0"::"m"(d):"memory");
    outb(0x21,0xff);outb(0xa1,0xff);
    outb(0x20,0x11);wait_io();outb(0xa0,0x11);wait_io();
    outb(0x21,32);wait_io();outb(0xa1,40);wait_io();
    outb(0x21,4);wait_io();outb(0xa1,2);wait_io();
    outb(0x21,1);wait_io();outb(0xa1,1);wait_io();
    outb(0x21,0xff);outb(0xa1,0xff);
}
void timer_init(void){outb(0x43,0x34);outb(0x40,11932&255);outb(0x40,11932>>8);outb(0x21,inb(0x21)&~1u);}
struct frame *interrupt_dispatch(struct frame *f){
    if (f->vector == 128) return syscall_dispatch(f);
    if(f->vector<32){
        if ((f->cs & 3) == 3 && f->vector != 2 && f->vector != 8 && f->vector != 18)
            return process_fault(f);
        print("\nEXCEPTION ");decimal(f->vector);print(" error=");hex(f->error);print(" rip=");hex(f->rip);
        if(f->vector==14){uintptr_t cr2;__asm__ volatile("mov %%cr2,%0":"=r"(cr2));print(" cr2=");hex(cr2);}
        panic(f->vector==6?"Invalid Opcode":f->vector==13?"General Protection Fault":f->vector==14?"Page Fault":"CPU exception");
    }
    if(f->vector==39){outb(0x20,0x0b);if(!(inb(0x20)&128))return f;}
    if(f->vector==47){outb(0xa0,0x0b);if(!(inb(0xa0)&128)){outb(0x20,0x20);return f;}}
    if(f->vector==32){++ticks;tcp_stream_tick();}
    if(f->vector==33)keyboard_irq();
    if(f->vector==44)mouse_irq();
    if(f->vector==37)audio_irq();
    if(f->vector>=40)outb(0xa0,0x20);
    outb(0x20,0x20);
    return f->vector==32?schedule(f):f;
}
