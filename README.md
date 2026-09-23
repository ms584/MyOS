# MyOS — PIC and Timer IRQ

A learning OS with a BIOS two-stage bootloader and a 32-bit C kernel.
Phase 2.2 adds an 80x25 VGA text driver: clear, print, LF/CR, wrapping,
scrolling, and hardware cursor positioning.

## Build and run (Linux / WSL Ubuntu)

Dependencies: GCC with 32-bit code generation, GNU binutils, NASM, Python 3,
and qemu-system-x86_64. No hosted C library is linked.

```bash
bash build.sh
python3 test-vga.py
python3 test-exceptions.py
python3 test-timer.py
qemu-system-x86_64 -drive format=raw,file=build/myos-current.img,if=floppy -boot order=a
```

Expected startup:

```text
MyOS Kernel
32-bit Protected Mode
VGA driver initialized
IDT initialized
Timer IRQ initialized: 100 Hz
```

`test-vga.py` boots separate test images, checks VGA memory and hardware cursor
registers, and verifies clear, cursor placement, wrapping, scrolling, CR,
bottom-right behavior, and invalid coordinates. Its test-only entry first poisons
BSS with 0xA5 to verify startup zeroing. Finally it runs `test-boot.py` against the
normal image. Tests do not modify kernel source or replace the normal image.
QEMU termination after the startup test is normal.

## Layout and limits

- Boot sector: LBA 0, 512 bytes; Stage 2: LBA 1–4, 2048 bytes.
- Kernel: LBA 5–12, loaded at 0x10000; maximum raw size 4096 bytes.
- Entry clears the direction flag and BSS before calling C.
- ELF code/rodata use RX and data/BSS use RW segments. These describe ELF
  layout; CPU memory protection requires paging, which is not enabled yet.
- Stage 2 selects BIOS color text mode before entering protected mode.
- Only timer IRQ0 is enabled. The handler updates a counter and never writes
  VGA; console output remains single-threaded.

Build outputs and test images are ignored by Git. The previous `build/myos.img`
is preserved; use `build/myos-current.img` to run this phase.

## Phase 2.3: CPU exceptions

The IDT installs interrupt gates for vectors 0–31 using the existing flat GDT.
Assembly stubs normalize error codes and align the stack before calling C.
The fatal handler prints the vector, CPU error code and fault EIP, then halts.
`test-exceptions.py` triggers UD2 (#UD) and an invalid data selector (#GP) in
separate images and checks the exact fault EIP against ELF symbols.

Exception handlers remain fatal, with no recovery, user-mode support, or
dedicated double-fault stack. Paging is still off.
`build-phase22.sh` remains a compatibility wrapper for the current build.

## PIC and PIT — roadmap Phase 4, timer foundation for Phase 8

The legacy PICs are remapped to vectors 32–47. Only master IRQ0 is unmasked;
keyboard, cascade and all slave IRQs stay masked. PIT channel 0 uses mode 2
with divisor 11932 (approximately 100 Hz). After initialization the kernel
uses STI/HLT to sleep until interrupts arrive.

The IRQ stub preserves general and segment registers, clears DF for C, and
restores interrupted EFLAGS through IRETD. The handler counts timer ticks and
sends EOI; spurious IRQ7/15 have special EOI handling. `timer_ticks()` returns
a 32-bit counter that wraps after approximately 497 days. No scheduler yet.

`python3 test-timer.py` boots an isolated image and waits for 100 real PIT
interrupts, checking register/DF preservation, counter progress, PIC masks
and the cleared in-service register. The normal screen is static; it does
not display live ticks. Build outputs now use `build/myos-current.img` so
future steps do not require changing phase-numbered paths.
