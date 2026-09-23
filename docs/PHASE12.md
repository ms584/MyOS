# Phase 12 — Interactive shell

The boot task now runs a bounded kernel-mode command shell using PS/2 input.
It calls RAMFS directly; this is not a separate Ring 3 shell process or a POSIX
shell. Existing Ring 3 demos and timer-preempted tasks keep running.

## Commands at `myos>`

| Command | Behavior |
|---|---|
| `help` | List commands and the RAM persistence limit |
| `clear` | Clear framebuffer/VGA screen |
| `echo TEXT` | Print text and a newline |
| `pwd` | Show current directory |
| `cd [PATH]` | Change directory; no argument returns to `/` |
| `ls [PATH]` | List directory children; default current directory |
| `cat PATH` | Display file; nonprinting bytes become dots |
| `touch PATH` | Create an empty file, preserve existing contents |
| `write PATH TEXT` | Create or overwrite file with text, no added newline |
| `mkdir PATH` | Create one directory; parent must exist |
| `rm PATH` | Remove a closed file or empty directory |
| `stat PATH` | Show file/directory type and size in bytes |
| `mem` | Show available physical-page count |
| `uptime` | Approximate seconds from PIT ticks |

Example:

```text
mkdir /home/demo
cd /home/demo
write hello.txt Hello from MyOS
cat hello.txt
ls
stat hello.txt
rm hello.txt
cd ..
rm demo
```

Paths may be absolute or relative, with `.` and `..`. `pwd` shows a normalized
absolute directory. Intermediate components are checked before normalization,
so `file/..` cannot bypass the directory check. Removing the current directory
or one of its ancestors is rejected.

Commands and paths are case-sensitive. Spaces separate arguments; `write` and
`echo` use the rest of the line as literal text after leading spaces. Quotes,
escapes, variables, redirection, pipes, history, tab completion and executable
launch are not implemented. Paths cannot contain spaces. Arrow keys remain
ignored. `write PATH` writes an empty file. No shutdown/reboot command is added.

The line holds at most 255 printable characters. Backspace edits the line and
works across screen wrapping; it cannot erase the prompt. Once a line exceeds
the limit, the entire command is discarded at Enter, even if Backspace follows.
Errors return to the prompt without halting the kernel. Files remain volatile:
reboot loses changes. See PHASE11.md for RAMFS resource limits.

## Validation and watching commands

```bash
cd ~/myos
python3 tools/run_logged.py -- python3 tests/test_system.py --suite shell
python3 tools/run_logged.py -- python3 tests/test_system.py
qemu-system-x86_64 -m 64M \
  -drive format=raw,file=build/myos-current.img,if=floppy -boot order=a
```

The shell test injects actual PS/2 events through QEMU QMP and checks the command
output, file lifecycle, relative paths, argument errors, unknown commands,
Backspace, wrapped input, overlong-line rejection and cleared screen contents.
The full suite runs it in both framebuffer and VGA modes along with existing
memory/process/syscall/fault/keyboard checks. Normal build is restored afterward.

Watch live from another VS Code WSL terminal with `tail -f build/activity.log`.
The owner always pushes manually.
