# Phase33.2: typed Starboard system/log services

Baseline09d1627, 2026-10-05. This dependency slice does NOT execute Cobalt or
close33.1/33.2. No33.3, no push. Origin/main09d1627 fetched05:29:09+07;
the owner already pushed the preceding paired-lifecycle checkpoint.

## Contract and implementation

The exact pinned system.h/log.h/time_zone.h are vendored verbatim under
user/starboard-system/include/starboard. Their hashes are verified against
PHASE331_ABI_SOURCES.json before the Ring3 fixture is built. Configuration and
export shims are scoped to this module, not silently added to Mesa's SDK.
user/platform-system.c is compiled against those declarations, not guessed
varargs signatures: SbLog takes priority and an already-formatted string;
Fatal priority does not terminate. SbLogRaw is allocation/lock/stdio-free.

Private syscall51 (include/system-call.h) prevalidates a whole log record and
emits it in one interrupt-disabled call on the current single-CPU target.
It copies at most256 stack bytes at a time, with no process-page allocation.
Records are capped at64KiB (longer messages retain that prefix). No formatting
is added; native UI ownership routes output to serial. This is not SMP locking.

Memory queries report managed physical-page capacity and THIS process's owned
image/runtime/TLS/page-table/anonymous pages, not global used memory. They
exclude kernel-heap bookkeeping. Locale en_US and timezone Etc/UTC are fixed
guest settings, not inherited from the host. Error text supports full-length
queries, NULL, short NUL-terminated buffers and INT_MIN; last error is TLS errno.
Stack query returns one real caller PC (up to the requested count); there is no
unwind service. Symbol lookup and extensions return unavailable without writing
output. No GPU memory-stat capability; its queries return-1/ENOTSUP.

Random uses the existing native security_random/RDRAND service, with whole
destination validation and copying only after a successful bounded entropy
request. No insecure PRNG fallback. The no-error-return Starboard APIs abort
when secure entropy is unavailable; CPUs without RDRAND cannot use this API.
Testing varying samples is functional evidence, not cryptographic validation.

Path0 is the actual /assets namespace. Paths1/2/5/8 create /home/cobalt and
cache/debug/tmp as needed under serialized RAMFS calls. Short/NULL/invalid/RX
outputs remain untouched and are validated before mkdir. A parent can remain
if later child creation fails. Executable path6 comes from the launcher's owned
read-only argv0, not a hard-coded Cobalt name; raw ELF live has no owned name.
Font/font-config/Evergreen-update paths are unavailable. These RAMFS directories
are volatile without a marked data disk; permanent-record persistence requires
the existing marked disk and fsync/snapshot mechanism. No new persistence test
or reboot guarantee is claimed by this slice.

Properties expose only truthful MyOS firmware/platform/friendly-name and
limit-ad-tracking policy. Certification/speech/ad identifiers are unavailable;
signing returns false without fabricated credentials. There is no guest debugger:
BreakIntoDebugger aborts. There is no application splash overlay; hiding it is
idempotent. No platform-error dialog exists, so RaisePlatformError returns false
and never invokes a response callback.

Network disconnection combines actual NIC carrier with existing DHCP/configured
state. The RTL8139 MediaStatus LinkDown bit is checked without changing TCP's
existing configured-state semantics. Bit mapping is verified against the
[QEMU RTL8139 emulator](https://raw.githubusercontent.com/qemu/qemu/master/hw/net/rtl8139.c).
Real hardware and actual Internet/engine requests are not established by this
QEMU carrier test.

## Evidence and outstanding validation

- BIOS system-hfd47kym and later final-system run, UEFI system-zf3rhb1e:
  typed ABI,8 workers/atomic2KiB records (3 each), Fatal returns, TLS errors,
  signal-handler raw log, buffer/path/property preservation, map/unmap accounting,
  random calls, no NIC,2 launches/exact pages+heap PASS.
- BIOS system-z2m9g3nh: NIC before DHCP disconnected; after DHCP connected;
  actual QMP-discovered NIC link down then up correctly changes query PASS.
- BIOS system-gd7xpfsl: no-RDRAND secure-random SIGABRT and no-debugger SIGABRT,
  repeated launch/reclaim PASS. Fixture uses raw syscall3 to terminate its handler.
- BIOS system-6gmkmzjg: pinned SABI launcher, owned executable asset path and
  rejection of writes into readonly argv, repeated launch/reclaim PASS.
- Final57868 exit0 05:29:28+07: paired-core/provider staging, adversarial
  getter/bias rejection and UEFI launcher/network/no-entropy all PASS.
  Actual core missing55 (22 fewer), plus two genuine getters bound.
- Full93005 exit0 06:04:28+07: ALL architectural regression of this new
  syscall/driver slice PASS (including the new typed services), normal BIOS+UEFI
  rebuilt/restored. Log build/starboard-system-full-regression.log.
  Do not reuse09d1627's earlier full PASS as evidence for these changes.
- Normal kernel252864/runtime31760 bytes. PROCESS_TESTS=1 kernel261056,
  cap262144: only1088 bytes free. Normal BSS end0x7d230<0x80000. Normal BIOS+UEFI
  restored before scoped tests. Do not increase caps or weaken W^X to fit more.

Fixture-tested imports185/242=76.4%; actual pair binds187/242=77.3%; missing55.
Unavailable optional APIs are tested unavailable contracts, not feature support.
Actual Cobalt acceptance0/5. Required next: remaining network/file and truthful
boot capability contracts, pinned content/fonts/ICU/trust resources and hierarchy,
execute actual Cobalt HTML/JS/DOM/input/verifiedHTTPS>=60s/close-relaunch.

Final normal hashes (06:04:28+07):

```text
kernel.bin f6e91a8741a8afe9dda6f973176df366d80068c9c52e611fdbcd4dec344da1e5
platform-runtime.bin eea015901c751dcf25f67956d3ffa4b4621265bf2182ff3f82e7c35ec4afc302
myos-current.img 6e0d2bf25b4a9ffb3ac7def0e58435a51c007e7faa0b5e97202ed29496414f92
myos-uefi.img 63600174676de83f33526dc6f53fca2ccef9a7ef17d0cdcb89cc81e6713c1e0d
```

## Next budget experiment (NOT integrated)

Private /tmp/myos-runtime-pack-8eolhnw6: original31760, token payload25960
(plus16-byte MRT1 header), freestanding x64 C decoder444 text/0data/0bss bytes.
Host ASan/UBSan compares exact raw bytes, CRC corruption, every single-byte
mutation/canaries, every truncation and explicit malformed boundaries/backrefs.
This is not MyOS/OOM/runtime-equivalence proof. Source/probes are in the Windows
workspace work directory (runtime-pack.{h,c}, runtime-pack-check.c,
probe_runtime_pack.py); none installed in the repo. Packaging only, no media.
Follow-up must preserve uncompressed export addresses, CRC/bounds-check before
publishing RX pages, free any bounded temporary buffer on all paths, and prove
native OOM/cancel/relaunch/byte identity. Keep current W^X/boot/TLS/caps unchanged.
Plain runtime LTO saves24 bytes, syscall-helper outline grows232: reject both,
do not spend semantic/full retests adopting these unhelpful candidates.
