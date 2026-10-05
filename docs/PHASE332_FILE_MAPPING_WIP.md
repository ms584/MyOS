# Phase 33.1/33.2 immutable file mapping prerequisite — PRIVATE ONLY

Latest checkpoint 2026-10-05 19:03 +07. Owner requests local commit then pause.
Main source eb31a69 and normal images unchanged; no source promotion/push/33.3.
Durable unpromoted source/test snapshot: docs/wip/phase332-file-map, excluded
from main builds. Preserve main source until mandatory regressions complete.

## Implemented in private /tmp/myos-file-map-u9NQ2N

Kernel SYS57 fixed32-byte checked request, separate from planned FS SYS56.
Real immutable asset contents are read into process-owned physical pages;
readonly MAP_SHARED has identical immutable bytes, private writable copies
never write back. Pin original runtime descriptor object and serialize offset
save/read/restore with existing I/O. Mapping survives close/reuse; no retained
kernel descriptor required because backing is immutable and fully copied.
Kernel PTE software bits10=file,11=shared preserve identity through PROT_NONE
and mprotect, partial unmap, fixed replacement. Shared write upgrades EACCES;
mixed protect/discard rejection preflighted before changing any page.
Last partial EOF-page zero padded; real bytes beyond unrounded request included
up to mapped page extent. Whole pages beyond EOF explicitly ENOTSUP (no SIGBUS
implementation); executable file maps, mutable files and MADV_DONTNEED/FREE on
file data explicitly ENOTSUP, never anonymous zeroing/fake data. No COW/mutable
shared-file coherence/full POSIX mmap claim. Required further backing/discard
semantics must be implemented when needed, not silently substituted.
Failure rolls back pages/page tables/reservations and restores source offset.
Source in desktop work: process-file-map.c/.h,syscall-file-map.c/.h,
runtime-file-map.c,vm-file-call.h; vm-area.h sparse prerequisite unchanged.

## Evidence

- Build87956 terminal0 17:34:42, normal private BIOS/UEFI, kernel238816B467sectors.
- Native16031 terminal0 17:36:36 BIOS+UEFI. Three successful launches/profile,
  readonly write fault, Escape while mapping held, relaunch, exact pages/heap.
  Asset12295B, full bytes/EOF tail, private last-page extent, source offset,
  dup/close/reused fd lifetime, shared write/protect failure, unsupported errors,
  partial hole/unmap, NOREPLACE, MAP_FIXED replacement and mixed-range rights.
  Outputs file-map-51ioy9w8, file-map-i31i3fqj. Earlier fixture compile warning
  fixed; initial page comparison counted an extra live open object, fixed to
  compare identical object sets. No kernel failure/weakening hidden by fixture.
- Real process.c ASAN/UBSAN unit17:37:37 /tmp/myos-file-map-unit-px6OpT:
  allocator failure after0..7 pages/table allocations, backing read failure
  after0..15 reads, exact pages/ledger/offset recovery; shared RO persists after
  PROT_NONE, discard rejected unchanged. Physical/heap/TLB and FS boundaries
  mocked, real VM/file-map implementation compiled. run_file_map_unit.sh.
- Genuine core/provider54169 terminal0 17:39:25, diagnostic7lrfczvv, stillFATAL.
- Trace92117 terminal0 17:42:26, diagnosticpsxt8t50: two genuine pak mmap
  requests len1318438 succeed at8020017000 and80202e9000. Backtrace no longer
  resource-load3cabb path; new core frames11d129d,11d0abf,11d0a1b,11d0c26,
  10990a5,108e5c2. Function11d1164 fetches PathService key501 via34692c0 and
  joins literal c0c540 "Experiment Config" before config load. Which branch
  fails (path get vs config decode) still unproven. Zero HTTP; exact heap125184.
  Actual HTML/JS/input/HTTPS/endurance0/5. Diagnostic28 abort thunks unchanged,
  never count as APIs. No engine patched, no assertion bypass.
- Sparse retry90043 terminal1 17:46:49, file-map full28885 terminal1 18:02:08:
  both isolated harnesses lack build/engine-evaluation/contracts/window.h.
  Failure occurs in native_ui fixture preparation; not a runtime PASS or a
  demonstrated kernel regression. Dynamic lifecycle/relocation scopes passed
  before this in sparse retry. Full suites remain incomplete; normal BIOS
  restored by test_system finally, queued final UEFI/new scopes not reached.
- Automatic approval review failed due usage limit; suggested retry18:18.
  Rejected action was not executed. Do not bypass review or assume reset.
  At this observation all known handles terminal; no live suite/process.
- Superseding observation18:57 commands available again; origin/main3cbd25f
  unchanged. All pinned contracts+core/provider copied to private trees18:59.
  New full68512 LIVE18:59:16, file-map-full-retry.log; freeze /tmp/myos-file-map-
  u9NQ2N source/tests/images and poll SAME. Full+normal restore+file/sparse native
  scopes queued; no PASS claim. Main normal kernel/runtime/BIOS/UEFI hashes
  unchanged19:02:43. No engine diagnostic live (92117 already terminal0).

## Next exact actions

1. Owner pause: on resume read Git/docs and poll SAME68512; dependencies already
   copied and clone helpers corrected. Do not restart merely from stale docs.
2. Finish complete architectural regression/new BIOS+UEFI scopes on nondiagnostic
   file-map tree, restore/check normal images; only then consider promotion.
3. Inspect PathService501 provider/11d1164 branch (get vs decoded config).
   core1225700 is just FilePath constructor, NOT provider registration;19:00
   disassembly corrects former inference.11d0f29/f60 construct config JSON.
   Pinned cobalt/base contains generated_resources_types.h only; actual source
   lives elsewhere (cobalt/common/configuration/browser). Need targeted upstream
   source and SbSystemGetPath/stat/open tracing, not resource ID501 guessing.
4. Restore/update main docs from staged PROJECT_STATE_SERVER.md/HANDOFF_SERVER.md
   and this file; local docs checkpoint. Source must exclude diagnostic thunks
   and runtime-file-map-trace.c. No fake config, codecs, handles, imported APIs.

Repro scripts build_file_map.sh,run_file_map_native.sh,run_file_map_unit.sh,
run_file_map_full.sh,run_file_map_cobalt.sh,run_file_map_trace.sh.
Engine diagnostic tree /tmp/myos-sparse-vm-BCcMw4 retains pinned135 resources.
