void kernel_main(void)
{
    volatile unsigned short* vga = (unsigned short*)0xB8000;

    const char* message = "Hello from C Kernel!";

    for (int i = 0; message[i] != '\0'; i++) {
        vga[i] = (0x0F << 8) | message[i];
    }

    while (1) {
        __asm__ volatile ("hlt");
    }
}