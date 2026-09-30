# Phase 33.1 Starboard storage slice

The runtime implements the five storage imports present in the pinned Cobalt
export table and also exports `SbStorageWriteRecord`, which is declared by the
pinned storage contract but is not an import in that CSV.

Records are named files under `/home/.sb_<name>_<bank>`. Names are limited to
31 ASCII letters, digits, period, underscore, or hyphen; a null name selects
`default`. A process may have only one open record, consistent with the
contract's non-thread-safe single-open model. Each bank contains a fixed header
and up to 65,504 data bytes (the 65,536-byte RAMFS file cap minus the header).
Header metadata and payload are validated before a bank is selected; the newest
valid generation is used, with fallback to the previous bank if the other bank
is invalid.

Writes replace the inactive bank, write the complete record, and `fsync` before
switching the open handle. Thus a failed write leaves the older
generation intact. `SbStorageReadRecord` reads from the start each time and
returns a best-effort prefix. Close releases the handle; writes are already
committed synchronously. Delete refuses the currently open record and commits
a newer tombstone to the inactive bank before returning, so reboot does not
resurrect the older generation; deleting a tombstoned record again returns
false. Durable operation requires a persistent v2
data disk: the runtime propagates `fsync` errors instead of claiming persistence
when the backing filesystem cannot commit a snapshot.

`tests/test_elf_dynamic.py --storage-only` exercises the ABI from Ring 3 over
four boots on one disposable v2 data disk: write plus immediate read-back,
invalid name and second-open rejection, reboot persistence, durable deletion,
and tombstone state after another reboot. The complete dynamic ELF suite also
passes with this slice. This fixture does not execute Cobalt and does not close
Phase 33.1/33.2.
