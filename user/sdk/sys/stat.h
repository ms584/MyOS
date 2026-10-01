/* Target x86-64 stat layout, matching the existing pinned-musl runtime. */
#ifndef MYOS_SDK_SYS_STAT_H
#define MYOS_SDK_SYS_STAT_H
#include <sys/types.h>
#include <time.h>
#include <stddef.h>
struct stat {
    dev_t st_dev;ino_t st_ino;nlink_t st_nlink;
    mode_t st_mode;uid_t st_uid;gid_t st_gid;unsigned __pad0;
    dev_t st_rdev;off_t st_size;blksize_t st_blksize;blkcnt_t st_blocks;
    struct timespec st_atim,st_mtim,st_ctim;
    long __unused[3];
};
#define st_atime st_atim.tv_sec
#define st_mtime st_mtim.tv_sec
#define st_ctime st_ctim.tv_sec
#define S_IFMT 0170000
#define S_IFREG 0100000
#define S_IFDIR 0040000
#define S_IFCHR 0020000
#define S_IFBLK 0060000
#define S_IFIFO 0010000
#define S_IFLNK 0120000
#define S_IFSOCK 0140000
#define S_ISREG(m) (((m)&S_IFMT)==S_IFREG)
#define S_ISDIR(m) (((m)&S_IFMT)==S_IFDIR)
#define S_ISCHR(m) (((m)&S_IFMT)==S_IFCHR)
#define S_ISBLK(m) (((m)&S_IFMT)==S_IFBLK)
#define S_ISFIFO(m) (((m)&S_IFMT)==S_IFIFO)
#define S_ISLNK(m) (((m)&S_IFMT)==S_IFLNK)
#define S_ISSOCK(m) (((m)&S_IFMT)==S_IFSOCK)
#define S_IRUSR 0400
#define S_IWUSR 0200
#define S_IXUSR 0100
#define S_IRGRP 0040
#define S_IWGRP 0020
#define S_IXGRP 0010
#define S_IROTH 0004
#define S_IWOTH 0002
#define S_IXOTH 0001
#define S_IRWXU 0700
#define S_IRWXG 0070
#define S_IRWXO 0007
#ifdef __cplusplus
extern "C" {
#endif
int stat(const char *,struct stat *);
int fstat(int,struct stat *);
int fstatat(int,const char *,struct stat *,int);
int mkdir(const char *,mode_t);
#ifdef __cplusplus
}
#define MYOS_STAT_ASSERT static_assert
#else
#define MYOS_STAT_ASSERT _Static_assert
#endif
MYOS_STAT_ASSERT(sizeof(struct stat)==144,"target stat size");
MYOS_STAT_ASSERT(offsetof(struct stat,st_mode)==24&&offsetof(struct stat,st_size)==48,"target stat fields");
MYOS_STAT_ASSERT(offsetof(struct stat,st_atim)==72&&offsetof(struct stat,st_mtim)==88&&
    offsetof(struct stat,st_ctim)==104&&offsetof(struct stat,__unused)==120,"target stat time fields");
#undef MYOS_STAT_ASSERT
#endif
