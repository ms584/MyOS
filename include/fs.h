#ifndef MYOS_FS_H
#define MYOS_FS_H
#include "asset.h"
#include "fs-volume-call.h"
#include "record-lock.h"
#define FS_NODES 64
#define FS_HANDLES 32
#define FS_PATH 128
#define FS_NAME 32
/* Bounded printable ASCII components, not a full POSIX/UTF-8 namespace.
 * Spaces and punctuation are required by real Cobalt cache/config paths.
 * Slash separates components; controls/DEL/non-ASCII remain rejected. */
static inline bool fs_name_byte(unsigned char c)
{return c>=0x20&&c<=0x7e&&c!='/';}
#define FS_FILE_MAX 65536
enum { FS_READ=1, FS_WRITE=2, FS_CREATE=4, FS_TRUNC=8, FS_EXCL=16, FS_DIRECTORY=32,
       FS_AUTO_DIRECTORY=64 };
#define FS_AT_FDCWD (-100)
enum { FS_FILE=1, FS_DIR=2 };
struct fs_handle { unsigned node, flags; size_t offset; struct asset_file asset; };
struct fs_entry { char name[FS_NAME]; uint32_t type, reserved; uint64_t size; };
void fs_init(void);
int fs_mkdir(const char *);
int fs_unlink(const char *);
/* remove_directory: 0=file only, 1=directory only. */
int fs_unlink_at(struct fs_handle *,int,const char *,unsigned);
int fs_stat(const char *,struct fs_entry *);
int fs_volume_stat(const char *,struct fs_volume_info *);
int fs_list(const char *,unsigned,struct fs_entry *);
int fs_open(struct fs_handle *,const char *,unsigned);
int fs_open_at(struct fs_handle *,int,const char *,unsigned);
int fs_stat_at(struct fs_handle *,int,const char *,struct fs_entry *);
int fs_list_at(struct fs_handle *,int,const char *,unsigned,struct fs_entry *);
int fs_rename_at(struct fs_handle *,int,const char *,int,const char *);
int fs_rename(const char *,const char *);
int fs_record_lock(struct fs_handle *,int,int32_t,unsigned,struct record_lock_abi *);
int fs_record_release(struct fs_handle *,int);
int fs_close(struct fs_handle *,int);
void fs_close_all(struct fs_handle *);
int64_t fs_read(struct fs_handle *,int,void *,size_t);
int64_t fs_write(struct fs_handle *,int,const void *,size_t);
int64_t fs_seek(struct fs_handle *,int,int64_t,int);
int fs_fstat(struct fs_handle *,int,struct fs_entry *);
int fs_truncate(struct fs_handle *,int,size_t);
/* Only /dev/shm page-backed files. Acquired frames transfer one reference to
 * caller; writable mapping ceiling follows the original descriptor access. */
int fs_shared_rights(struct fs_handle *,int,unsigned *);
uintptr_t fs_shared_page(struct fs_handle *,int,size_t);
unsigned fs_node_count(void);
void fs_test(void);
void fs_user_test(void);
#endif
