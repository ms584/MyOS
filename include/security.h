#ifndef MYOS_SECURITY_H
#define MYOS_SECURITY_H
#include "os.h"
void security_init(void);
bool security_time_ready(void);
bool security_entropy_ready(void);
uint64_t security_unix_time(void);
bool security_random(void *out,size_t size);
void security_status(void);
#endif
