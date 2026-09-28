#ifndef MYOS_ASSET_H
#define MYOS_ASSET_H
#include "os.h"
/* Dedicated MYOS-ASSET-v1 volume, separate from mutable RAMFS snapshots. */
struct asset_file { uint64_t offset,size; };
bool ata_asset_read(uint32_t sector,void *buffer);
uint32_t ata_asset_sectors(void);
int asset_open(const char *name,struct asset_file *file);
bool asset_read(void *file,uint64_t offset,void *buffer,size_t count);
void asset_list(void);
#endif
