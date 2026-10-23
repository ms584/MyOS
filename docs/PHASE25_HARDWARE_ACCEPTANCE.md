# Phase 25 — selected real-hardware acceptance record

Status: **First physical boot FAIL before kernel; remaining acceptance BLOCKED/NOT RUN** (2026-10-23).
Owner photos/provenance: [physical-boot-evidence.json](wip/phase24/2026-10-23/physical-boot-evidence.json).
Next candidate and diagnostic/photo procedure: [GOP retest](PHASE24_GOP_RETEST_TH.md).
The owner attempted the USB boot; no physical USB/disk write or firmware setting change by agent.
Target: Lenovo ThinkBook 15 G3 ACL Type 21A4 selected by owner.
Inventory fields are filled; physical boot failure is recorded; other fields remain unfilled. This is not completed acceptance.
Prerequisites and detailed procedure: [PHASE24_HANDOFF_TH.md](PHASE24_HANDOFF_TH.md).
The roadmap gate is unchanged.

| Required fact | Current value / owner to fill |
|---|---|
| Machine / CPU / RAM | ThinkBook 21A4 / Ryzen 5 5500U / 8 GB (owner report) |
| Board / firmware / mode | LNVNB161216 / GQCN45WW(V1.31) / UEFI; Secure Boot OFF in owner Windows report; exact MTM still optional |
| SSD / Wi-Fi / graphics | Samsung system NVMe144d:a809 / QCA61x4A168c:003e / Radeon1002:164c; all remain unaccepted under MyOS |
| Keyboard / mouse controller IDs | Built-in PS/2 FUJ7401 / external USB30fa:0301 behind AMD xHCI1022:1639; Windows CompatibleID confirms advertised HID boot mouse03/01/02 (wip/phase24/2026-10-16/mouse-evidence.json); MyOS SET_PROTOCOL and post-handoff events untested |
| USB make/model/capacity/sector size | Samsung DUO MUF-32CB/APC; USB090c:1000;32078036992 bytes; logical/physical512B; SCSI/BOT class08/subclass06/protocol50 per owner samsung-usb-info.json; owner boot reaches MyOS loader then fails; kernel storage NOT RUN; no agent physical write |
| Boot image / source commit | See Phase 24 evidence; recheck actual tested checksum/commit |
| Image readback / firmware settings | USB readback evidence NOT PROVIDED; owner Windows report UEFI/Secure Boot OFF; no settings changed by agent |
| Allowed scratch persistence medium | Samsung DUO selected for prepared typeDA data partition; owner performed initial boot; no agent physical-write action; never the system SSD |
| Evidence directory / tester / date | Owner-supplied photos received2026-10-23; docs/wip/phase24/2026-10-23/; new-image physical retest NOT RUN |

| Acceptance item | Result | Evidence / observation |
|---|---|---|
| Owner-selected USB identity, checksum and readback | BLOCKED | Samsung name in firmware photo; exact flashed checksum/readback not provided |
| Selected USB firmware entry and loader handoff | FAIL | EFI USB Samsung entry executes MyOS loader, then unsupported framebuffer range before kernel; generic boot failed follows |
| GOP display on built-in screen | FAIL before kernel | Loader range check rejects reported framebuffer; actual base/pitch/size unknown; new mapping/diagnostics await target retest |
| Keyboard and pointer after kernel handoff | BLOCKED | PS/2 and xHCI/HID preparation identified; advertised boot-mouse protocol confirmed; actual target boot/SET_PROTOCOL/input handoff still untested |
| Settings / Calculator / Snake / Shell (kernel-resident) | NOT RUN | QEMU results are separate |
| Supported persistence save/reboot/restore | BLOCKED | Generic xHCI/BOT512 preparation verified in QEMU; actual Samsung/AMD/flush untested because owner first boot failed before kernel; further physical writes remain owner-controlled |
| Network / audio limitations recorded | NOT RUN | Native target drivers absent; inventory IDs and limitations recorded in Phase24 guide |
| Failures, recovery and limits documented | BLOCKED | First loader failure and new-image retest procedure recorded; kernel-level recovery acceptance remains untested |

Use PASS, FAIL, BLOCKED or NOT RUN and attach evidence for every tested item.
Do not mark Phase 25 complete until the selected-machine roadmap criteria
are satisfied. Keep unsupported network/audio explicit; no inherited physical
acceptance from Phase 32/33. No USB/disk/firmware action is executed by this file.
