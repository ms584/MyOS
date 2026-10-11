# Phase33.5 checkpoint21 — native virtual absolute mouse (NOT ACCEPTED)

Owner says small movement sends pointer far away. X11/warp diagnostic failed
actual input acceptance despite successful boot; retained in checkpoint20.
QEMU default PC has vmmouse/vmport. MyOS can opt into this native virtual
hardware with `mouse absolute` in the normal shell. Default remains PS/2;
`mouse relative` restores it. No host browser/player/media decoder involved.
Physical-machine pointer driver support is not implied.

New driver detects hypervisor and protocol/version, reads only bounded complete
packets, scales absolute coordinates to native display and maps buttons through
the existing event path. Unavailable/malformed devices retain PS/2 fallback.

Initial real-QEMU centre/corners/small moves/click/keyboard/PS2 return test PASS.
Absent vmport, affected BIOS/UEFI and full regression remain pending.
Normal build succeeded; source changed, prior cursor full regression cannot
establish this new build's acceptance. Owner frontend input still unverified.
Phase32/33.5/33 stay OPEN. Owner-requested WIP checkpoint0d5d09b pushed
to origin/main; remote verified. Follow-up pause receipt is committed separately.

Protocol references:
- https://github.com/qemu/qemu/blob/v10.2.1/hw/i386/vmmouse.c
- https://github.com/torvalds/linux/blob/master/drivers/input/mouse/vmmouse.c
