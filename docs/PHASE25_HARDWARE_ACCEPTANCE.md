# Phase 25 — selected real-hardware acceptance record

Status: **NOT RUN / BLOCKED by remaining Phase 24 preparation** (2026-10-16).
Target: Lenovo ThinkBook 15 G3 ACL Type 21A4 selected by owner.
Inventory fields are filled; physical test fields remain unfilled. This is not evidence of completed tests.
Prerequisites and detailed procedure: [PHASE24_HANDOFF_TH.md](PHASE24_HANDOFF_TH.md).
The roadmap gate is unchanged.

| Required fact | Current value / owner to fill |
|---|---|
| Machine / CPU / RAM | ThinkBook 21A4 / Ryzen 5 5500U / 8 GB (owner report) |
| Board / firmware / mode | LNVNB161216 / GQCN45WW(V1.31) / UEFI; Secure Boot OFF in owner Windows report; exact MTM still optional |
| SSD / Wi-Fi / graphics | Samsung system NVMe144d:a809 / QCA61x4A168c:003e / Radeon1002:164c; all remain unaccepted under MyOS |
| Keyboard / mouse controller IDs | Built-in PS/2 FUJ7401 / external USB30fa:0301 behind AMD xHCI1022:1639; Windows CompatibleID confirms advertised HID boot mouse03/01/02 (wip/phase24/2026-10-16/mouse-evidence.json); MyOS SET_PROTOCOL and post-handoff events untested |
| USB make/model/capacity/sector size | Samsung DUO MUF-32CB/APC; USB090c:1000;32078036992 bytes; logical/physical512B; SCSI/BOT class08/subclass06/protocol50 per owner samsung-usb-info.json; MyOS physical test NOT RUN; physical write NOT authorized |
| Boot image / source commit | See Phase 24 evidence; recheck actual tested checksum/commit |
| Image readback / firmware settings | MyOS readback NOT RUN; Windows report says UEFI / Secure Boot OFF; no settings changed |
| Allowed scratch persistence medium | Samsung DUO selected for prepared typeDA data partition; physical write/boot authorization still required; never the system SSD |
| Evidence directory / tester / date | NOT RUN |

| Acceptance item | Result | Evidence / observation |
|---|---|---|
| Owner-selected USB identity, checksum and readback | NOT RUN | No physical write |
| Selected USB firmware entry and loader handoff | NOT RUN | No real boot |
| GOP display on built-in screen | NOT RUN | Mode/format/range unconfirmed |
| Keyboard and pointer after kernel handoff | BLOCKED | PS/2 and xHCI/HID preparation identified; advertised boot-mouse protocol confirmed; actual target boot/SET_PROTOCOL/input handoff still untested |
| Settings / Calculator / Snake / Shell (kernel-resident) | NOT RUN | QEMU results are separate |
| Supported persistence save/reboot/restore | BLOCKED | Generic xHCI/BOT512 preparation verified in QEMU; actual Samsung/AMD/flush behavior and physical-write authorization pending |
| Network / audio limitations recorded | NOT RUN | Native target drivers absent; inventory IDs and limitations recorded in Phase24 guide |
| Failures, recovery and limits documented | NOT RUN | Fill from physical observations |

Use PASS, FAIL, BLOCKED or NOT RUN and attach evidence for every tested item.
Do not mark Phase 25 complete until the selected-machine roadmap criteria
are satisfied. Keep unsupported network/audio explicit; no inherited physical
acceptance from Phase 32/33. No USB/disk/firmware action is executed by this file.
