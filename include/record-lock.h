#ifndef MYOS_RECORD_LOCK_H
#define MYOS_RECORD_LOCK_H
#include "os.h"
/* Pinned musl LP64 flock. Padding is opaque, never an input precondition. */
struct record_lock_abi {
    int16_t type,whence;uint32_t padding;
    int64_t start,length;int32_t pid;uint32_t tail_padding;
};
_Static_assert(sizeof(struct record_lock_abi)==32,"musl flock ABI");
_Static_assert(offsetof(struct record_lock_abi,start)==8&&
               offsetof(struct record_lock_abi,length)==16&&
               offsetof(struct record_lock_abi,pid)==24,"musl flock field offsets");
#define RECORD_LIMIT 64u
enum { RECORD_READ=0, RECORD_WRITE=1, RECORD_UNLOCK=2,
       RECORD_GET=5, RECORD_SET=6, RECORD_SET_WAIT=7 };
/* Caller serializes against namespace/close with IRQ exclusion. Key is inode
 * identity, owner is the kernel file-table identity, not a user-supplied PID.
 * Ranges are normalized [start,end), end2^63 means through future EOF. */
int record_apply(uint64_t,uintptr_t,int32_t,unsigned,int,uint64_t,uint64_t,struct record_lock_abi *);
void record_release(uint64_t,uintptr_t);
#endif
