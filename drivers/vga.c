#include "vga.h"

static unsigned int cursor_row;
static unsigned int cursor_column;

static void outb(unsigned short port, unsigned char value)
{
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static void update_cursor(void)
{
    unsigned int position = cursor_row * VGA_WIDTH + cursor_column;
    outb(0x3d4, 0x0f);
    outb(0x3d5, (unsigned char)position);
    outb(0x3d4, 0x0e);
    outb(0x3d5, (unsigned char)(position >> 8));
}

void vga_set_cursor(unsigned int column, unsigned int row)
{
    if (column >= VGA_WIDTH || row >= VGA_HEIGHT)
        return;
    cursor_column = column;
    cursor_row = row;
    update_cursor();
}

void vga_clear(void)
{
    volatile unsigned short *buffer = (volatile unsigned short *)0xb8000;
    for (unsigned int i = 0; i < VGA_WIDTH * VGA_HEIGHT; ++i)
        buffer[i] = 0x0f20;
    /* Reset the logical cursor whenever the screen is cleared. */
    cursor_row = 0;
    cursor_column = 0;
    update_cursor();
}

static void scroll_if_needed(void)
{
    volatile unsigned short *buffer = (volatile unsigned short *)0xb8000;
    if (cursor_row < VGA_HEIGHT)
        return;
    for (unsigned int i = 0; i < (VGA_HEIGHT - 1) * VGA_WIDTH; ++i)
        buffer[i] = buffer[i + VGA_WIDTH];
    for (unsigned int i = (VGA_HEIGHT - 1) * VGA_WIDTH;
         i < VGA_HEIGHT * VGA_WIDTH; ++i)
        buffer[i] = 0x0f20;
    cursor_row = VGA_HEIGHT - 1;
}

void vga_putchar(char ch)
{
    volatile unsigned short *buffer = (volatile unsigned short *)0xb8000;
    if (ch == '\n') {
        cursor_column = 0;
        ++cursor_row;
    } else if (ch == '\r') {
        cursor_column = 0;
    } else {
        buffer[cursor_row * VGA_WIDTH + cursor_column] =
            (unsigned short)(0x0f00u | (unsigned char)ch);
        if (++cursor_column == VGA_WIDTH) {
            cursor_column = 0;
            ++cursor_row;
        }
    }
    scroll_if_needed();
    update_cursor();
}

void kprint(const char *text)
{
    while (*text)
        vga_putchar(*text++);
}
