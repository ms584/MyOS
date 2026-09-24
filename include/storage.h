#ifndef MYOS_STORAGE_H
#define MYOS_STORAGE_H
#include "os.h"

/* Legacy primary IDE master/slave, LBA28, one sector per request. */
#define STORAGE_SECTOR_SIZE 512u
bool storage_present(void);
bool storage_ready(void);
uint32_t storage_sectors(void);
void storage_init(void);
int storage_load(void);
int storage_save(void);
uint64_t storage_generation(void);
#endif
