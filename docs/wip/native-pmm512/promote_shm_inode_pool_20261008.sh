#!/usr/bin/env bash
set -euo pipefail
cd /home/xytron404/myos
candidate=/home/xytron404/pmm-syscall-R4sQZZ
test "$(cat "$candidate/shm-inode-pool-full.status")" = TERMINAL_EXIT=0
sha256sum -c build/pmm-syscall-promotion-normal.sha256
cd "$candidate"
sha256sum -c shm-inode-pool-normal.sha256
cd /home/xytron404/myos
while IFS= read -r -d '' file; do
 if ! cmp -s "$file" "$candidate/$file"; then
  case "$file" in include/fs.h|fs/ramfs.c|fs/snapshot-impl.inc|fs/fs-test.c) ;;
   *) echo "Unexpected candidate input difference: $file"; exit 1;;
  esac
 fi
done < <(git ls-files -z boot kernel memory cpu user include drivers fs net audio security shell gui video tools tests build.sh)
for file in include/fs.h fs/ramfs.c fs/snapshot-impl.inc fs/fs-test.c; do
 cp "$candidate/$file" "$file"
done
printf 'RUNNING\n' >shm-inode-pool-promotion.status
trap 'result=$?; bash build.sh >shm-inode-pool-promotion-restore.log 2>&1 || result=1; python3 tools/build_uefi.py >shm-inode-pool-promotion-uefi-restore.log 2>&1 || result=1; printf "TERMINAL_EXIT=%s\n" "$result" >shm-inode-pool-promotion.status' EXIT
bash build.sh >shm-inode-pool-promotion-build.log 2>&1
python3 tools/build_uefi.py >shm-inode-pool-promotion-uefi-build.log 2>&1
sha256sum -c "$candidate/shm-inode-pool-normal.sha256"
cp "$candidate/shm-inode-pool-normal.sha256" build/shm-inode-pool-promotion-normal.sha256
cp "$candidate/shm-inode-pool-full.status" docs/wip/native-pmm512/shm-inode-pool-full.status
cp "$candidate/shm-inode-pool-normal.sha256" docs/wip/native-pmm512/shm-inode-pool-normal.sha256
{
 sha256sum "$candidate/shm-inode-pool-full.log"
 tail -1 "$candidate/shm-inode-pool-full.log"
 for firmware in bios uefi; do
  for scope in shared_memory statvfs fs_runtime_lifetime; do
   echo "SCOPE $scope $firmware"
   tail -1 "$candidate/shm-inode-pool-$scope-$firmware.log"
  done
 done
} >docs/wip/native-pmm512/shm-inode-pool-verified-summary.txt
printf 'TERMINAL_EXIT=0\n' >shm-inode-pool-promotion.status
trap - EXIT
