# MyOS — Phases 22–24: UEFI, verified releases and virtual USB

MyOS now boots from BIOS into a **64-bit x86-64 kernel** with a framebuffer
console, PS/2 keyboard and mouse input, physical/virtual memory allocation, a kernel heap,
a timer-driven preemptive scheduler, isolated Ring 3 demo processes, a bounded system-call API, and a hierarchical RAM filesystem.

## Product direction

The revised goal is a MyOS graphical desktop with Settings, Calculator and
Snake, followed by online browsing and actual YouTube playback. Current code
includes Desktop 1.0 with working Settings, Calculator and Snake; browser work is planned.

- Phases 14–20: graphics, mouse, windows, Settings, Calculator, Snake and Desktop 1.0.
- Phases 21–25: BIOS/UEFI distribution, release tooling, USB and selected hardware. BIOS/UEFI images and release tooling are complete in QEMU; Phase 24 has a validated virtual USB image. Hardware preparation remains pending and Phase 25 is deferred.
- Phases 26–32: app runtime, networking, HTTPS, browser/search, audio, video and YouTube.

See [docs/ROADMAP.md](docs/ROADMAP.md) for every phase's acceptance criteria and
the mapping from the old roadmap. Phases 22–23 are complete; Phase 24 is partial and Phase 25 is deferred (the owner has no USB). Browser feasibility is a
separate long-term milestone, with no promised completion date.

The owner-approved execution order (2026-09-25) is **26 → 27 → 28 → 29 →
30 → 31 → 32 → remaining Phase 24 → Phase 25**. Phase 26 is next. Develop
and validate in QEMU first, then select the physical target and complete its
drivers and acceptance tests. Existing phase numbers and the working USB
image are retained; physical support remains unverified.

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
Focused runs use `--suite boot|memory|keyboard|exceptions|timer|processes|syscalls|filesystem|shell|storage|graphics|mouse|desktop|settings|calculator|snake|release`.
Test builds additionally verify thirteen user-fault cases, process isolation,
kill/reap, resource reuse, allocation-failure rollback, and 29 user syscall checks. Normal builds do not
run the intentionally faulting user programs.
The old `test-boot.py`, `test-vga.py`, `test-timer.py`, `test-exceptions.py` and
`build-phase22.sh` commands are compatibility entry points for the current
implementation. Earlier 16/32-bit implementations remain in Git history.

## Design and present limits

- BIOS floppy layout: boot at LBA 0, 8 KiB Stage 2 at LBA 1–16, kernel from LBA 17.
  The build derives the kernel sector count and rejects kernels above 256 KiB.
- BIOS information at 0x7000; E820 at 0x7400; font at 0xA000; VBE data at 0xB200.
  UEFI supplies allocated page tables, font, GOP framebuffer and a translated
  firmware memory map through a shared boot handoff. Kernel at 0x10000; the
  first 1 MiB is reserved by the physical allocator.
- Four-level paging initially identity-maps 4 GiB. The PMM manages only usable
  E820 pages below 256 MiB. The heap maps 4 MiB at virtual address 4 GiB with NX.
  Dynamically allocated page tables are reclaimed when mappings are removed.
- `kmalloc`/`kfree` are 16-byte-aligned first-fit allocation with coalescing.
  The heap is fixed-size; allocation failure returns NULL. Invalid/double frees
  panic. Bootstrap identity mappings remain writable/executable; this is not
  user-process isolation or full kernel W^X enforcement.
- PIC IRQ0, IRQ1 and IRQ12 (plus IRQ2 cascade) are unmasked; PIT rate is approximately 100 Hz. IRQ handlers
  do not write the console. Exceptions print diagnostics and halt.
- Scheduler: one CPU, round robin, four kernel-task slots plus four user-process
  slots. Kernel tasks share an address space; processes own CR3 roots and guarded
  one-page user stacks plus 16 KiB kernel stacks. Process resources/slots are
  reclaimed on reap; completed kernel-task stacks remain reserved. FPU/SIMD is
  unsupported, with x87 blocked using CR0.TS. Kernel stacks have no guard yet.
- BIOS VBE/VGA and native UEFI GOP, with PS/2 keyboard and mouse. Firmware can
  boot the hybrid image from virtual USB; the kernel has no native USB
  controller, HID or mass-storage driver. The Phase 13 driver supports only a dedicated legacy
  ATA data disk; testing is on QEMU, not hardware.

See [docs/ROADMAP.md](docs/ROADMAP.md) for the revised Desktop/app roadmap and original phase mapping.
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

## Phase 14 graphics

