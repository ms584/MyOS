# MyOS — Phase 13: ATA Storage and Persistent RAMFS

MyOS now boots from BIOS into a **64-bit x86-64 kernel** with a framebuffer
console, PS/2 keyboard input, physical/virtual memory allocation, a kernel heap,
a timer-driven preemptive scheduler, isolated Ring 3 demo processes, a bounded system-call API, and a hierarchical RAM filesystem.

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
an **interactive command shell**; type `help` to list commands. Two CPU-bound kernel
tasks run in the background without voluntarily yielding; the timer preempts
them. A third task returns and is removed from the runnable set. Two Ring 3 processes
also run with separate page tables, user stacks and private memory.
A third user demo prints through `sys_write`, gets its PID, yields and exits
with status 42; the kernel reclaims it. See [docs/PHASE10.md](docs/PHASE10.md)
for the syscall ABI and [docs/PHASE9.md](docs/PHASE9.md) for process foundations.

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
Focused runs use `--suite boot|memory|keyboard|exceptions|timer|processes|syscalls|filesystem|shell|storage`.
Test builds additionally verify thirteen user-fault cases, process isolation,
kill/reap, resource reuse, allocation-failure rollback, and 29 user syscall checks. Normal builds do not
run the intentionally faulting user programs.
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
- Scheduler: one CPU, round robin, four kernel-task slots plus four user-process
  slots. Kernel tasks share an address space; processes own CR3 roots and guarded
  one-page user stacks plus 16 KiB kernel stacks. Process resources/slots are
  reclaimed on reap; completed kernel-task stacks remain reserved. FPU/SIMD is
  unsupported, with x87 blocked using CR0.TS. Kernel stacks have no guard yet.
- VBE framebuffer and VGA fallback, PS/2 keyboard only. No USB or UEFI yet. The Phase 13 driver supports only a dedicated legacy
  ATA data disk; testing is on QEMU, not hardware.

See [docs/ROADMAP.md](docs/ROADMAP.md) for the original phase mapping.
Phase 9 adds supervisor-only kernel mappings to each process; this is not a
security-audited sandbox. User-program loading is currently an embedded demo,
with INT 0x80 calls for console/process operations and RAM files.

The owner pushes to GitHub manually; development never automatically pushes.

## Phase 11 and visible commands

RAMFS supports directories, binary file I/O, metadata and directory iteration.
Each process owns eight file handles; exit/fault/kill closes them. Files live in RAM and disappear on reboot unless saved to a dedicated MyOS
ATA data disk. A Ring 3 demo reads `/etc/motd` at boot.
See [docs/PHASE11.md](docs/PHASE11.md) for limits, ABI and acceptance tests.
The `myos>` prompt now accepts interactive file commands; see [Phase 12](docs/PHASE12.md).

To watch development commands/results in your VS Code WSL terminal:

```bash
cd ~/myos
tail -f build/activity.log
```

To record your own build or tests, use `python3 tools/run_logged.py -- bash build.sh`
or `python3 tools/run_logged.py -- python3 tests/test_system.py`.
Only commands run through this wrapper are recorded; edits are reviewed with
`git diff`. The log begins with Phase 11 and stays local under ignored `build/`.

## Phase 12 shell

Type `help` in QEMU. Try `ls`, `cat /etc/motd`, `mkdir /home/demo`,
`cd /home/demo`, `write note Hello MyOS`, `cat note`, `pwd`, `mem`, and `uptime`.
The shell runs in kernel mode; there is no external executable launcher yet.
See [docs/PHASE12.md](docs/PHASE12.md) for syntax, limits and test coverage.

## Phase 13 persistent data disk

Create a dedicated MyOS data image once with `python3 tools/create_data_disk.py`,
then boot QEMU with both `build/myos-current.img` as floppy and
`build/myos-data.img` as IDE primary master. The Shell saves after file changes;
`disk` shows storage state and `sync` forces a save. Reuse the same data image
on the next boot. See [docs/PHASE13.md](docs/PHASE13.md) for the exact command,
disk layout, validation and limits. The format is specific to MyOS, not FAT32.
