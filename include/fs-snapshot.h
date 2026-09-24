#ifndef MYOS_FS_SNAPSHOT_H
#define MYOS_FS_SNAPSHOT_H
#include "fs.h"
#include "storage.h"
/* Fixed on-disk slot: 512-byte metadata sector + 4096-byte data payload. */
#define FS_SNAPSHOT_SECTORS_PER_SLOT 9u
struct fs_disk_slot {
    uint32_t used,parent,type,size;
    char name[FS_NAME];
    uint8_t reserved[STORAGE_SECTOR_SIZE-16-FS_NAME];
    uint8_t data[FS_FILE_MAX];
};
bool fs_snapshot_export(struct fs_disk_slot *slots);
bool fs_snapshot_valid(const struct fs_disk_slot *slots);
bool fs_snapshot_import(const struct fs_disk_slot *slots);
#endif
