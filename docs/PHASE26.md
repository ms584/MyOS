# Phase 26 — File-based Ring 3 application runtime and browser-component feasibility

**QEMU acceptance baseline complete.** This phase supplies a freestanding C app
format, bounded user memory, file import, a v2 large-file data disk, monotonic
time ticks, and a port of one actual NetSurf browser dependency. This is an
application-runtime feasibility result, **not** a browser or a YouTube port.
Networking, HTTPS, browser UI, audio and video belong to Phases 27–32.

## Try it

```bash
cd ~/myos
python3 tools/run_logged.py -- bash build.sh
python3 tools/create_data_disk.py --large build/my-apps.img
python3 tools/run_image.py --firmware uefi --data build/my-apps.img
```

At `myos>`: `run /bin/hello`. To put a host-built app into a **new** data image,
use the exclusive-create importer, then boot with that image:

```bash
python3 tools/make_app_disk.py --large build/imported-apps.img build/wapcaplet.app wapcaplet
python3 tools/run_image.py --firmware uefi --data build/imported-apps.img
```

At `myos>`: `run /bin/wapcaplet`. It prints `NetSurf libwapcaplet Ring 3:
string interning PASS` and exits with status 26. The importer rejects an
existing target file; do not pass a save disk you want to preserve. To test a
multi-page C executable, import `build/capacity.app` as `capacity`, then run
`run /bin/capacity`; the test suite also copies it to `/home` and confirms it
still runs after a reboot. The Shell runs in kernel mode; imported apps run in
separate Ring 3 processes. Built-in desktop apps remain kernel-mode modules.

## App ABI and bounds

`MYOSAPP1` remains an 8-byte magic, little-endian 32-bit code length, and
FNV-1a checksum followed by a flat x86-64 code/rodata image. It is linked at
`0x8000000000`; `_start` must be at the first byte. Code can occupy up to 16
RX pages (64 KiB), but the 16-byte header makes maximum packaged code 65,520
bytes. The private data/BSS page is at `0x8000010000` (4 KiB, zeroed); the
guard and stack follow it, and the private 128 KiB RW/NX heap starts at
`0x8000020000`. The loader validates image size/checksum and maps noncontiguous
physical pages. The checksum detects accidental corruption, not malicious code.
`user/app-runtime.c` provides a small free-list allocator plus the C memory
and string primitives needed by the NetSurf component. `user/app-lib.h` exposes
console, 100 Hz monotonic ticks, exit, and user-heap constants; existing file
syscalls are available to apps. Scheduling, timeout (5 seconds), fault
containment and reaping continue to apply. The runtime uses integer registers;
FPU/SIMD remains deliberately unavailable and is not needed by the tested
`libwapcaplet` port. Future components that require FPU/SIMD must add safe
per-process state management before use.

This is a static flat format. There is no ELF loader, relocations, dynamic
linking, POSIX libc, threads, shared memory, or user-facing GUI API. Two
existing demo processes leave two process slots for launched apps. The
private heap is 128 KiB per loaded app. Time is monotonic tick count rather
than wall-clock time; Phase 28 must supply a reliable clock for TLS.

## Files and persistence

RAMFS files now grow from 4 KiB to at most 64 KiB, with buffers allocated
as needed. Existing 2 MiB `MYOS-DATA-v1` images retain their original 4 KiB
**persistent** file limit and exact dual-bank layout. Saving a RAMFS with a
larger file to a v1 disk fails explicitly with error 28; it does not truncate
the snapshot. `--large` creates a separate 16 MiB `MYOS-DATA-v2` image with
64 KiB per-file slots. The version is selected by the disk signature; an
unrecognized disk stays RAM-only. Both versions validate checksums and node
relationships and recover the previous valid bank. A v2 bank is 8,257 sectors,
starting at LBA 16 and 8,273. Each save rewrites the inactive bank and may
take several seconds in QEMU. Use at least 64 MiB QEMU RAM for v2: the kernel
heap expands from 4 MiB to 16 MiB when enough physical pages are present,
allowing the 4.2 MiB snapshot buffer. The 16 MiB boot baseline remains on a
4 MiB heap and v1 storage.

## Browser-component result and limits

The vendored MIT-licensed `libwapcaplet` source comes from the
[NetSurf project](https://www.netsurf-browser.org/projects/libwapcaplet/) at
Git commit `c7c128d3eb3223b216c974471f82e9337fbcf4ba`. NetSurf lists it as
one of its [browser dependencies](https://github.com/netsurf-browser/netsurf/blob/master/docs/building-Framebuffer.md).
MyOS compiles it as freestanding x86-64 C, links against the small runtime,
and exercises string interning, equality, case-insensitive comparison and
cleanup from Ring 3 in QEMU. Source and license are under `vendor/libwapcaplet/`.
This demonstrates a real browser component using app BSS, private heap and
console/exit ABI. It does **not** establish that the whole NetSurf browser
fits, or that NetSurf can play YouTube; no final browser engine is selected.
Phase 29 must evaluate a candidate against current JavaScript/DOM/media needs.
The 64 KiB file and 128 KiB app-heap ceilings will need further expansion for
a full engine, along with Phase 27 network and Phase 28 HTTPS APIs. These
limits are explicit capacity findings from this phase, not claims of browser
readiness.

## Validation

```bash
python3 tools/run_logged.py -- python3 tests/test_system.py --suite apps
python3 tools/run_logged.py -- python3 tests/test_system.py
```

Acceptance boots QEMU with real PS/2 input, checks code spanning pages,
large-file copy, persistence after reboot and corrupt-bank recovery, runs the imported NetSurf
component, and regresses small v1 disks, corrupted files, app timeout and
other system phases. QEMU success is not real-hardware or security-audit
evidence. Watch the commands with `tail -f build/activity.log` in VS Code.
The owner pushes GitHub changes manually.
