/* This host harness covers immutable asset copies only. Mutable shared frames
 * are exercised in native test_shared_memory.py, not simulated here. */
#include <assert.h>
#include <process.h>
int fs_shared_rights(struct fs_handle *table,int descriptor,unsigned *rights)
{
    (void)table;(void)descriptor;(void)rights;
    assert(!"immutable-only test unexpectedly requested shared rights");return -95;
}
uintptr_t fs_shared_page(struct fs_handle *table,int descriptor,size_t offset)
{
    (void)table;(void)descriptor;(void)offset;
    assert(!"immutable-only test unexpectedly requested a shared frame");return 0;
}
