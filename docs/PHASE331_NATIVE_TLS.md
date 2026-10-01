# Phase 33.1/33.2 native x64 ELF TLS

2026-10-01, work from local daa0cea. Phase 33.1/33.2 remain OPEN.
This enables a measured renderer prerequisite; it is not native graphics,
dependency-loader, complete libc ABI or Cobalt acceptance.

## Contract and bounds

One eager ET_DYN module with a validated PT_TLS is supported. Module ID is 1.
GD/LD use DTPMOD64/DTPOFF64 and __tls_get_addr; IE uses TPOFF64. TLSDESC writes
two words and uses a native assembly resolver preserving all registers except
the returned RAX. These contracts follow the
[AMD64 psABI TLS relocation definitions](https://uclibc.org/docs/psABI-x86_64.pdf).
FS:0 holds the thread pointer, consistent with the
[psABI thread-pointer clarification](https://gitlab.com/x86-psABIs/x86-64-ABI/-/merge_requests/14).

TLS memory is bounded to 60 KiB; alignment is a power of two up to 4096 bytes.
First-byte alignment is preserved, including a nonzero PT_TLS virtual-address
residue. Initialized bytes must have the identical file/VA mapping in a LOAD;
TBSS needs no image-backed bytes. Defined STT_TLS offsets/sizes must fit the
module. External/undefined TLS modules, ET_EXEC TLS, larger/overaligned blocks,
dependency loading, symbol versions, IFUNC and GNU-unique remain unsupported.
Missing imports still cannot reach user code or constructors.

Ordinary relocations cannot treat a TLS offset as an image VA. TLS symbol/addend
overflow is rejected. TLSDESC validates the ENTIRE 16-byte destination against
writable LOAD bounds and protected linker metadata, including its second word.
The existing single-object eager linker and W^X rules remain in force.

GNU ld's RELRO may cover final-page padding beyond a LOAD's p_memsz. Protection
now permits only pages already mapped by one nonexecuting writable LOAD; an
unmapped tail, another segment, metadata corruption or non-page-aligned end
does not become valid. The protected pages are still genuinely read-only.

## Ownership, identity and teardown

After relocation and before constructors, the loader captures an immutable
kernel-owned template containing initialized bytes and zero TBSS. Constructors
can modify main-thread TLS without altering the template future workers use.
Main plus eight worker slots have private RW/NX pages with unmapped inter-slot
guards, outside both ELF image and anonymous mmap regions. Slots are bounded
process-owned allocations, not repeated per-thread memory leaks.

The scheduler installs the actual main/worker FS base on context switches.
Each slot has a MyOS TCB: self pointer, reserved word, TLS base and size. This
is NOT a glibc/musl pthread TCB, DTV, canary or arbitrary native-libc substitute.
Private errno/pthread/key/window/event pages retain their old identities.
Byte 48 in each private errno page records this slot's TCB for the address
provider; no-TLS processes keep it zero. Invalid module IDs/offsets return NULL
and EINVAL. Installed TLS owns FS; explicit worker FS replacement is rejected.

Workers start from the relocated pristine template, preserve data and errno
under preemption, and run existing key destructors before TLS is cleared.
Join/detach reclamation clears slots for reuse. Detached zombie kernel/user
stacks are now reaped on the next syscall by a noncurrent task, not only at
the next thread creation; current/live stacks are never reclaimed.
Fault/forced exit stops all tasks and releases template, mappings and heap.
C++ TLS initialization/destructor registration and dynamic module loading are
not newly implemented by this slice.

## Native evidence and next gate

Initial GD/IE/TLSDESC fixture passed BIOS 07:49:05 +07, including constructors,
relocated pointer templates, zero TBSS, distinct FS/errno, key destructors,
12 waves of join/detach slot reuse, exact faults, cancel/OOM and relaunch cleanup.
Expanded LD, zero-only/import-free, 4096/nonzero-residue alignment and
TLSDESC/RELRO tail tests passed 07:56:26 after the RELRO fix: twelve invalid and
three unsupported cases, repeated launch and exact page/heap restoration.
Stronger actual-address alignment assertions are in the full suite. The full
system pipeline 72357 stopped 08:11:17 at a 40s v2 reboot boot timeout. Normal
restored; diagnostic boot ready by 40.01s with persisted app status 26. Apps
now share the suite's bounded 60s boot default; storage/app assertions unchanged.
Session 55894 (08:17:07) reran apps/all later full checks and Cobalt/UEFI, and
exited 0 at 08:44:17. Stronger actual-address alignment and native UEFI TLS passed.
Earlier unchanged checks retained: resumed full coverage, not a fresh full
invocation PASS. Storage, 16/256 MiB, faults/VGA, launcher/events/UI and UEFI
DMA/frames/captured PCM/network passed. Prior daa0cea tests are not substituted.
Its initial self-test build is 261816 bytes (512 sectors), BSS end 0x7f540;
The file/BSS layout limits remain unchanged. Normal image restored: kernel
253624, runtime 29936, image 1474560 and BSS end 0x7d540.

Reproduce: `python3 tests/test_native_tls.py`, also `--uefi`; the all/apps system
suite includes this test. Source: user/elf-native-tls.c and zero-only fixture;
production: kernel/elf.c, kernel/elf-link.c, memory/process.c, cpu/scheduler.c,
user/platform-runtime.c, user/platform-tls.asm. Detailed status: PROJECT_STATE.md.

The host Mesa candidate still needs target libc/C11/C++ dependency closure,
native EGL/GLES context/present/texture/text and teardown proof. Its Linux DSO
cannot be loaded merely because these TLS relocations now exist. Actual Cobalt
HTML/JS/DOM/input/verified HTTPS >=60s plus close/relaunch remains required.
No Phase 33.3 work. No push.
