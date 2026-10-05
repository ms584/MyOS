# Phase 33.2: bounded resident runtime packaging

Baseline: local system/log checkpoint `9838bd1`, 2026-10-05 (+07).
This is kernel payload packaging, not codecs/media or Phase 33.3.
Phase 33.1/33.2 remain OPEN; real Cobalt acceptance is still 0/5.

## Contract and implementation

`tools/pack_runtime.py` deterministically packs the existing raw runtime into
`build/platform-runtime.mrt`. Original `.bin`, ELF, exported addresses and
runtime linker assertions remain unchanged. The kernel blob contains MRT1.
16-byte header: `MRT1`, little-endian raw length, payload length and raw CRC32.
Literal tokens 0..127 encode 1..128 bytes; tokens 128..255 encode 3..130 bytes
copied from a nonzero 16-bit backward distance (overlap permitted).
The packer uses matches of at least four bytes and bounded candidate chains.

`kernel/runtime-pack.c` has no heap, globals, recursion or host dependencies.
It validates header, exact expected raw length, payload length, every input/output
bound, backward distances, complete output and CRC32. CRC detects corruption;
it is not authentication or a replacement for trusting the kernel build.

`memory/process.c` uses generated `PLATFORM_RUNTIME_BYTES`, not compressed size
or an unchecked header value. It allocates one bounded private kernel buffer,
decodes and verifies BEFORE mapping any runtime executable page, then uses the
existing RX mapping path. The buffer is freed on decode failure, page-map failure
and success; existing caller discard owns partial image/page-table rollback.
Each main/worker errno page is still private RW/NX. Kernel, boot reservations,
64 KiB runtime limit, ABI/export addresses and W^X rules are NOT enlarged.

## Evidence and current status

- Original runtime: 31760 bytes, SHA256
  `eea015901c751dcf25f67956d3ffa4b4621265bf2182ff3f82e7c35ec4afc302`.
- MRT1 is 25976 bytes including header. Normal kernel: 247072 vs 252864
  before integration (5792 bytes saved, compiler/layout-dependent).
- BIOS scoped `build/runtime-pack-scoped.log`: PASS, artifact
  `build/runtime-pack-evaluation/pack-an52e4tw`.
- ASan/UBSan decoder exact roundtrip, every single-byte mutation with canaries,
  every truncation with corrected header length, invalid distances/bounds/CRC PASS.
- Native fixture hashes every byte of mapped raw runtime independently; three
  launches return 88 with exact heap/page reclamation. Three attempted writes
  to runtime RX fault vector14/error7 at runtime base and reclaim exact resources.
- Private BIOS image CRC mutation: three launches rejected before fixture entry;
  physical pages unchanged, kernel's staged heap reclamation check intact.
  Only private copies are damaged; normal build artifacts remain untouched.
- Scoped41563 exit0 06:22:03: BIOS256 genuine-renderer/lifecycle/cancel and BIOS64
  second-image OOM PASS (`pair-exec-jpbp5nh3`, `pair-exec-pdgfk3pj`). UEFI exact
  bytes/RX faults/relaunch/resources PASS (`pack-ixzu5sdc`). Logs
  `build/runtime-pack-{pair,pair-oom,uefi}.log`.
- Host81180 exit0 06:23:05: raw-size boundaries 1/2/3/127/128/129/130/131/65535/
  65536, deterministic literal bytes and overlapping backreferences exact under
  ASan/UBSan; sampled mutation/truncation guards for these additional cases.
  Empty and 65537-byte pack input rejected. Artifact `pack-_8zazp1o`, log
  `build/runtime-pack-host-boundaries.log`. Full suite permanently runs the native
  pack fixture, exhaustive current-runtime negatives and boundary cases.
- Full61037 exit0 06:57:27: ALL architectural regression PASS, including permanent
  runtime packing tests, legacy apps/ELF/TLS/threads/network/UI/storage, fault
  variants and VGA fallback. Normal BIOS/UEFI restored and hashed.
  Log `build/runtime-pack-full-regression.log`; final UEFI build
  `build/runtime-pack-final-uefi-build.log`. No active image/source owner.
- Normal kernel247072, BSS end0x7bb90 (<0x80000); PROCESS_TESTS1 kernel255256,
  6888 bytes below the unchanged262144 loader cap. Scope is packaging only;
  no new Cobalt symbols or native web acceptance.

Final normal hashes:

```text
kernel.bin 06be60e94bdf69019a8a86481738701515b030abf7ce29bf8a46f2b69dc939a7
platform-runtime.bin eea015901c751dcf25f67956d3ffa4b4621265bf2182ff3f82e7c35ec4afc302
platform-runtime.mrt 5e6951db36daa701171e8b635ba23dceb729e4da5d67445e897758be3adcacfa
myos-current.img 43c81292f11c7ab6fbfd2fade8b13d0e893177fb811c0d3f7db6623d8d1d8080
myos-uefi.img 2438352e26eadf244f43e2df0f389cdd4fe553a3ee6b31e0162d52cc39d464f2
```

## Reproduce / finish this checkpoint

```text
python3 tests/test_runtime_pack.py
python3 tests/test_runtime_pack.py --uefi
python3 tests/test_elf_pair_execution.py --provider build/renderer-provider-evaluation/provider-84sokh6g
python3 tests/test_elf_pair_execution.py --provider build/renderer-provider-evaluation/provider-84sokh6g --ram 64
python3 tests/test_system.py
python3 tools/build_uefi.py
```

Use run_logged.py; never inspect activity.log through that logger. Do not mutate
source/shared images during tests. Record terminal results, restore/hash normal
images, update PROJECT_STATE/HANDOFF and local commit only. Owner pushes.
Next: async owner/generation-safe DNS (do not call the blocking legacy resolver
from INT80), remaining required imports, hierarchical content/ICU/fonts/trust
assets and actual Cobalt startup/web gates. No fake-success stubs or Phase 33.3.
