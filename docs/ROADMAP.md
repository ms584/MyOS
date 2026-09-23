# MyOS roadmap and acceptance status

The original roadmap has **19 phases, numbered 0–18**. Earlier commits named
2.1–2.3 include parts of phases 3–5. New commit names list the phases actually
covered; they are not evidence that every future extension is implemented.

| Phase | Original scope | Current acceptance status |
|---|---|---|
| 0 | Development environment | Complete: WSL, GCC, NASM, binutils, QEMU |
| 1 | Bootloader | Complete baseline: BIOS two-stage CHS loader with retries |
| 2 | Bootloader → Kernel | Complete: load a build-sized kernel and call C |
| 3 | Protected Mode / Long Mode | Complete: real → protected → 64-bit long mode |
| 4 | GDT / IDT / Interrupts | Complete baseline: 64-bit exceptions, PIC, IRQ0/IRQ1 |
| 5 | VGA / Framebuffer / Console | Complete baseline: both outputs, text/font, scrolling, input prompt |
| 6 | Keyboard Driver | Complete baseline: PS/2 IRQ1, US modifiers, bounded input buffer, echo |
| 7 | Memory Management | Complete baseline: E820 PMM, paging APIs, NX/RO mappings, kernel heap |
| 8 | Timer + Scheduler | Complete baseline: PIT, preemptive round robin, context switch, task exit |
| 9 | Process / User Mode | Complete baseline: Ring 3, private address spaces/stacks, fault containment and reap |
| 10 | System Calls | Not started |
| 11 | Filesystem | Not started |
| 12 | Shell | Not started; input echo is not a shell |
| 13 | Storage Driver | Not started |
| 14 | BIOS Boot distribution | Initial BIOS path exists; hardware/distribution work remains |
| 15 | UEFI | Not started |
| 16 | Build System | Reproducible scripts exist; distribution build work remains |
| 17 | USB Image | Not started |
| 18 | Real hardware boot | Not started |

“Complete baseline” means the original learning milestone works and has QEMU
acceptance coverage, not production OS completeness. Phase 8 uses kernel tasks;
Phase 9 now adds separate Ring 3 processes; see PHASE9.md for scope and tests. See README for resource and security
limits. Never skip these remaining phases merely because the console boots.

## Phase 8 exit checklist

- `python3 tests/test_system.py` passes with framebuffer and VGA fallback.
- Memory cases pass at 16, 64 and 256 MiB.
- Actual PS/2 key events reach the console while two CPU-bound tasks run.
- Timer switches tasks without cooperative yields, preserving integer context/DF.
- Returned tasks are not scheduled again.
- Missing/RO/NX pages produce the expected page-fault diagnostics.
- ELF is x86-64 with separate RX and RW load segments.
- Normal boot image is restored after test variants; no automatic GitHub push.
