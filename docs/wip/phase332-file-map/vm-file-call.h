#ifndef MYOS_VM_FILE_CALL_H
#define MYOS_VM_FILE_CALL_H
#include <stdint.h>
#define MYOS_VM_FILE_CALL 57u
#define VM_FILE_FIXED 1u
#define VM_FILE_SHARED 2u
struct vm_file_request {
    uint64_t address,length,offset;
    int32_t descriptor;
    uint16_t protection,flags;
};
_Static_assert(sizeof(struct vm_file_request)==32,"file mapping syscall ABI");
#endif
