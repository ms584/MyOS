#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
cd /tmp/myos-file-map-u9NQ2N
cp "$stage/elf-file-map.c" user/
cp "$stage/test_file_map.py" tests/
python3 tests/test_file_map.py
python3 tests/test_file_map.py --uefi
