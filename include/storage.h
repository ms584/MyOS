#ifndef MYOS_STORAGE_H
#define MYOS_STORAGE_H
#include "os.h"

/* Snapshot backend: marked legacy ATA or bounded prepared USB partition. */
#define STORAGE_SECTOR_SIZE 512u
bool storage_present(void);
const char *storage_backend(void);
bool storage_ready(void);
uint32_t storage_sectors(void);
void storage_init(void);
int storage_load(void);
int storage_save(void);
uint64_t storage_generation(void);
#endif
