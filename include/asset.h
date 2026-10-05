#ifndef MYOS_ASSET_H
#define MYOS_ASSET_H
#include "os.h"
/* Dedicated MYOS-ASSET-v1 volume, separate from mutable RAMFS snapshots. */
struct asset_file { uint64_t offset,size; };
bool ata_asset_read(uint32_t sector,void *buffer);
uint32_t ata_asset_sectors(void);
int asset_open(const char *name,struct asset_file *file);
int asset_runtime_open(struct asset_file *file);
int asset_directory(const char *name);
int asset_child(const char *directory,unsigned position,char name[48],uint64_t *size,unsigned *type);
int asset_child_long(const char *directory,unsigned position,char name[240],uint64_t *size,unsigned *type);
bool asset_read(void *file,uint64_t offset,void *buffer,size_t count);
int asset_stat(const char *name,uint64_t *size);
int asset_entry(unsigned position,char name[48],uint64_t *size);
void asset_list(void);
#endif
