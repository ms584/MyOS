#ifndef MYOS_BLOCK_H
#define MYOS_BLOCK_H
#include "storage.h"
void block_init(void);
bool block_read(uint32_t,void *);
bool block_write(uint32_t,const void *);
bool block_flush(void);
#endif
