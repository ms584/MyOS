ENTRY(kernel_entry)

PHDRS
{
    code PT_LOAD FLAGS(5); /* Read + execute */
    data PT_LOAD FLAGS(6); /* Read + write */
}

SECTIONS
{
    . = 0x10000;
    .text : { KEEP(*(.text.entry)) *(.text*) } :code
    .rodata : { *(.rodata*) } :code
    . = ALIGN(16);
    .data : { *(.data*) } :data
    .bss (NOLOAD) :
    {
        __bss_start = .;
        *(COMMON)
        *(.bss*)
        __bss_end = .;
    } :data
    ASSERT(kernel_entry == 0x10000, "Kernel entry must be at 0x10000")
    ASSERT(__bss_end <= 0x80000, "Kernel overlaps reserved stack area")
}
