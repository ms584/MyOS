# Phase 25 — selected real-hardware acceptance record

Status: **COMPLETE selected offline profile.** Readback resolved; see [closure audit](PHASE25_COMPLETION.md).

Earlier checkpoint status (superseded): **Physical boot/mouse/USB save+sync+power-cycle RAMFS/Settings/Snake restore PASS; Calculator/pointer controls PASS byowner report; Shell-file PASS; physical-media readback FAIL (boot-prefix mismatch)** (2026-10-24).
Current evidence: [desktop-controls-evidence.json](wip/phase24/2026-10-24/desktop-controls-evidence.json) and [samsung-reboot-evidence.json](wip/phase24/2026-10-24/samsung-reboot-evidence.json).
The earlier pre-kernel failure below is historical; the current retry passes that point.
Owner photos/provenance: [physical-boot-evidence.json](wip/phase24/2026-10-23/physical-boot-evidence.json).
Current save/restore procedure: [Samsung BOT retest](PHASE24_USB_BOT_RETEST_TH.md); continue same USB without reflashing.
The owner attempted the USB boot; no physical USB/disk write or firmware setting change by agent.
Target: Lenovo ThinkBook 15 G3 ACL Type 21A4 selected by owner.
Inventory fields are filled; physical first-failure and kernel/Shell retry evidence are recorded; mouse movement/backend/sync/power-cycle snapshot andSettings/Snake retention filled; Calculator/pointer controls reportedPASS; explicitShell file before/afterboot gen14 nowPASS; physicalmedia readback initially differed; exact content audit now resolves all differences. Current selected acceptance is complete per PHASE25_COMPLETION.md.
Prerequisites and detailed procedure: [PHASE24_HANDOFF_TH.md](PHASE24_HANDOFF_TH.md).
The roadmap gate is unchanged.

| Required fact | Current value / owner to fill |
|---|---|
| Machine / CPU / RAM | ThinkBook 21A4 / Ryzen 5 5500U / 8 GB (owner report) |
| Board / firmware / mode | LNVNB161216 / GQCN45WW(V1.31) / UEFI; Secure Boot OFF in owner Windows report; exact MTM still optional |
| SSD / Wi-Fi / graphics | Samsung system NVMe144d:a809 / QCA61x4A168c:003e / Radeon1002:164c; NVMe/Wi-Fi unsupported/outside profile; selected GOP display works, no accelerated GPU claim |
| Keyboard / mouse controller IDs | Built-in PS/2 FUJ7401 / external USB30fa:0301 behind AMD xHCI1022:1639; Windows CompatibleID confirms advertised HID boot mouse03/01/02 (wip/phase24/2026-10-16/mouse-evidence.json); MyOS HID initialized and1849reports; owner mouse movement confirmed; click/drag/release PASS owner report2026-10-24 |
| USB make/model/capacity/sector size | Samsung DUO MUF-32CB/APC; USB090c:1000;32078036992 bytes; logical/physical512B; SCSI/BOT class08/subclass06/protocol50 per owner samsung-usb-info.json; USB BOT32768sectors,syncgen11,owner power-cycle/reinsertion thenrestoredgen11; Settings/Best10 confirmed same; no agent physical write |
| Boot image / source commit | source6fc22f5ed84be859a8381de057b498c55b6b812c;64MiB myos-usb-data.img,SHA256d66310b71d0ecb737941bf5a6dfb22448cf728d0211456de923a06d0effa85aa; physical raw boot-prefix differs only in validated host metadata; existing boot contents match; marker/tail match |
| Image readback / firmware settings | Owner raw readback FAIL resolved by exact291byte host metadata audit; owner Windows report UEFI/Secure Boot OFF; no settings changed by agent |
| Allowed scratch persistence medium | Samsung DUO selected for prepared typeDA data partition; owner performed initial boot; no agent physical-write action; never the system SSD |
| Evidence directory / tester / date | Owner photos2026-10-23/24; latestdocs/wip/phase24/2026-10-24/; actual mouse movement,USB backend andsync confirmed |

| Acceptance item | Result | Evidence / observation |
|---|---|---|
| Owner-selected USB identity, checksum and readback | PASS content integrity, raw-prefix mismatch explained | Owner physical-usb-readback.json: correct Disk1 Samsung; boot-prefix expected65047f87... actual086eb420...; marker/tail match. All291 changedbytes decoded as disk signature and newSystemVolumeInformation metadata; existingboot content byte-identical. See PHASE25_COMPLETION.md |
| Selected USB firmware entry and loader handoff | PASS for kernel/Shell reach | Retry photo shows live kernel and Shell; owner-selected retry image context; later physical readback content audit confirms existing boot contents unchanged. Earlier loader range FAIL retained as history |
| GOP display on built-in screen | PASS for visible console | Retry photo shows800x600x32 kernel text and graphics boot checks; exact base/pitch/size still not captured; Desktop and Snake rendering observed in desktop-mouse-evidence.json |
| Keyboard and pointer after kernel handoff | PASS selectedkeyboard/mouse; click/drag/release byowner report | PS/2 and xHCI/HID preparation identified; advertised boot-mouse protocol confirmed; kernel boot confirmed; owner confirms movement2026-10-23; new photo HID30fa:0301 initialized and1849reports; typed Shell commands accepted; owner confirms requestedclick/drag/release normal |
| Settings / Calculator / Snake / Shell (kernel-resident) | PASS selected app checks; Calculator byowner report | Shell/Desktop/Settings/Snake visible; SettingsSaved to disk andSnakeBest10Disk after owner-confirmed power cycle with same prior values. Owner confirms requestedCalculator12+34=46 andclick/drag/release normal; not visible inphoto; desktop-controls-evidence.json |
| Supported persistence save/reboot/restore | PASS selected snapshot andSettings/Snake app records; explicitShell file readback PASS | syncgen11 followed byowner power-cycle/reinsertion,bootrestored RAMFSgen11,diskgen11; owner confirms sameSettings andBest10. Snapshot checks/import andapp record validation apply; samsung-reboot-evidence.json. Also phase25.txt exacttext afterboot restoredgen14 confirmed inwip/phase25/2026-10-24/shell-file-evidence.json. Not power-loss-during-write recovery acceptance |
| Network / audio limitations recorded | PASS documentation only; drivers unsupported | Native target drivers absent; inventory IDs and limitations recorded in Phase24 guide |
| Failures, recovery and limits documented | PASS recorded scope; hardware interrupted-write test NOT RUN | Loader/startup/BOT failures,corrections andowner power-cycle/reinsertion recovery documented; interrupted-write fault recovery onhardware not tested |

Use PASS, FAIL, BLOCKED or NOT RUN and attach evidence for every tested item.
Do not mark Phase 25 complete until the selected-machine roadmap criteria
are satisfied. Keep unsupported network/audio explicit; no inherited physical
acceptance from Phase 32/33. No USB/disk/firmware action is executed by this file.
