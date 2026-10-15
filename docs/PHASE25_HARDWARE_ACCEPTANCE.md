# Phase 25 — selected real-hardware acceptance record

Status: **NOT RUN / BLOCKED by remaining Phase 24 preparation** (2026-10-15).
Target: Lenovo ThinkBook 15 G3 ACL Type 21A4 selected by owner.
This is an unfilled handoff template, not evidence of completed tests.
Prerequisites and detailed procedure: [PHASE24_HANDOFF_TH.md](PHASE24_HANDOFF_TH.md).
The roadmap gate is unchanged.

| Required fact | Current value / owner to fill |
|---|---|
| Machine / CPU / RAM | ThinkBook 21A4 / Ryzen 5 5500U / 8 GB (owner report) |
| Board / MTM / firmware / mode | UNKNOWN |
| SSD / Wi-Fi / graphics | 256 GB, exact controller UNKNOWN / QCA61x4A / Radeon shared graphics |
| Keyboard / mouse controller IDs | Built-in keyboard + separate mouse; transport/IDs UNKNOWN |
| USB make/model/capacity/sector size | UNKNOWN; physical write NOT authorized in this task |
| Boot image / source commit | See Phase 24 evidence; recheck actual tested checksum/commit |
| Image readback / firmware settings | NOT RUN / UNKNOWN |
| Allowed scratch persistence medium | NONE SELECTED; do not use the system SSD |
| Evidence directory / tester / date | NOT RUN |

| Acceptance item | Result | Evidence / observation |
|---|---|---|
| Owner-selected USB identity, checksum and readback | NOT RUN | No physical write |
| Selected USB firmware entry and loader handoff | NOT RUN | No real boot |
| GOP display on built-in screen | NOT RUN | Mode/format/range unconfirmed |
| Keyboard and pointer after kernel handoff | BLOCKED | Confirm transports; prepare required drivers |
| Settings / Calculator / Snake / Shell | NOT RUN | QEMU results are separate |
| Supported persistence save/reboot/restore | BLOCKED | No target storage driver/authorized medium |
| Network / audio limitations recorded | NOT RUN | Native target drivers absent |
| Failures, recovery and limits documented | NOT RUN | Fill from physical observations |

Use PASS, FAIL, BLOCKED or NOT RUN and attach evidence for every tested item.
Do not mark Phase 25 complete until the selected-machine roadmap criteria
are satisfied. Keep unsupported network/audio explicit; no inherited physical
acceptance from Phase 32/33. No USB/disk/firmware action is executed by this file.
