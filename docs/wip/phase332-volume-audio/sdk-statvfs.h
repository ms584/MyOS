/* LP64 layout verified against pinned Cobalt musl and statvfs ABI wrapper. */
#ifndef MYOS_SDK_SYS_STATVFS_H
#define MYOS_SDK_SYS_STATVFS_H
#include <stdint.h>
#include <stddef.h>
typedef uint64_t fsblkcnt_t;
typedef uint64_t fsfilcnt_t;
struct statvfs {
 unsigned long f_bsize,f_frsize;
 fsblkcnt_t f_blocks,f_bfree,f_bavail;
 fsfilcnt_t f_files,f_ffree,f_favail;
 unsigned long f_fsid,f_flag,f_namemax;
 int __reserved[6];
};
#define ST_RDONLY 1
#define ST_NOSUID 2
#define ST_NODEV 4
#define ST_NOEXEC 8
#define ST_SYNCHRONOUS 16
#define ST_MANDLOCK 64
#define ST_WRITE 128
#define ST_APPEND 256
#define ST_IMMUTABLE 512
#define ST_NOATIME 1024
#define ST_NODIRATIME 2048
#define ST_RELATIME 4096
#ifdef __cplusplus
extern "C" {
#endif
int statvfs(const char *,struct statvfs *);
#ifdef __cplusplus
}
static_assert(sizeof(struct statvfs)==112&&offsetof(struct statvfs,f_flag)==72&&offsetof(struct statvfs,f_namemax)==80,"target statvfs ABI");
#else
_Static_assert(sizeof(struct statvfs)==112&&offsetof(struct statvfs,f_flag)==72&&offsetof(struct statvfs,f_namemax)==80,"target statvfs ABI");
#endif
#endif
