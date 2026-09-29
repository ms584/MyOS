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
 ASSERT(__bss_end < 0x72000,"kernel exceeds low-memory reservation")
}
