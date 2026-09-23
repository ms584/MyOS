#include "vga.h"
#include "idt.h"
#include "timer.h"

void kernel_main(void)
{
    vga_clear();
    kprint("MyOS Kernel\n");
    kprint("32-bit Protected Mode\n");
    kprint("VGA driver initialized\n");

    idt_init();
    kprint("IDT initialized\n");

    timer_init();
    kprint("Timer IRQ initialized: 100 Hz\n");

    for (;;) {
        __asm__ volatile ("sti; hlt");
    }
}
