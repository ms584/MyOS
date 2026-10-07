# Recovering this private shared-memory snapshot

Not a completed browser or production promotion. Current evidence/limits:
../../PHASE332_SHARED_MEMORY_WIP.md and ../../PROJECT_STATE.md.

Start from source eb31a69/main docs c33f49a, then apply nondiagnostic
phase332-file-map and phase332-file-names snapshots and phase332-volume-audio.
Do not overwrite an active test tree. Snapshot paths below are local files,
not build inputs until explicitly copied into a disposable private checkout.

| Snapshot | Destination |
|---|---|
| memory-shared-20261007.c | memory/memory.c |
| os-shared-20261007.h | include/os.h |
| shm-backing.c / shm-backing.h | fs/shm-backing.c / include/shm-backing.h |
| fs-shared-20261007.h | include/fs.h |
| ramfs-shared-20261007.c | fs/ramfs.c |
| snapshot-shared-20261007.inc | fs/snapshot-impl.inc |
| process-shared-20261007.c | memory/process.c |
| build-shared-20261007.sh | build.sh |
| elf-shared-memory-20261007.c | user/elf-shared-memory.c |
| test_shared_memory_20261007.py | tests/test_shared_memory.py |

The8-worker snapshot uses phase332-file-map/process-file-map.h as process.h.
For16 workers additionally apply phase332-thread-capacity (separate evidence),
including its clean runtime/system fixture/native guard fixture changes.
Do not combine16-worker struct layout with an8-worker process.h.

PMM ref metadata consumes128KiB kernel heap; normal files stay64KiB.
Only page-backed /dev/shm files up to1MiB support writable shared aliases.
Mapped shrink is EBUSY, not SIGBUS; ephemeral /dev/shm omitted from snapshots.
Main boot does not automatically recreate /dev/shm after snapshot import yet;
test/diagnostic creates it with real mkdir. Do not claim seamless mount setup.

Native scopes and ASAN/UBSAN described in WIP doc. New full required before
promotion; baseline fm-prC903 full excludes these changes. Never copy abort
probe assembly, tracing runtime or patched engine into production.
