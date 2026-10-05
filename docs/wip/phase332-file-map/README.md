# Unpromoted Phase33.1/33.2 file-mapping source checkpoint

These are source/test snapshots, NOT main build inputs or a finished phase.
Baseline main source eb31a69; sparse reservation/entropy and immutable file
mapping have native BIOS/UEFI and allocation/read-failure evidence in
../../PHASE332_FILE_MAPPING_WIP.md. Mandatory full regression still pending.
No diagnostic missing-call thunks or trace runtime are included in this source
snapshot. Never promote success stubs. Runtime snapshot is runtime-file-map.c.
Kernel source snapshot is process-file-map.c; protocol vm-file-call.h/SYS57.
process-file-map.h becomes include/process.h, syscall-file-map.c becomes
kernel/syscall.c, syscall-file-map.h becomes include/syscall.h. Follow project
editing constraints, inspect diffs against current source before any promotion.

Private frozen regression /tmp/myos-file-map-u9NQ2N, handle68512. Scripts retain
the original desktop staging path; use that surviving work directory, or adapt
paths in a new isolated tree, never overwrite a running source/build tree.
Proofs are copied from terminal successful runs, not the pending full suite.
Normal main image hashes remained unchanged19:02:43. Owner always pushes.
