# Phase 33.1 — native runtime groundwork (in progress)

**NOT COMPLETE.** The first implementation slice provides read-only large
assets, restricted static ELF64 execution in Ring 3, and native inspection of
the pinned Cobalt binary. Cobalt itself has not been linked or started. The
HTML/JavaScript smoke required by [the engine assessment](PHASE33_ENGINE_EVALUATION.md)
remains open. YouTube is still not playable.

The original first-slice limits below are retained as a historical baseline.
[Phase 33.2](PHASE332.md) now adds a restricted dynamic linker and Ring 3
constructors for self-contained fixtures; it still cannot start Cobalt.

## Delivered in this slice

- [Per-import register](PHASE331_IMPORTS.csv): 243 exact imports categorized
  with [ABI contracts](PHASE331_ABI.md), binding strength and pinned source.
  The 42 inspected header hashes are preserved separately. Existing MyOS
  kernel services are not mislabeled as implemented Cobalt user imports.
- `MYOS-ASSET-v1`: a dedicated read-only asset disk, up to 32 flat named files,
  with 64-bit lengths/offsets checked against actual LBA28 disk capacity.
  Assets are streamed by bounded reads, not copied into the 64 KiB RAMFS.
- Primary IDE asset reads preserve the selected mutable data disk. The asset
  disk can be master alone, or slave alongside the existing master data disk.
  No snapshot-format migration and no asset writes are introduced.
- `elf inspect NAME`: checks ELF64/x86-64 headers, segment bounds/alignment,
  permissions and overlap; reads the dynamic-symbol section inventory from
  disk and reports each unresolved global/weak import. Sectionless files have
  no import inventory in this first inspector. This is not dynamic linking.
- `elf run NAME`: a separate static ET_EXEC path with dynamic page ownership,
  RX code, RO data, RW/NX data/BSS and a four-page NX stack with a guard.
  Partial allocation, exit, fault and timeout reclaim owned pages and tables.
  Legacy MYOSAPP1 loading retains its layout and behavior.

Execution remains deliberately restricted: at most 16 non-overlapping,
page-separated LOAD segments within the 128 MiB window beginning at
`0x8000000000`; finite available physical memory remains the practical limit.
ELF interpreters, TLS program headers, ET_DYN execution and executable stacks
are rejected. ELF fixtures use the existing MyOS INT 0x80 syscall ABI and a
standalone entry point (no argc/envp/auxv/libc startup is supplied).
No relocation processing, dynamic constructors, POSIX APIs, user thread API,
FP/SIMD context support, graphics or media bindings are added by this slice.
The five-second fixture timeout is retained pending a cancellable app lifecycle.

## Try a native ELF fixture

In the WSL repository:

```bash
cd ~/myos
python3 tools/run_logged.py -- bash build.sh
python3 tools/run_logged.py -- nasm -f elf64 user/elf-fixture.asm -o build/elf-fixture.o
python3 tools/run_logged.py -- ld -m elf_x86_64 --build-id=none \
  -T user/elf-fixture.ld -o build/elf-fixture.elf build/elf-fixture.o
python3 tools/run_logged.py -- python3 tools/make_asset_disk.py \
  build/phase331-assets.img fixture=build/elf-fixture.elf
qemu-system-x86_64 -m 64M \
  -drive format=raw,file=build/myos-current.img,if=floppy -boot order=a \
  -drive format=raw,file=build/phase331-assets.img,if=ide,index=0
```

The packer refuses to overwrite an existing file. Reuse that image or choose
a different new output name. At the MyOS prompt:

```text
assets
elf inspect fixture
elf run fixture
run /bin/hello
```

Expected: `ELF Ring 3: data, BSS and >64 KiB asset PASS`, exit status 33, then
the legacy app still exits with status 42. This fixture checks initialized
data, distant read-only bytes, and zeroed/writable multi-page BSS inside Ring 3.

To attach both disks, use the mutable MyOS data disk as IDE index 0 and the
asset disk as IDE index 1. The `assets` namespace is separate from shell `ls`.

## Inspect the real pinned Cobalt release

The previous static assessment left `build/engine-evaluation/libcobalt.so` on
this machine. It is an ignored upstream artifact, not included in Git. If
missing, obtain the exact asset URL recorded in
[PHASE33_COBALT_AUDIT.json](PHASE33_COBALT_AUDIT.json), extract `lib/libcobalt.so`,
and verify the recorded SHA256 before use. Never substitute an arbitrary build
and assume the same ABI. This preparation does not fetch YouTube media.

```bash
python3 tools/run_logged.py -- python3 tools/check_cobalt_contract.py \
  --library build/engine-evaluation/libcobalt.so
python3 tools/run_logged.py -- python3 tools/make_asset_disk.py \
  build/phase331-cobalt.img cobalt=build/engine-evaluation/libcobalt.so
```

Boot with this asset disk using the QEMU command above, then:

```text
assets
elf inspect cobalt
elf run cobalt
```

Expected inspection: 103,391,688 file bytes, four LOAD segments, 242 strong
unresolved symbols and one weak symbol. Expected execution: unsupported
runtime feature, **not executed**. This proves that MyOS can seek/read metadata
across this large file and match the host-audited import list. It does not load
the entire engine into RAM, relocate it or run a browser. 64 MiB suffices for
this inspection only and is not a Cobalt runtime RAM recommendation.

## Verification

```bash
python3 tools/run_logged.py -- python3 tools/check_cobalt_contract.py
python3 tools/run_logged.py -- python3 tests/test_elf_runtime.py
python3 tools/run_logged.py -- python3 tests/test_elf_runtime.py \
  --cobalt build/engine-evaluation/libcobalt.so
python3 tools/run_logged.py -- python3 tests/test_system.py
```

Observed 2026-09-28: focused QEMU tests passed at 16 MiB for static ELF
execution, malformed input, RX/NX faults, partial-load OOM recovery, timeout
and repeated reuse. Two-disk persistence and asset immutability passed across
reboot. Native inspection at 64 MiB matched every one of the actual pinned
binary's 243 imports. The full system suite passed and restored the normal
framebuffer image; its apps section now includes the asset/ELF regressions.
Logs: `build/phase331-regression.log`, `build/system-tests/elf-*.log` and the
normal `build/activity.log`. Tests modify only disposable images in build/.

## Next implementation gate

1. Resolve remaining renderer/dependency and ABI questions in PHASE331_ABI.md.
2. Extend the isolated loader to stage ET_DYN, apply the four observed
   relocation types, resolve real platform symbols, protect RELRO and handle
   constructors only after all essential bindings are valid. Test rollback.
3. Implement the required user VM, threads/TLS, FP/SIMD state, waits and app
   cancellation. Do not globally remove the demo timeout to simulate progress.
4. Build the MyOS Starboard layer and prove native Cobalt HTML/JavaScript/input
   smoke plus resource measurement. Only then can Phase 33.1 pass.

The actual online Rickroll video/audio/control demonstration remains the
unchanged Phase 33 gate. The owner always pushes manually.

## Runtime continuation — 2026-09-28

Per-task x87/MMX/SSE context support has been added; see
[implementation and test status](PHASE331_FP.md). This supersedes the earlier
FP/SIMD limitation only. The owner requires completion of 33.1 and 33.2
before starting 33.3. Cobalt startup remains unimplemented; neither parent
gate is being marked complete.
