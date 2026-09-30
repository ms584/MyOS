ENTRY(kernel_entry)
PHDRS { code PT_LOAD FLAGS(5); data PT_LOAD FLAGS(6); }
SECTIONS {
 . = 0x10000;
 .text : { KEEP(*(.text.entry)) *(.text*) } :code
 .rodata : { *(.rodata*) } :code
 . = ALIGN(4096);
 .data : { *(.platform_runtime*) *(.data*) } :data
 .bss (NOLOAD) : { __bss_start = .; *(COMMON) *(.bss*) __bss_end = .; } :data
 ASSERT(kernel_entry == 0x10000,"entry address mismatch")
 /* UEFI reserves 0x10000..0x7ffff for kernel+BSS; BIOS enters with its
    stack at 0x90000.  Keep the image itself under the separate 256 KiB cap. */
 ASSERT(__bss_end < 0x80000,"kernel exceeds low-memory reservation")
}
