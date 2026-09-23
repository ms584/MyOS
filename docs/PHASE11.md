# Phase 11 — RAM filesystem

The filesystem baseline is a hierarchical RAMFS. It is volatile: reboot resets
all files. FAT32, disk persistence and block-device drivers remain future work
(storage is Phase 13). At the Phase 11 milestone the prompt only echoed input. Phase 12 now adds
`ls`, `cat`, `mkdir` and `rm`; see PHASE12.md.

## Behavior

- Initial directories: `/bin`, `/home`, `/etc`, `/dev`.
- `/etc/motd` contains `Welcome to MyOS RAMFS.` followed by a newline.
- Create/open/read/write/close files; create directories; remove closed files
  and empty directories; inspect metadata and enumerate children.
- Absolute paths only, repeated slashes and `.`/`..` supported. Root's parent
  remains root. Names allow ASCII letters, digits, underscore, hyphen and dot.
- 64 nodes including root; names at most 31 bytes, paths at most 127 bytes.
- Files at most 4096 bytes; eight open descriptors per process, numbered 3–10.
- Independent open offsets, binary data, EOF, read/write access flags,
  truncate-on-open. No seek, append flag, rename, links or executable loader yet.
- Open files cannot be removed. Exit, user fault and kill close descriptors
  immediately; process memory is reclaimed later by reap.
- Files are shared; there are no users, ownership or filesystem permissions.
  Open flags constrain each handle, not other processes' access to the node.
- Single CPU, interrupt-disabled filesystem operations; not an SMP design.

## INT 0x80 ABI extension

RAX holds the call number and signed result. Other general registers are
preserved. Calls 1–4 from Phase 10 are unchanged.

| Number | Operation | RDI | RSI | RDX | Success result |
|---|---|---|---|---|---|
| 5 | FOPEN | NUL-terminated path | flags | unused | descriptor |
| 6 | FREAD | descriptor | output buffer | length ≤256 | bytes read; 0 at EOF |
| 7 | FWRITE | descriptor | input buffer | length ≤256 | bytes written |
| 8 | FCLOSE | descriptor | unused | unused | 0 |
| 9 | MKDIR | path | unused | unused | 0 |
| 10 | UNLINK | path | unused | unused | 0 |
| 11 | READDIR | path | entry output | zero-based index | 1 entry, 0 end |
| 12 | STAT | path | entry output | unused | 0 |

FOPEN flags: READ=1, WRITE=2, CREATE=4, TRUNC=8. READ or WRITE is required;
TRUNC requires WRITE. The metadata structure is 48 bytes: name[32], uint32 type
(1=file, 2=directory), uint32 reserved=0, uint64 size. Unused name bytes are zero.
READDIR does not synthesize `.` or `..`; order is internal node order, not sorted.
An end-of-directory result leaves the output unchanged.

Errors: -2 missing, -9 bad descriptor/access, -12 out of memory, -14 bad user
pointer, -16 busy, -17 exists, -20 not directory, -21 is directory, -22 invalid
argument, -24 descriptor capacity, -28 node/file capacity, -36 name/path too
long, -39 directory not empty. Invalid requests can satisfy multiple errors;
do not depend on error precedence. Zero-length I/O does not dereference its
buffer but still requires a valid descriptor/access mode.

All user pointers are checked through every page-table level. Output requires
RW permission at every level. The whole read destination is validated before
advancing its descriptor offset; rejected cross-page writes produce no partial
file updates. Pointer overflow, kernel mappings and RX code destinations are
rejected. No unchecked user address is dereferenced in kernel mode.

## Build and acceptance

```bash
cd ~/myos
python3 tools/run_logged.py -- bash build.sh
python3 tools/run_logged.py -- python3 tests/test_system.py --suite filesystem
python3 tools/run_logged.py -- python3 tests/test_system.py
qemu-system-x86_64 -m 64M \
  -drive format=raw,file=build/myos-current.img,if=floppy -boot order=a
```

Guest kernel tests cover path resolution, binary I/O, separate offsets,
truncate/EOF, access flags, metadata/iteration, invalid paths and removal.
Full test builds additionally cover node/descriptor/file limits, heap exhaustion
and recovery, 18 Ring 3 file checks, bad/cross-page/RX pointers and automatic
descriptor closure on exit/fault/kill. The normal boot runs a user program that
opens, reads and closes `/etc/motd`, then exits and is reaped.

The full host suite also checks existing phases at 16/64/256 MiB, keyboard,
framebuffer/VGA, scheduling and expected CPU faults. It restores the normal
image after test variants. QEMU acceptance is not real-hardware validation.

## Watch development commands

In a second VS Code WSL terminal:

```bash
cd ~/myos
tail -f build/activity.log
```

The runner appends its command, working directory, time, output and exit status.
Build tracing includes compiler/linker/image commands; the Python test harness
also traces subprocess launches. This starts with Phase 11, is not a historical
record of earlier work, and is not an audit of commands run outside the runner.
Edits made by an installer are visible in `git diff`, not expanded into the log.
Ctrl+C stops this viewer only. The local log is ignored by Git. To inspect code:

```bash
git status --short
git diff
git show --stat HEAD
```

The owner always pushes manually.
