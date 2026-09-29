ENTRY(malloc)
SECTIONS {
  . = 0x803f000000;
  .text : { *(.text*) *(.rodata*) }
  /DISCARD/ : { *(.comment*) *(.note*) *(.eh_frame*) }
}
