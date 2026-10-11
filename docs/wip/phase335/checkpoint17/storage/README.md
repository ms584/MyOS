# Windows space reclamation — prepared, not executed

Verified guest cleanup receipt is checkpoint16/cleanup-receipt.json:
37 tar-compared source archives, net8605818880 bytes. Original archives remain
in build/phase335-cleanup-20261011; source/core/current inputs retained.
No Windows compact has run. C: free-byte changes outside a measured compact
interval cannot be attributed solely to this cleanup. fstrim coverage896.7GiB
is logical discard coverage, not returned Windows bytes.

compact-WslMyos.ps1 is scoped to the verified Ubuntu VHDX literal:
C:\Users\Xytron\AppData\Local\wsl\{87aa3900-660f-45f7-81fd-4b21dfb5ca96}\ext4.vhdx
Run the copy on Windows storage, not directly from the WSL filesystem, because
it must retain its transcript while WSL is stopped. The Windows review copy is:
C:\Users\Xytron\.codex\.chatgpt-projects\g-p-6ab0d26def24819190389687f9489350\review-phase335\compact-WslMyos.ps1

Windows administrator/UAC privileges are required. Before running, finish all
tests/builds, save WSL editor buffers and close editors that automatically
reconnect to Ubuntu. The preflight refuses active QEMU/tests/build/cleanup,
checks all5 normal hashes, syncs/trims. It does not shut down or delete files.
The Windows script then shuts down WSL (only Ubuntu installed in this machine),
aborts if Ubuntu reopens, and issues only select-vdisk/compact-vdisk/exit.
It records raw DriveInfo C: free bytes and VHDX length before/after, then reopens
Ubuntu and checks normal hashes. A failed check must not be called success.
The temporary DiskPart command file alone is removed; the transcript remains.
No format/delete/attach commands and no Windows restart are part of this script.

Command in Administrator PowerShell, after all work has finished:

```powershell
& 'C:\Users\Xytron\.codex\.chatgpt-projects\g-p-6ab0d26def24819190389687f9489350\review-phase335\compact-WslMyos.ps1'
```

Guard tested against a real running media VM (old helper) and against current
full regression (committed helper); both refuse. PowerShell syntax checked.
No end-to-end compaction result yet; do not claim returned C: bytes.
Microsoft documents that compact applies to detached/read-only dynamic VHDs:
[compact vdisk](https://learn.microsoft.com/en-us/windows-server/administration/windows-commands/compact-vdisk).
[WSL disk space](https://learn.microsoft.com/en-us/windows/wsl/disk-space)
explains how to identify the distribution VHDX; do not remove that file.
