#include "fs-unit-platform.h"
uintptr_t page_alloc(void);
void page_free(uintptr_t);
bool page_retain(uintptr_t);
unsigned page_references(uintptr_t);
