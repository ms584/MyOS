#ifndef MYOS_SDK_SYS_TYPES_H
#define MYOS_SDK_SYS_TYPES_H
#include <stddef.h>
#include <stdint.h>
/* Audited LP64 runtime argument/return widths; not a complete POSIX header. */
typedef int64_t ssize_t;
typedef int64_t off_t;
typedef uint32_t mode_t;
typedef int32_t pid_t;
typedef uint32_t uid_t;
typedef uint32_t gid_t;
typedef uint64_t dev_t;
typedef uint64_t ino_t;
typedef uint64_t nlink_t;
typedef int64_t blksize_t;
typedef int64_t blkcnt_t;
#endif
