# Phase 22 — Native UEFI boot

The x86-64 GNU-EFI loader selects 800x600 BGR GOP graphics, reserves the
kernel region, allocates page tables and handoff data through firmware, and
passes a fresh memory map after ExitBootServices to the **same kernel.bin**
used by BIOS. No BIOS interrupts are called by the UEFI path. The kernel now
receives its page-table root, stack, font and framebuffer from a shared boot
ABI. UEFI loader allocations and all non-conventional firmware regions remain
reserved in the physical allocator. ExitBootServices retries a stale map key
without allocating more memory. Failure to reserve required memory stops the
loader before it overwrites any firmware-owned region.

```bash
cd ~/myos
python3 tools/build_uefi.py
python3 tools/run_image.py --firmware uefi --data build/myos-data.img
```

At `myos>`, enter `desktop`. Settings, Calculator and Snake have the same
controls as BIOS. A separate IDE data image restores Settings and Snake score
after reboot. OVMF variables are copied to `build/run-uefi-vars.fd`; the system
firmware template is not modified. The built-in 8x16 MyOS bitmap font derives
from DejaVu Sans Mono; its source license is included under `assets/`.

Acceptance uses OVMF with 128 MiB RAM, QEMU PS/2 input and 800x600 GOP. Tests
cover PMM exhaustion/recovery, page tables, preemption, Ring 3 demos, the three
apps, screenshot pixels and ATA persistence/reboot. The loader requires NX,
a framebuffer below 4 GiB and an available physical range 0x10000–0x7ffff.
It reserves that range explicitly. Other layouts/modes are not accepted yet;
this is not a general UEFI hardware claim. Secure Boot signing is not included.

Boot-service ownership and handoff follow the [UEFI Boot Services specification](https://uefi.org/specs/UEFI/2.10/07_Services_Boot_Services.html).
The firmware provides USB input only before ExitBootServices; the kernel still
uses its PS/2 drivers afterward.
