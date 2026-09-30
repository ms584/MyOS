#ifndef MYOS_FS_H
#define MYOS_FS_H
#include "asset.h"
#define FS_NODES 64
#define FS_HANDLES 8
#define FS_PATH 128
#define FS_NAME 32
#define FS_FILE_MAX 65536
enum { FS_READ=1, FS_WRITE=2, FS_CREATE=4, FS_TRUNC=8, FS_EXCL=16 };
enum { FS_FILE=1, FS_DIR=2 };
struct fs_handle { unsigned node, flags; size_t offset; struct asset_file asset; };
struct fs_entry { char name[FS_NAME]; uint32_t type, reserved; uint64_t size; };
void fs_init(void);
int fs_mkdir(const char *);
int fs_unlink(const char *);
int fs_stat(const char *,struct fs_entry *);
int fs_list(const char *,unsigned,struct fs_entry *);
int fs_open(struct fs_handle *,const char *,unsigned);
int fs_close(struct fs_handle *,int);
void fs_close_all(struct fs_handle *);
int64_t fs_read(struct fs_handle *,int,void *,size_t);
int64_t fs_write(struct fs_handle *,int,const void *,size_t);
int64_t fs_seek(struct fs_handle *,int,int64_t,int);
int fs_fstat(struct fs_handle *,int,struct fs_entry *);
int fs_truncate(struct fs_handle *,int,size_t);
unsigned fs_node_count(void);
void fs_test(void);
void fs_user_test(void);
#endif
