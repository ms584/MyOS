ENTRY(_start)
SECTIONS {
 . = 0x8000000000;
 .text : { *(.text*) *(.rodata*) }
 . = 0x8000010000;
 .data : { *(.data*) }
 .bss : { *(.bss*) *(COMMON) }
 ASSERT(SIZEOF(.data) == 0, "writable globals are unsupported in app v1")
 ASSERT(SIZEOF(.bss) <= 4096, "app BSS exceeds the private data page")
 /DISCARD/ : { *(.comment*) *(.note*) *(.eh_frame*) }
}
