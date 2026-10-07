# Phase33.1/33.2 — pinned uname ABI repair (private WIP)

## Evidence and cause

Actual Cobalt core27.lts.3, source3befd332486952f6603ae3baf2a8713878d6e200,
remains unchanged. `startup-diagnostic-jzptp0v7` terminal0 at13:04:00 is
diagnostic completion only: zero controlled HTTP requests, NOT browser PASS.
After12 workers and262144/4-byte shared mappings, #GP at0x80035A9FDA.
Checked saved frame caller0x800346A5D6. The actual core disassembly shows
0x346a5b3 branches to0x346a5d1 on FS:0x28 stack-canary mismatch, then calls
the HLT crash routine. Do not bypass the stack check or skip HLT.

The caller allocates/memsets1285 bytes (0x505) for `uname`. The pinned
`third_party/musl/include/sys/utsname.h`, `STARBOARD` branch, has precisely
five257-byte arrays, no domainname. Main/runtime mistakenly declares six
257-byte arrays and clears/writes1542 bytes: a257-byte overwrite. Therefore
this is a real existing-runtime ABI bug, not evidence to disable protection.
Pinned source cached at main/build/cobalt-config-audit-20261007/
third_party-musl-include-sys-utsname.h. `base/system/sys_info_posix.cc`
OperatingSystemVersion uses the same structure and uname call.

## Clean correction and proof plan

Private clean runtime removes domainname and asserts sizeof==1285. Diagnostic
runtime has the same correction, but diagnostic wrappers/abort thunks must
NEVER be promoted. No new import count is claimed for this existing API.
Clean system fixture uses independent five-field definition, size assertion,
16-byte prefix and300-byte suffix sentinels, field contents and NULL/EFAULT.
Its old hardcoded8-thread bounds are corrected to actual17 threads/68 user
tasks, already separately saturated/tested by the capacity fixture.

Initial scope returned4 due to the stale sysconf8 fixture before reaching
uname; retained as uname-abi-old-bound.log, NOT PASS. Corrected session80634
terminal0 at13:15:51, script run_uname_abi_20261007.sh, private cd-QJYMwG.
BIOS16/64 and UEFI64/256 four relaunches/exact resources PASS; outputs
uname-abi-6wmffxpj and uname-abi-zqyjhl4z. Genuine startup9ciyx3qt no longer
hits the canary failure and reaches actual Local Storage/leveldb. It creates
all16 workers, then another creation returns EAGAIN; ImmediateCrash follows.
Zero HTTP; heap293120->309504 includes new persistent RAMFS files vs leak
not isolated. Diagnostic terminal0 is NOT actual-browser acceptance PASS.

## Next capacity scope (separate proof)

The proven16-worker snapshot remains docs/wip/phase332-thread-capacity.
New capacity32 is a separate unproven batch:32 workers,33 total threads,
132 per-UID user tasks across4 processes. Runtime/native TLS guard separation
moves native base to runtime+0x40000, with compile-time overlap assertion.
System fixture now references the shared bound rather than hardcoded values.
Session51522 started13:17:38; capacity32.status and capacity32*.log in private
cd-QJYMwG. At13:22 BIOS16/64 and UEFI64/256 saturation/reuse/exact resources
PASS (thread-capacity-_lzhkz2k, thread-capacity-23c_yh9k). Native TLS/guard/
cancel and thread services/teardown also PASS. Guarded system/uname scopes
PASS BIOS16/64 (uname-abi-ahcy8yhw), UEFI64/256 (uname-abi-su2smycf).
Session51522 terminal0 13:22:57. Actual engine p2388a2x reaches Code Cache
then calls missing rename (diagnostic abort99, not a working rename). LevelDB
fcntl command6 (F_SETLK) returns-EINVAL repeatedly. Zero HTTP. Heap325888->
354560 includes new cache/RAMFS files versus leak not isolated. No actual
web acceptance PASS. Diagnostic-only fcntl command/result logging identifies
this; it is not in clean runtime. Do not reuse16-worker PASS as32 proof.
Nondiagnostic recovery input snapshots:
docs/wip/phase332-uname-capacity32 (not part of main builds).

New clean full/scopes session93459 LIVE13:23:30, shared-clean-full.status /
shared-clean*.log in cd-QJYMwG. Clean runtime, generator, process source and
main system module restored; build asserts no MyOSMissingImport symbols.
Mandatory full remains PENDING; earlier fm-prC903 full excludes this batch.
Freeze cd sources/builds until93459 terminal. Main source/images unchanged.
Next real imports: merge inode identity prerequisite with filename/volume/
shared backing, move DIR APIs to pinned inode operations, then true rename;
implement record locks for F_SETLK, not a no-op success. Remaining FS10 and
player/decode15 require contracts/testing. Diagnostic thunks NEVER APIs.
Phase33.1/33.2 remain OPEN; no33.3, no push, no YouTube claim.
