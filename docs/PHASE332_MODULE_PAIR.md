# Phase33.2: actual core/provider biased staging and binding

2026-10-05 (+07), baseline4cc8b59 plus tested pair staging/binding.
Phase33.1/33.2 OPEN, no33.3/push; Cobalt actual acceptance0/5.
Runtime fixture semantics163/242=67.4%. Actual paired strong binding165/242=68.2%
leaves77 missing plus separately tracked weak TLS. Binding is not execution or
complete ABI semantics. Neither counter is overall phase completion.

## Current implementation: explicit pair-link, not a live launcher

elf pair-link CORE PROVIDER stages both sealed sources in ONE process/address
space. elf_plan_read_at records each load_bias; all dynamic/symbol/relocation/
initializer/RELRO addresses use the owning bias, not a fixed USER_BASE.
Core remains atUSER_BASE. Provider follows the actual page-rounded maximum
core LOAD end with a full unmapped separation page, bounded by existing128MiB
ELF_SPAN. No guessed96MiB layout: pinned core end0x632faa0 yields provider bias
0x0000008006331000. Parser checks address overflow, LOAD/file bounds, page
overlap, alignment, W^X and biased dynamic/RELRO/PT_TLS bounds before staging.

Process owns both images' mapped physical pages and page tables in one bounded
capacity allocation; duplicate mapping fails and releases everything. Runtime is
mapped ONCE. Provider exports are read from its actual bounded dynamic table:
exact unambiguous strong FUNC, nonempty file-backed RX range, valid visibility.
Only SbGetEglInterface/SbGetGlesInterface are eligible peer bindings; other
imports use the tested platform allowlist. No arbitrary dependency search,
interposition, DT_NEEDED/IFUNC escape, synthetic tables or hardcoded getter VAs.
Resolver's final form prioritizes these actual provider getters explicitly.

Provider relocates before core; both receive real RELRO. Missing strong core
imports still abort before ANY constructor/function-table getter/entry runs.
One native PT_TLS image is allowed explicitly; two are rejected before mapping.
The pinned core has no PT_TLS (emulated TLS); real provider initialized8/total80
bytes therefore remains native module1. Its pristine template is copied from
its own relocated biased address. This is NOT arbitrary multi-module DTV support
or proof of paired worker TLS behavior, because the pair has not executed.
Success/failure discard checks exact physical page AND live kernel-heap return.

## Current evidence and pending checks

Initial BIOS9281 exit0 02:41:09: actual pair-z36ujyp0, repeated core/provider
stage/binding/RELRO/discard and adversarial provider cases PASS.
Matrix75135 exit0 02:47:36: UEFI256 pair-sn06y2rz normal/adversarial PASS;
BIOS128OOM pair-kkd3_jc0, BIOS64OOM pair-7fxrvhzx, UEFI128OOM pair-8v9as_n6
all PASS/repeated/exact pages+heap checks/shell+app recovery. BIOS128 explicitly
observes module1 mapping begin before OOM: partial second-image rollback proved.
Actual paired core missing strong imports79->77; both getter imports no longer
missing, provider missing0, both RELRO protected. No constructors executed.

tests/test_elf_pair.py verifies actual pinned core/provider SHA, computed bias,
two real getter bindings, both RELRO gates,77 missing and no execution. Mutations
of actual provider test excessive LOAD span, weak/non-strong getter and relative
addend that crosses backward below provider bias; two native TLS images rejected.
All preserve pages, with kernel heap assertions, and leave shell/apps usable.
Disposable asset/frozen boot images and serial logs are retained per pair.

Final8994 terminal exit0 03:27:23: final resolver BIOS pair-egpl7xwj/UEFI
pair-r0rgozq0 scoped PASS, then mandatory tests/test_system.py ALL PASS and normal
BIOS restored. All jobs terminal; no live image owner. Logs:
build/renderer-evaluation/module-pair-*.log. UEFI normal image matches its scoped
frozen image; BIOS normal matches its frozen image, checked03:29:05. Old full32406
is historical pre-pair evidence; full8994 covers the current architecture.
Normal245692/runtime29904; maximum test253888/cap262144 (8256 free). Kernel ELF
entry0x10000, RX/RW LOADs only, BSS end0x7b648<0x80000. No cap, low-memory, W^X
or worker-guard relaxation. User runtime/renderer code unchanged by this slice.

Normal hashes03:29:05 (BIOS/UEFI equal final scoped frozen images):
- kernel.bin 6a3b0a74ac55f1ff5a8e2197e88e67ecc4383fc8bf991c96609f259d32cc9f3c
- platform-runtime.bin ad9dc5c818b4dd933f9689a9e9a415396cee6852f36aa906b2d8ad97aa82c924
- myos-current.img fb0b056af59b8966dfa4623c63f37d0167a6124af0bae0d1984a6fdb9ad109e1
- myos-uefi.img 95c49db87cac2d55a517fd7becf3cad5abdfbdeb2fc785e285bac8a2bc31d287

## Required next work, unchanged end-state

Use the actual pair for executable provider dependency init/fini order with
core SABI validation before any constructor. Preserve partial-init/fault/cancel
and SDK exit frame ownership; audit per-image C++ registrations/env/exit helpers
against actual engine requirements, not assumed process-global semantics.
Concrete exit audit03:11:16: provider's user/platform-exit.c runs its own
__cxa_finalize(0), then the single owned bootstrap frame's remaining fini array
(frame[0]/frame[6]) and fini(frame[5]) on the caller's stack, then syscall3 stops
all threads. It does NOT traverse a provider/core lifecycle list. Do not reuse
this unchanged for paired execution or jump a worker onto a sleeping main stack.
Core import-register/runtime-export checks have no cxa/getenv/setenv entries;
core dynamic defined-symbol check03:13:36 also found no exports for exit/_Exit/
cxa/getenv/setenv. This does not prove absence of hidden/internal implementations;
do not presume it shares provider SDK registrations/environment or exit ABI.
Prove actual provider getter/table calls, worker TLS/emutls isolation, live input,
close/relaunch/OOM and exact resources. Then remaining77 imports/weak TLS and
Cobalt HTML/JavaScript/DOM/timers/input/verifiedHTTPS >=60s acceptance.
Pair staging uses the default small stack; it is not Cobalt RAM/stack sizing.
