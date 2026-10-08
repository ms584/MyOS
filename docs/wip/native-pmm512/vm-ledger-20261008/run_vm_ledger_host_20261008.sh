#!/usr/bin/env bash
set -euo pipefail
cd /home/xytron404/myos
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
out=$(mktemp -d build/vm-ledger-host-XXXXXX)
cp "$stage/vm-ledger-20261008.h" "$out/vm-ledger.h"
gcc -std=c11 -O2 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Wall -Wextra -Werror -Iinclude -I"$out" "$stage/vm-ledger-unit-20261008.c" -o "$out/ledger-unit"
ASAN_OPTIONS=detect_leaks=1 "$out/ledger-unit"
printf 'EVIDENCE %s\n' "$out"