At the QEMU Shell, type `gfx` for the graphics test scene. Press R to redraw
with another color, or another key to return to the Shell. The code now has
clipped pixel/rectangle/text drawing and full-screen off-screen surfaces.
Framebuffer metadata is validated and the VGA text fallback remains usable.
See [docs/PHASE14.md](docs/PHASE14.md) for APIs, limits and acceptance tests.
Later phases add the mouse, desktop and built-in apps described below.


## Phase 15 mouse input

The PS/2 mouse now works through IRQ12. In `gfx`, movement updates a bounded
cursor, left click changes its color, and mouse and keyboard events work while
the timer preempts background tasks. At the Shell, `mouse` reports position,
buttons and queue drops. See [docs/PHASE15.md](docs/PHASE15.md) for controls,
driver limits and QEMU acceptance tests.

## Phase 16 desktop

At the QEMU Shell, type `desktop` to open the kernel-resident desktop. Click
Demo A/B/C launcher tiles, drag window title bars, click X to close, or press
1–3 to open and Q/Esc to exit. Demo A/B/C remain placeholder windows;
Settings, Calculator and Snake are separate working windows. See [docs/PHASE16.md](docs/PHASE16.md)
for controls, architecture, limits and QEMU acceptance tests.

## Phase 17 Settings

In `desktop`, open the Settings tile or press 4. Select dark/light theme,
blue/green background, and a six-digit RGB accent, then press Save. With the
recognized Phase 13 data disk attached, these values return after reboot;
without it, the status says RAM-only. The Shell command `settings` reports
committed values. See [docs/PHASE17.md](docs/PHASE17.md) for the exact QEMU
workflow, input rules, data validation and tests.

## Phase 18 Calculator

In `desktop`, open the Calculator tile or press 5. Type or click digits,
`.` and `+ - * /`, then press `=` or Enter. C clears; Backspace edits.
Arithmetic has three decimal places and reports invalid input, overflow and
division by zero. See [docs/PHASE18.md](docs/PHASE18.md) for examples,
limits and QEMU tests. The app runs inside the kernel, not a user process.

## Phase 19 Snake

In `desktop`, open Snake from the launcher or press 6. Steer with arrow
keys or W/A/S/D, pause with P or Space, and restart with R or the on-screen
button. Food grows the snake and raises the score. Best score survives
reboot when a recognized Phase 13 data disk is attached; otherwise it is
RAM-only. See [docs/PHASE19.md](docs/PHASE19.md) for controls, data limits
and QEMU validation. Snake runs in the kernel, not a user process.


## Phase 20 Desktop 1.0

The desktop top bar opens Settings, Calculator and Snake directly. The three
apps can run together; errors in one do not prevent using the others or the
Shell. A dedicated QEMU demo preserves Settings and the Snake best score on
its data image across reboot. Run `python3 tools/desktop_demo.py` from the
repository, then enter `desktop` at the `myos>` prompt. See
[docs/PHASE20.md](docs/PHASE20.md) for controls, limits and verification.
These built-in apps run inside the kernel.

## Phase 21 BIOS HDD image

`python3 tools/package_bios.py` builds `build/myos-bios.img`, a standalone
16 MiB raw BIOS hard-disk image containing Desktop 1.0. It boots as a QEMU
IDE primary master; a separate MyOS data disk on the primary slave keeps
Settings, Snake scores and Shell files across reboot. See
[docs/PHASE21.md](docs/PHASE21.md) for commands, tests and limits. The older
floppy image remains the development test target.


## Phases 22–24: UEFI and distribution

The same kernel now boots through native x86-64 UEFI with an 800x600 GOP
framebuffer and a firmware memory-map handoff. The release workflow runs the
complete system suite, BIOS/UEFI desktop and persistence checks, and boots the
same hybrid USB image under both firmware types in QEMU. It checks independent
FAT parsing, unchanged user saves and byte-identical rebuilds before packaging.

```bash
python3 tools/run_logged.py -- python3 tools/release.py
python3 tools/run_image.py --firmware uefi --media usb
```

At the Shell enter `desktop`. Add `--data build/myos-data.img` for a separate
save disk. See [release instructions](docs/RELEASE.md), [UEFI](docs/PHASE22.md),
[release tooling](docs/PHASE23.md) and [USB scope](docs/PHASE24.md).
The validated archive is `build/release/myos-desktop.zip`; BIOS/UEFI/USB images
occupy 16/32/64 MiB uncompressed. UEFI acceptance uses 128 MiB RAM in QEMU.
Secure Boot, USB HID and real-hardware acceptance are not implemented.
Phase 24 hardware-specific work remains pending; Phase 25 is deferred because
the owner has no spare USB device. No physical device is written by these tools.
