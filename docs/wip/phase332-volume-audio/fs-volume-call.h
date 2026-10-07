#ifndef MYOS_FS_VOLUME_CALL_H
#define MYOS_FS_VOLUME_CALL_H
#include <stdint.h>
#include <stddef.h>
#define MYOS_FS_VOLUME_CALL 58u
/* Pinned Cobalt musl_statvfs LP64 fields. No native host statvfs involved. */
struct fs_volume_info {
 uint64_t block_size,fragment_size,blocks,blocks_free,blocks_available;
 uint64_t files,files_free,files_available,filesystem_id,flags,name_max;
 int32_t reserved[6];
};
_Static_assert(sizeof(struct fs_volume_info)==112,"volume LP64 size");
_Static_assert(offsetof(struct fs_volume_info,filesystem_id)==64&&offsetof(struct fs_volume_info,flags)==72&&offsetof(struct fs_volume_info,name_max)==80,"volume LP64 fields");
#endif
