#ifndef MYOS_FS_OPERATION_CALL_H
#define MYOS_FS_OPERATION_CALL_H
enum { FS_CALL_OPEN_AT=0, FS_CALL_STAT_AT=1, FS_CALL_RENAME_AT=2,
       FS_CALL_LIST_AT=3, FS_CALL_UNLINK_AT=4, FS_CALL_RECORD=5,
       FS_CALL_RECORD_RELEASE=6 };
/* Paths resolve from kernel-owned directory inodes while IRQs are excluded.
 * RECORD uses directory=fd, flags=pinned musl fcntl command, buffer=flock.
 * Owner and PID NEVER come from this user-controlled request. */
struct fs_operation_request {
    uint32_t operation,flags;
    int32_t directory,second_directory;
    uint64_t path,second_path,buffer,length,value;
};
_Static_assert(sizeof(struct fs_operation_request)==56,"file operation ABI");
#endif
