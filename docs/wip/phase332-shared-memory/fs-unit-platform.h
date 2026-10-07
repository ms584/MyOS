#ifndef MYOS_FS_UNIT_PLATFORM_H
#define MYOS_FS_UNIT_PLATFORM_H
/* Only privileged IRQ/console/allocator boundaries are substituted. */
#define MYOS_OS_H
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#define PAGE_SIZE 4096u
static inline uint64_t irq_save(void){return 0;}
static inline void irq_restore(uint64_t flags){(void)flags;}
void *kmalloc(size_t);
void kfree(void *);
void check(bool,const char *);
void print(const char *);
void *memcpy(void *,const void *,size_t);
void *memset(void *,int,size_t);
#endif
