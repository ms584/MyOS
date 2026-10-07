# Phase33.1/33.2 native image protection and near-VM WIP

2026-10-07 19:01 +07. No promotion, no33.3, no push, Cobalt gates unproven.
Clean staged sources in docs/wip/phase331-image-vm are NOT main build inputs.
The separate diagnostic-only directory must not be promoted.

## Implemented / native scope evidence

- Image mprotect stores PT_LOAD original write/execute ceilings in software
  PTE bits9/10 and image marker11. RELRO permanently clears write ceiling.
  Only owned image segments eligible: no loader/bootstrap/stack/TLS pages,
  no code writes or data execution, no partial changes on invalid mixed range.
  Read-only/NONE can be restored within original rights. Existing W^X stays.
- image-protect-TSMFT6 scope51912 terminal0 18:33:05: BIOS q2grrxcs,
  UEFI95u77jc4, faults RO-write/NONE-read/NX-execute and exact cleanup;
  full test_elf_dynamic.py BIOS also PASS, not full test_system.py proof.
- Near VM pool0x8040000000..0x8100000000 is disjoint from images/TLS/stacks,
  existing low/high pools. SYS_VM_MAP_HINT59 honors free anonymous hints,
  rounds down page, otherwise falls back; never overwrites existing pages.
  Old MAP_FIXED/NOREPLACE behavior unchanged; file hints still best-effort
  ignored. PROT_NONE reserves address space without committing RAM.
- vm-hint-PznuHf scope96474 terminal0 18:44:31 BIOS y8fbpout/UEFIz8o6ya_v
  and image protection scopes:512MiB reservation, mincore, one-page commit,
  collisions/fallback/NOREPLACE, image preservation,3relaunch/cleanup PASS.

## Actual Cobalt evidence / remaining blocker

Core/provider/resources hashes verified;242paired, no missing abort probes.
Actual32334 terminal0 diagnostic18:31:08 passed V8 ELF-data mprotect.
61429 revealed512MiB near-code hint discarded; default returned8GiB away.
88327 terminal0 diagnostic18:47:31 now honors0x80629c0000 but still fatal.
20936 terminal0 diagnostic18:56:04 startup-diagnostic-ayyb6bf7a shows
mprotect(0x80629c0000,0x20000000,7) RWX. Runtime correctly rejects by W^X;
the original assertion terminates at0x80034A738C. Controlled HTTP requests[].
This is a permission-policy conflict, not evidence of physical exhaustion.
Audit artifacts build/cobalt-v8-policy-audit-20261007, binary disassembly
build/v8-code-range-reserve-disassembly.log. Sourcepin3befd332486952f6603ae3baf2a8713878d6e200;
do not assume source flags equal prebuilt flags: strings confirms js-flags
and --jitless in the actual binary. New99662 interpreter diagnostic pending.
No skipped assertion, no fake RWX success, no host JS renderer. It must still
pass HTML/DOM/timers/input/verified concurrent HTTPS/60s/relaunch criteria.
Actual99662 terminal0 diagnostic19:02:52 startup-diagnostic-a2n4uh1_ passes
CodeRange with interpreter flag, then VizCompositorTh Check false at
immediate_crash.h193, first return0x800488CB58. HTTP[]. Inspect native GL
surface/context factory before changing it. Provider84sokh6g is real Mesa
OSMesa software renderer plus MyOS EGL core, not a hosted GL service;
its report native_provider_executed=false was packaging-time status,
not proof of Cobalt renderer operation. No WebAssembly/performance claim.
No production configuration selection or engine assertion bypass.

## Full regression / reproduction

Run commands via tools/run_logged.py. Never read activity.log through it.
Full89407 FAILED18:54:47 fault3 panic15s timeout during PROCESS_TESTS boot.
Focused45265 terminal0 exact error3/cr2 PASS14.02s; normal BIOS/UEFI restored.
Retry81590 started19:00:34: timeout60 matches existing normal boot budget;
all error/rip/cr2 assertions retained; failure logs preserved, restore trap.
Full56286 image tree and20782 VMhint tree RUNNING, sources/build frozen.
Use ps plus SAME tool sessions and terminal status/logs, not stale RUNNING.
Scripts run_image_protect_scope/full, run_vm_hint_scope/full preserve exact
configurations. Windows staging path in scripts is current host workspace;
on another host adjust staging root only and keep source/hash checks.
Mandatory architectural test_system.py must pass before clean promotion.
Normal main4 hashes clean-prerequisites-normal.sha256 must remain unchanged.
