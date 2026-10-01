ENTRY(malloc)
PHDRS { code PT_LOAD FLAGS(5); }
SECTIONS {
  . = 0x803f000000;
  .text : { *(.text*) *(.rodata*) *(.data.rel.ro*) } :code
  .mutable (NOLOAD) : { *(.data*) *(.bss*) *(.got*) *(.tdata*) *(.tbss*) *(COMMON) } :code
  ASSERT(SIZEOF(.mutable) == 0,"runtime state must not live in RX pages")
  ASSERT(. <= 0x803f010000,"runtime overlaps private TLS pages")
  /DISCARD/ : { *(.comment*) *(.note*) *(.eh_frame*) }
}
