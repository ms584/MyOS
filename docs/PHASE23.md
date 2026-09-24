# Phase 23 — Build, validation and release tooling

`python3 tools/release.py` is the complete workflow: run the full existing
system suite, boot the packaged BIOS image, build native UEFI and the USB
image, exercise apps/persistence and both virtual USB firmware paths, validate
the FAT filesystem with independent tools, verify the prior desktop demo,
then rebuild every image and compare SHA-256 hashes. User save-image hashes
are checked before/after; no save disk is recreated by the workflow.

The release contains BIOS/UEFI/USB images, instructions, third-party license
notices, SHA256SUMS and a manifest with exact source-content hash, Git revision,
dirty-state flag, tool versions, firmware hashes, artifact sizes/hashes and
passed test names. FAT timestamps, PE timestamp and ZIP timestamps are fixed.
Byte-for-byte reproducibility is required with the same source and toolchain;
cross-toolchain output equality is not claimed.

Commands from the repo root:

```bash
python3 tools/run_logged.py -- python3 tools/release.py       # full verified release
python3 tools/release.py build                              # unvalidated build only
python3 tools/run_logged.py -- python3 tools/release.py test # validation evidence
python3 tools/release.py package                            # package matching evidence
```

Packaging refuses stale/missing evidence if sources, artifacts or toolchain
changed. Committing unchanged source content can update the recorded revision
without rerunning the same tests. Output: `build/release/myos-desktop.zip`,
`manifest.json` and `SHA256SUMS`. Generated images and user data remain ignored
by Git. Git push and publication are performed by the owner manually.

See [RELEASE.md](RELEASE.md) for dependencies and commands to boot each image.
