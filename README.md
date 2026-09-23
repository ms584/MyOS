# MyOS — Phase 2.3

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
qemu-system-x86_64 -drive format=raw,file=build/myos-phase23.img,if=floppy -boot order=a
```

Expected startup:

```text
MyOS Kernel
32-bit Protected Mode
VGA driver initialized
IDT initialized
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
- Interrupts remain disabled until future IDT/PIC work. VGA output is intended
  for single-threaded kernel use, not simultaneous IRQ writers.

Build outputs and test images are ignored by Git. The previous `build/myos.img`
is preserved; use `build/myos-phase23.img` to run this phase.

## Phase 2.3: CPU exceptions

The IDT installs interrupt gates for vectors 0–31 using the existing flat GDT.
Assembly stubs normalize error codes and align the stack before calling C.
The fatal handler prints the vector, CPU error code and fault EIP, then halts.
`test-exceptions.py` triggers UD2 (#UD) and an invalid data selector (#GP) in
separate images and checks the exact fault EIP against ELF symbols.

IRQs remain disabled; vectors 32–255 are not installed. This phase does not
support recovery, user-mode exceptions, or a dedicated double-fault stack.
Paging is still off. Keep interrupts disabled until PIC/IRQ support is ready.
`build-phase22.sh` remains a compatibility wrapper for the current build.
