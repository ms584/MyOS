#include "idt.h"
#include "vga.h"

struct gate {
    unsigned short low, selector;
    unsigned char zero, flags;
    unsigned short high;
} __attribute__((packed));
struct descriptor {
    unsigned short limit;
    unsigned int base;
} __attribute__((packed));
static struct gate idt[256];
extern void (*isr_table[32])(void);

void idt_init(void)
{
    __asm__ volatile ("cli");
    for (unsigned int i = 0; i < 32; ++i) {
        unsigned int address = (unsigned int)isr_table[i];
        idt[i] = (struct gate){address & 0xffff, 0x08, 0, 0x8e, address >> 16};
    }
    struct descriptor pointer = {sizeof(idt) - 1, (unsigned int)idt};
    __asm__ volatile ("lidt %0" : : "m"(pointer) : "memory");
}

static void hex(unsigned int value)
{
    kprint("0x");
    for (int shift = 28; shift >= 0; shift -= 4)
        vga_putchar("0123456789ABCDEF"[(value >> shift) & 15]);
}

/* PUSHAD registers, normalized vector/error, then the CPU exception frame. */
struct frame {
    unsigned int edi, esi, ebp, esp, ebx, edx, ecx, eax;
    unsigned int vector, error, eip, cs, eflags;
};
__attribute__((noreturn)) void exception_handler(const struct frame *frame)
{
    vga_clear();
    kprint("CPU Exception: ");
    if (frame->vector == 6) kprint("Invalid Opcode");
    else if (frame->vector == 13) kprint("General Protection Fault");
    else if (frame->vector == 0) kprint("Divide Error");
    else kprint("Unhandled Exception");
    kprint("\nVector: "); hex(frame->vector);
    kprint("\nError: "); hex(frame->error);
    kprint("\nEIP: "); hex(frame->eip);
    kprint("\nCPU halted");
    for (;;) __asm__ volatile ("cli; hlt");
}
