# Phase 33.1/33.2: open-file unlink lifetime

RAMFS now hides an unlinked file immediately from path lookup and listing,
but retains its node/data while file handles reference it. Independent opens
and shared-descriptor dup continue I/O; the final kernel close frees data and
the node. The old name can identify a new, independent file immediately.
Unlinked children do not keep an otherwise-empty parent directory nonempty.
Snapshot export excludes orphan nodes, including its v1 size-limit check;
no on-disk format change or retained orphan after import is introduced.

SDK stdlib declarations expose the existing mkstemp/mkdtemp providers.
Actual sealed Mesa 25.0.7 anon_file.c is compiled without a behavioral patch,
using real guest environment, formatting, temporary creation, unlink,
fcntl/cloexec and ftruncate. This does NOT prove shared mmap/SCM_RIGHTS or
a complete anonymous-buffer sharing contract.

Native tests/test_unlink_sdk.py / user/elf-unlink-sdk.c:

- independent and dup handles survive unlink; stat/open fail ENOENT;
- old and new same-name data remain independent; parent/name reuse works;
- 128 orphan final-close cycles recycle the bounded node pool;
- blocked-pipe Escape cancellation retains open orphan/dup handles until
  teardown, then exact heap/pages return; four ordinary relaunches succeed;
- temporary template rejection, distinct names, zero-filled extension,
  mkdtemp lifetime, actual Mesa CLOEXEC and 128 create/close cycles;
- negative/oversize Mesa requests fail and release resources;
- disposable v1/v2 disks: fsync with an 8 KiB live orphan succeeds, named
  data survives reboot and the orphan name never returns.

Initial 93659 PASS 15:42:28. Expanded 79689 PASS 15:47:30 (+07),
build/format-evaluation/sdk-nh2mfw41. Expanded initial 50928 correctly refused
execution because the fixture omitted environment providers; corrected by
linking the tested platform-environment.c, not adding runtime stubs.
Private normal boot copied from sdk-n7zkd57d; shared full-test image untouched.

Full 62280/81645 stopped on audio disk boot 30s timeout; normal finally
restorations completed. Diagnostic 71194 PASS 15:52:59: process-test kernel
imports that disk and reaches shell in 30.74s. tests/test_audio.py now uses
the existing common 60s shell-boot budget; media assertions unchanged.
Full 83161 passed all prior framebuffer/audio tests, then video's same
30s ATA boot budget expired 16:02:30. Video now also uses common 60s wait.
Suffix 89247 started 16:05:33: original full-suite main from video onward,
including browser/release/apps/storage/memory/faults/all VGA fallback/finally.
Temporary resume wrapper omits only the already-passed prefix. Original
test_system.py SHA256 bac4de4267bfcc8dc2d0bb6f17f7eb3038121a53cb53d5331866c85bc79d0772.
No production change after the passed prefix; no assertions skipped in tail.
This is resumed aggregate coverage, NOT an uninterrupted full-run claim.
Suffix 89247 exit 0 16:33:42: every remaining test and VGA fallback passed,
normal framebuffer restored. Aggregate full coverage complete, NOT an
uninterrupted full run; production unchanged between passed prefix and tail.

Fresh normal UEFI build and tests/test_unlink_sdk.py --uefi PASS 16:34:55,
sdk-oeghfxmx: lifetime/actual Mesa/cancellation/exact reclamation checks.
Snapshot cases are separate BIOS evidence, not claimed as UEFI tests.
Enclosing 96289 reached subsequent hashes, proving test exit0, then failed
on wrong platform-runtime.so filename. Corrected .bin evidence exit0
16:36:11; no duplicate native test needed.

Normal artifacts: kernel252952/runtime29136/BIOS1474560/UEFI33554432 bytes;
BSS end0x7d2a0, PROCESS_TESTS261144 within unchanged262144 cap. SHA256:

```
kernel.bin 43b158981b925eed196ebde03e3325647236a7c14d093d48f58a081f823bed48
platform-runtime.bin f59f749181eeea1b5e71079fc0ff24c1a36c49134141b23f50da517a4823866a
myos-current.img 765a3f0c65e9746e80a14226ac2a142651ecbca1e3858cf2c5b2a61212335e5d
myos-uefi.img 827183c08dacf1e6a7f1a057e273fd71c1504d5102083972a7fcd1f1e8797fb3
```

Renderer99936 configure/boundary PASS, compile1 15:57:35,
target-config-qjsmm8xd: mkstemp cleared; abort/rand/sscanf/atexit,
OS-info/socket ENOSYS and u_qsort.cpp's unused <thread> include remain.
No native backend selected. All jobs terminal; normal images restored.

Limits: 64 RAMFS nodes, 64 KiB mutable files, 8 kernel file handles unchanged.
Directory descriptors remain path based; no full directory-inode lifetime,
stat inode/link-count/permission model or general POSIX conformance claim.
Forced cancellation reclaims resources, not guest destructor execution.
Phase 33.1/33.2 remain OPEN; Cobalt has not executed. No Phase 33.3 or push.
