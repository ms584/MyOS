ENTRY(_start)
SECTIONS {
 . = 0x8000000000;
 .text : { *(.text*) *(.rodata*) }
 .data : { *(.data*) }
 .bss : { *(.bss*) *(COMMON) }
 ASSERT(SIZEOF(.data) == 0, "writable globals are unsupported in app v1")
 ASSERT(SIZEOF(.bss) == 0, "BSS is unsupported in app v1")
 /DISCARD/ : { *(.comment*) *(.note*) *(.eh_frame*) }
}
