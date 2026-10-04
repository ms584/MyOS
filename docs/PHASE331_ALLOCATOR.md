# Phase33.1 native packed allocation follow-up (2026-10-04)

Tested follow-up to452faaa. This is not Cobalt/phase acceptance.

## Evidenced reason and ownership

Every old malloc consumed whole anonymous pages and searched VM_BASE first-fit.
600s native shader run timed out; CPU samples identify process_vm_map/vm_leaf.
Atomic allocation hints then exposed the real fault sooner:96-byte malloc
returned NULL at managed free pages0. Source ralloc creates many tiny objects;
QEMU512MiB cannot exceed current PMM256MiB managed cap. Not a TLS deadlock.

- user/platform-runtime.c: small objects1..992 bytes with alignment<=16 share
  anonymous RW/NX pages in64/128/256/512/1024-byte slots including32-byte headers.
  Page prefix64 bytes owns list/magic/bitmap/slot. Header requested size remains
  exact for malloc_usable_size/realloc copying. Larger/overaligned allocations
  retain the original page-based header/rounding/unmap path.
- One genuine native mutex owns all slab lists/bitmaps. No malloc recursion
  from mutex functions or slab creation. Free removes a slot and immediately
  unmaps an empty slab. Cross-thread allocations/free use process-private state.
- Main private RW/NX TLS bytes1744..1887: atomic cursor8, five list pointers40,
  reserved16, pinned opaque mutex80. Static size144. Neighboring window/event
  state1680..1743 is unchanged; state is NEVER in RX runtime code.
- Hint reservations are atomic, bounded by existing SYS_VM_MAP layout; kernel
  collision/permission/ownership checks remain authoritative. Failed hints use
  original gap search; cursor wrapping does not overwrite live regions.
- calloc now explicitly zeros requested bytes, including reused slots. No
  uninitialized slot reuse disguised as calloc. Overflow/zero-size/alignment/
  errno/posix_memalign output contracts remain as previously implemented.
- memory/process.c vm_forget swaps the last live owner into the removed slot
  and decrements vm_page_count. Ledger capacity follows peak live pages, not
  every past allocation. Cleanup still releases each live page exactly once.

## Tests and status (+07)

- Real Mesa BIOS91959/UEFI46868 PASS,2 launches each: PHASE332_NATIVE_RENDERER.md.
- tests/test_allocator_packed.py + user/elf-allocator-packed.c use installed
  runtime and genuine SDK strings, no configure archive/host libc. Private
  frozen64MiB BIOS/UEFI images. Three launches each required.
- Collision at VM_BASE, forced private cursor wrap (white-box, not public ABI),
  preserved mapping bytes/errno,512x96-byte packing consuming<64 managed pages,
  interleaved calloc reuse and untouched live objects, neighbor-state canaries,
  overflow, invalid alignment/output preservation,8..8192-byte alignment,
  eight workers/2400 mixed-size grow/shrink/zero/isolation iterations, actual
  large allocation exhaustion, failed realloc ownership and recovery.
- Scoped65329 terminal exit0 15:31:30: BIOS packed-yoexdyr8 and UEFI packed-
  9ndojdgg PASS three launches each/exact resources. Logs allocator-packed-
  native.log/allocator-packed-uefi.log in build/renderer-evaluation.
- Production architecture changed: old full93613 no longer covers this change.
  Pipeline9925 terminal exit0 16:08:50: full tests/test_system.py PASS,
  normal BIOS/UEFI restore, affected Mesa blob and lifetime BIOS/UEFI PASS. Log
  allocator-full-regression.log +allocator-normal-uefi-restore.log,
  allocator-blob-native.log/allocator-lifetime-native.log/allocator-lifetime-uefi.log.
  Ownership released. Blob native-e83rz9tb; lifetime BIOS native-l8wpahe4,
  UEFI native-oxo9w78y: OOM/finalizers/cancel/relaunch/exact resources PASS.

Normal restore16:08:50: kernel253880 bytes/runtime29904 bytes. Test-kernel
measured262072/cap262144,72-byte headroom. Full regression PASS; pinned95008
terminal exit0 16:11:52 verifies actual binary hash/staging/79 missing imports,
RELRO/reclamation and64MiB OOM rollback/shell recovery. Cobalt NOT executed.
Final normal-image hashes from terminal pipeline9925:

    kernel 00deded46c10bba7395b2069cab8f900d93683482759503b4243a471a2e294bc
    runtime ad9dc5c818b4dd933f9689a9e9a415396cee6852f36aa906b2d8ad97aa82c924
    BIOS fb1ba96497b3f8d9012fc835bdb6ac00175902fc11e140599dfc7f901e7701a1
    UEFI 12ffcf3fbff1156e3e4a5660d80de03075168d69125018758c7875f6dfc7cbad

Do not push or start33.3. Actual Cobalt0/5; imports163/242=67.4% subset only.
Next: genuine text/EGL/GLES/SABI bridge and strict native Cobalt web acceptance.
