# Cobalt cache filename fix — private WIP, 2026-10-07

Not production source; Phase33.1/33.2 OPEN. No33.3/push/acceptance claim.
Main source eb31a69; resumed HEAD/origin c33f49a. Normal main hashes unchanged.

## Evidence and cause

Genuine pinned core/provider diagnostic32182 terminal0 11:42:39:
/home/xytron404/cd-QJYMwG/build/startup-diagnostic-k3w7c2g0/startup.txt.
SbSystemGetPath(cache/id1) succeeds; stat/mkdir Base64 leaf TEzLw== both return
-22. Live fs/ramfs.c resolve whitelist rejects '='; fs/snapshot-impl.inc repeats
the whitelist. Upstream base/base_paths_starboard.h key501 is DIR_CACHE and
PathProviderStarboard creates the app-key subdirectory. No guessed JSON fix.

The fix shares a predicate for printable ASCII0x20..0x7e except slash. Includes
space and '=' needed by actual engine; old31-byte component/127-byte absolute
path bounds, dot/dot-dot/tree validation preserved. Controls/DEL/non-ASCII
remain explicitly unsupported. No full POSIX/UTF-8 filename claim. Both live
and v1/v2 snapshot validators use identical predicate, no on-disk layout change.

## Tests and actual engine follow-up

Real RAMFS ASAN/UBSAN unit fn-M1RAJ4 terminal0 PASS11:45:50. Only privileged
IRQ/console, allocator and immutable asset boundaries substituted; actual
parser/snapshot algorithms compiled. All printable bytes, exact Cobalt names,
invalid bytes/bounds/separator, v1/v2 roundtrip with real file bytes, OOM import
rollback and exact allocations cleanup tested. PROCESS_TESTS fs-test has a
native Cobalt-names/snapshot case. Mandatory full regression is NOT PASSED yet.

Post-fix genuine engine diagnostic cache-namefix.status terminal0:
cd-QJYMwG/build/startup-diagnostic-_uszhzqh/startup.txt. Base64 mkdir returns0;
engine proceeds through Experiment Config/Metrics Config/Variations paths,
then calls unsupported statvfs abort probe and exits99. Zero HTTP requests.
heap125184->129280 must be isolated as persistent FS file data vs leak; no
exact cleanup claim. Diagnostic28 abort probes never count as APIs/success.

## Live regression / external interruption

Recovered original file-map/sparse WIP real allocator rollback/build passed.
Original full20890 interrupted by SIGHUP; detached combined full also killed
11:49:31. journal shows init.scope stop timeout killing all jobs withSIGKILL.
WSL E_UNEXPECTED/0x8007274c, restart naturally; stop initiator not identified.
No global configuration/service change performed. Old /tmp68512 unavailable.

SAME combined nondiagnostic sources restarted in persistent fm-prC903 via
foreground PTY83868 at11:52:10. combined-full.log/status plus scoped BIOS/UEFI,
sparse no-entropy, normal restore/main hashes queued. Freeze until terminal.
No full/source promotion before complete regressions. Copy only nondiagnostic
sources; never copy runtime cache trace or abort probes into production.

## Next

1. Poll exact live job; finish native/full/storage scopes and normal restore.
2. Implement/test real statvfs semantics or explicitly supported failure
   contract without invented capacity; trace actual caller/fallback.
3. Trace FS allocations and repeat engine in the same VM to distinguish cache
   data from leak. Then follow actual next blocker through all five web gates.
