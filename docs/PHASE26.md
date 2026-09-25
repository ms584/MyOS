# Phase 26 — File-based Ring 3 app runtime (first baseline)

Phase 26 is **in progress**. The current milestone loads a bounded native
x86-64 executable from the MyOS filesystem into a private Ring 3 process.
It establishes a small freestanding C port and a real file import workflow.
It does not yet satisfy the browser-capacity or browser-feasibility gate in
the roadmap.

## Try it

From the WSL repo:

```bash
cd ~/myos
python3 tools/run_logged.py -- bash build.sh
python3 tools/run_image.py --firmware uefi
```

At `myos>` type:

```text
run /bin/hello
cp /bin/hello /home/hello
run /home/hello
```

The C app writes `Hello from /bin/hello (C, Ring 3)` and exits with status 42.
`cp` copies binary files without changing their bytes; the copy survives reboot
when a separate MyOS data disk is attached. To import a separately compiled
C app, create a **new** dedicated data image (the command refuses to replace
an existing file):

```bash
python3 tools/make_app_disk.py build/my-port-data.img build/port.app port
python3 tools/run_image.py --firmware uefi --data build/my-port-data.img
```

Then run `run /bin/port`. It uses the console and 100 Hz tick services and
exits with status 7. The host tool uses the existing dual-snapshot data-disk
format. Never use a save disk you wish to preserve as its output path.

## Format and limits

App v1 is a 16-byte `MYOSAPP1` header (8-byte magic, little-endian 32-bit
code length and FNV-1a checksum) followed by one flat x86-64 code/rodata
image linked at `0x8000000000`. `tools/pack_app.py` produces the package.
The checksum detects accidental corruption; it does not authenticate code.
Each app has one 4 KiB RX code page, one private 4 KiB RW/NX data page, a
4 KiB RW/NX stack with an unmapped guard, and a 16 KiB kernel stack. Two
existing demo processes leave two process slots for launched apps. The Shell
waits up to five seconds for an app, then kills/reaps it and returns an error.
Faults stop only the app. INT 0x80 service 13 returns the current PIT tick
count; a small `user/app-lib.h` provides C wrappers for console, time and exit.
The apps use integer registers only. x87/SIMD/FPU state is still unsupported.
Built-in desktop apps and the Shell still execute in kernel mode.

This is not ELF loading, dynamic linking, a general C runtime or a browser
port. The RAMFS file limit is still 4 KiB and the kernel heap is 4 MiB; neither
is adequate for a modern browser. Phase 26 still requires a capacity plan,
larger executable/address-space support, safe FPU/SIMD context management if
the chosen port needs it, and a documented browser-engine feasibility result.
No browser engine has been selected yet.

## Validation

`python3 tools/run_logged.py -- python3 tests/test_system.py --suite apps`
and the full system suite boot QEMU with real PS/2 input. Acceptance checks
load by path, C syscalls and exit, binary copy, corruption rejection, copied
file persistence, host-imported app, and timeout/reaping of a CPU-bound app.
The tests also verify another app still runs after timeout. QEMU success does
not establish real-hardware or security-audited support.
