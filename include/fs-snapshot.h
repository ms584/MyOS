#ifndef MYOS_FS_SNAPSHOT_H
#define MYOS_FS_SNAPSHOT_H
#include "fs.h"
#include "storage.h"
#define FS_V1_FILE_MAX 4096u
#define FS_V1_SLOT_BYTES (STORAGE_SECTOR_SIZE+FS_V1_FILE_MAX)
#define FS_V2_SLOT_BYTES (STORAGE_SECTOR_SIZE+FS_FILE_MAX)
/* The first 512 bytes of both slot versions have the same metadata. */
struct fs_disk_meta {
    uint32_t used,parent,type,size;
    char name[FS_NAME];
    uint8_t reserved[STORAGE_SECTOR_SIZE-16-FS_NAME];
};
_Static_assert(sizeof(struct fs_disk_meta)==STORAGE_SECTOR_SIZE,"snapshot metadata sector");
bool fs_snapshot_export(void *slots,size_t slot_bytes,size_t max_file);
bool fs_snapshot_valid(const void *slots,size_t slot_bytes,size_t max_file);
bool fs_snapshot_import(const void *slots,size_t slot_bytes,size_t max_file);
#endif
