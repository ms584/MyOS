#include "timer.h"
#include "io.h"

static volatile unsigned int ticks;

void timer_init(void)
{
    __asm__ volatile ("cli" : : : "memory");
    /* Remap master/slave PIC away from CPU exceptions, to vectors 32..47. */
    outb(0x21, 0xff); outb(0xa1, 0xff);
    outb(0x20, 0x11); io_wait(); outb(0xa0, 0x11); io_wait();
    outb(0x21, 0x20); io_wait(); outb(0xa1, 0x28); io_wait();
    outb(0x21, 0x04); io_wait(); outb(0xa1, 0x02); io_wait();
    outb(0x21, 0x01); io_wait(); outb(0xa1, 0x01); io_wait();
    outb(0x21, 0xff); outb(0xa1, 0xff);

    ticks = 0;
    /* PIT channel 0, rate generator, divisor 11932: approximately 100 Hz. */
    outb(0x43, 0x34);
    outb(0x40, 11932 & 0xff);
    outb(0x40, 11932 >> 8);
    outb(0x21, 0xfe);
}

unsigned int timer_ticks(void) { return ticks; }

void irq_handler(unsigned int vector)
{
    /* Spurious IRQ7 needs no EOI; spurious IRQ15 needs only master EOI. */
    if (vector == 39) {
        outb(0x20, 0x0b);
        if (!(inb(0x20) & 0x80)) return;
    }
    if (vector == 47) {
        outb(0xa0, 0x0b);
        if (!(inb(0xa0) & 0x80)) { outb(0x20, 0x20); return; }
    }
    if (vector == 32) ++ticks;
    if (vector >= 40) outb(0xa0, 0x20);
    outb(0x20, 0x20);
}
