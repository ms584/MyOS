# MyOS — foundations through Phase 8

MyOS now boots from BIOS into a **64-bit x86-64 kernel** with a framebuffer
console, PS/2 keyboard input, physical/virtual memory allocation, a kernel heap,
and a timer-driven preemptive kernel-task scheduler.

## Build, test, run

On Linux/WSL Ubuntu, install GCC, GNU binutils, NASM, Python 3 and QEMU.
The kernel is freestanding: no Linux runtime or C library is linked.

```bash
cd ~/myos
bash build.sh
python3 tests/test_system.py
qemu-system-x86_64 -m 64M \
  -drive format=raw,file=build/myos-current.img,if=floppy -boot order=a
```

QEMU shows the boot checks and a `myos>` input prompt. Click the window and type.
Shift, Caps Lock, Enter and Backspace work with a US keyboard layout. This is
an **input/echo console**, not the Phase 12 command shell. Two CPU-bound kernel
tasks run in the background without voluntarily yielding; the timer preempts
them. A third task returns and is removed from the runnable set.

The default console is VBE 800x600x32 with the BIOS 8x16 font. If unavailable,
the loader falls back to VGA text. To force the fallback:

```bash
TEXT_ONLY=1 bash build.sh
```

Run `bash build.sh` again to restore the framebuffer build. Builds use
`build/myos-current.img`; older phase images are not overwritten.

## Validation

`python3 tests/test_system.py` builds and boots isolated variants, then restores
the normal image even if a test fails. It tests:

- BIOS loading across CHS track/head boundaries; entry into a 64-bit C kernel.
- Boot, PMM/heap exhaustion and recovery, and preemption with 16, 64 and 256 MiB RAM.
- E820-derived page allocation; map/translate/unmap and page-table reclamation;
  heap alignment, splitting/coalescing, and out-of-memory behavior.
- Actual QEMU PS/2 key events, modifiers, releases, extended-key filtering,
  backspace, wrapping/scrolling and output in both framebuffer and VGA modes.
- Repeated timer preemption of two CPU-bound tasks; general-register and
  direction-flag preservation; safe task completion; task capacity limits.
- Invalid opcode and general protection exceptions with the exact fault RIP;
  page faults for missing pages, read-only writes and NX execution, including
  the CPU error code and CR2 address.

Serial transcripts and PPM screenshots are saved in `build/system-tests/`.
Focused runs use `--suite boot|memory|keyboard|exceptions|timer`.
The old `test-boot.py`, `test-vga.py`, `test-timer.py`, `test-exceptions.py` and
`build-phase22.sh` commands are compatibility entry points for the current
implementation. Earlier 16/32-bit implementations remain in Git history.

## Design and present limits

- BIOS floppy layout: boot at LBA 0, 8 KiB Stage 2 at LBA 1–16, kernel from LBA 17.
  The build derives the kernel sector count and rejects kernels above 256 KiB.
- Boot information at 0x7000; E820 at 0x7400; font at 0xA000; VBE data at 0xB200.
  Kernel at 0x10000; the first 1 MiB is reserved by the physical allocator.
- Four-level paging initially identity-maps 4 GiB. The PMM manages only usable
  E820 pages below 256 MiB. The heap maps 1 MiB at virtual address 4 GiB with NX.
  Dynamically allocated page tables are reclaimed when mappings are removed.
- `kmalloc`/`kfree` are 16-byte-aligned first-fit allocation with coalescing.
  The heap is fixed-size; allocation failure returns NULL. Invalid/double frees
  panic. Bootstrap identity mappings remain writable/executable; this is not
  user-process isolation or full kernel W^X enforcement.
- PIC IRQ0 and IRQ1 are unmasked; PIT rate is approximately 100 Hz. IRQ handlers
  do not write the console. Exceptions print diagnostics and halt.
- Scheduler: one CPU, round robin, up to four kernel tasks including the boot
  task, one shared address space, 16 KiB stacks. Completed stacks remain reserved;
  there is no task-slot recycling, FPU/SIMD state, stack guard or user mode yet.
- VBE framebuffer and VGA fallback, PS/2 keyboard only. No USB, UEFI, storage
  filesystem, system calls or shell commands yet. Testing is on QEMU, not hardware.

See [docs/ROADMAP.md](docs/ROADMAP.md) for the original phase mapping.
The owner pushes to GitHub manually; development never automatically pushes.
