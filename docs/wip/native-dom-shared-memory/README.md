# Phase33.1/33.2 native DOM and shared-memory WIP

These are reproducible evidence and private candidate sources, NOT production
promotion, web acceptance, verified HTTPS or Phase33.3 media completion.
See PROJECT_STATE.md and newest HANDOFF entry for live process/session handles.

- `process-shared-direct-20261008.c`: clean candidate; reserves shared VA
  without allocating a temporary private copy, tracks real shared references,
  applies protection only after installation and rolls back partial failures.
- `shm-backing-8m-20261008.h`: bounded resident8MiB, no swap/demand paging.
- `elf-shared-memory-8m-20261008.c`: real duplicate/alias coherence, rights,
  failed oversize atomicity, unlink/close/relaunch/exit resource checks.
- `shm-direct-bios/uefi.*`: scope68532 PASS at04:20:10, BIOS16/64 and UEFI64/256.
- `elf-server-capacity-20261008.c`: advertised-bound EMFILE test, keeps pending
  peer and echo/close-reuse/cancel/exact cleanup assertions; scoped59630 PASS.
- `requests-*` and `serial-*`: actual guest evidence, preserved verbatim;
  terminal shell prompt legitimately has a trailing space in serial2oau8607.
- `native-splash-h74v3taq.png`: actual QEMU final screenshot, lossless PPM→PNG;
  shows packaged native splash ONLY, not HTML-page or YouTube-video success.
- Diagnostic process/harness/task snapshot files stay OUT of production source.
  `--https-enforcement=false` is local HTTP diagnostic only; no HTTPS claim.
  Snapshot pauses/resumes QEMU and therefore is NOT a timing acceptance test.

Native core pin3befd332486952f6603ae3baf2a8713878d6e200; both actual core/provider
hashes checked by harness. Guest JS/DOM computes42 and requests timers; host
only serves the static test document and records requests. No host browser,
host JS execution, fabricated returns or engine assertion bypass.

Continue renderer unresponsive diagnosis, then genuine input/trusted concurrent
HTTPS/60s/relaunch cleanup. Full candidate regression must finish before promotion.
