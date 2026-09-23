#ifndef MYOS_VGA_H
#define MYOS_VGA_H

#define VGA_WIDTH 80u
#define VGA_HEIGHT 25u

/* Requires BIOS color text mode 03h and a flat, writable VGA mapping. */
void vga_clear(void);
void vga_putchar(char ch);
void kprint(const char *text);
/* Coordinates are zero-based; out-of-range requests are ignored. */
void vga_set_cursor(unsigned int column, unsigned int row);

#endif
