# Phase 13 — ATA PIO storage and persistent MyOS RAMFS snapshots

Phase 13 adds a legacy ATA PIO driver for an IDE **primary master** data disk.
The BIOS floppy boot image remains a separate device. The driver uses IDENTIFY,
single-sector LBA28 reads/writes, cache flush and bounded polling. It accepts
only a dedicated MyOS data image with the `MYOS-DATA-v1` signature. An unknown
disk remains read-only from MyOS's perspective.

The RAMFS remains in memory while MyOS runs. On boot, a valid saved snapshot
restores its files and directories. `mkdir`, `rm`, `touch` and `write` from the
Shell save after successful changes. `sync` explicitly saves the current RAMFS,
including changes made by Ring 3 file syscalls. `disk` shows detected capacity
and the last saved generation. Without a recognized data disk, the Shell still
works in RAM and those changes disappear on reboot.

## Try it in QEMU

From the MyOS repository in WSL Ubuntu:

```bash
cd ~/myos
python3 tools/create_data_disk.py
bash build.sh
qemu-system-x86_64 -m 64M \
  -drive format=raw,file=build/myos-current.img,if=floppy \
  -drive format=raw,file=build/myos-data.img,if=ide,index=0 \
  -boot order=a
```

At `myos>`:

```text
disk
mkdir /home/demo
write /home/demo/note Hello after reboot
cat /home/demo/note
disk
```

Close QEMU and launch it again with **the same** `build/myos-data.img`. Then:

```text
cat /home/demo/note
ls /home/demo
```

The creation tool refuses to overwrite an existing image. Do not re-create it
between boots. `build/` is ignored by Git, so copy the data image elsewhere if
you need a backup. Use `python3 tools/run_logged.py -- ...` for visible command
and output logging; view `tail -f build/activity.log` in a second WSL terminal.

## Disk layout and recovery

This is a **custom snapshot format**, not FAT32, a partitioned disk or a USB
filesystem. The first sector contains the MyOS data-disk signature; MyOS never
writes it. Two snapshot banks begin at LBA 16 and 593. Each bank is 577 sectors:
one header and 64 fixed slots of nine sectors (metadata and up to 4096 bytes
per file). The final used sector is LBA 1169. The supplied image is 4096
sectors (2 MiB); the driver requires at least 2048 sectors.

Every save writes the inactive bank's data first, flushes it, then writes and
flushes a generation header. On boot, MyOS checks format, checksum and node
structure and mounts the newest valid bank. If the newest copy is damaged, it
can recover the previous generation. The checksum is for accidental corruption,
not tamper resistance. A power loss during a save may lose the latest change;
older committed data should remain available. This is a learning baseline,
not a transactional production filesystem.

The boot self-tests run against fresh RAMFS before restoring the saved snapshot,
so saved user directories cannot disrupt those checks. Data-disk detection does
not write to unknown images. ATA operations fail on timeout or device errors;
the Shell reports a failed save while current RAM contents remain available.

## Verification and limits

```bash
cd ~/myos
python3 tools/run_logged.py -- python3 tests/test_system.py --suite storage
python3 tools/run_logged.py -- python3 tests/test_system.py
```

The storage test attaches a disposable test IDE image to QEMU, writes files
through actual PS/2 Shell input, closes and reopens the VM, confirms persistence,
then damages the latest bank and checks fallback. It also attaches an unsigned
image and verifies that MyOS leaves it byte-for-byte unchanged. The full suite
regresses earlier phases at 16/64/256 MiB and both display modes, and restores
the normal framebuffer boot image. Tests use QEMU only.

The driver supports the legacy IDE primary master with LBA28 only. It does not
handle SATA/AHCI, NVMe, USB, GPT, MBR, FAT32 or arbitrary disks. Snapshots store
the existing 64 RAMFS nodes and 4 KiB file limit; the data image size does not
increase RAMFS capacity. Kernel and Shell are not yet production-hardened.
