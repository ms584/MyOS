#ifndef MYOS_SHM_BACKING_H
#define MYOS_SHM_BACKING_H
#include "os.h"
#define SHM_MAX_BYTES (1024u*1024u)
#define SHM_MAX_PAGES (SHM_MAX_BYTES/PAGE_SIZE)
/* Each live slot owns one PMM reference. Each mapped alias owns another.
 * The inode may die before its mapped frames, with no dependency on a name. */
struct shm_backing { size_t size; uintptr_t pages[SHM_MAX_PAGES]; };
int shm_resize(struct shm_backing *,size_t);
void shm_release(struct shm_backing *);
int64_t shm_read(struct shm_backing *,size_t,void *,size_t);
int64_t shm_write(struct shm_backing *,size_t,const void *,size_t);
uintptr_t shm_acquire_page(struct shm_backing *,size_t);
#endif
