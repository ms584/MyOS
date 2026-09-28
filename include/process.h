#ifndef MYOS_PROCESS_H
#define MYOS_PROCESS_H
#include "os.h"
#include "fs.h"

#define USER_BASE 0x8000000000ull
#define USER_DATA (USER_BASE + 0x1000)
#define USER_STACK (USER_BASE + 0x3000)
#define PROCESS_LIMIT 4
#define PROCESS_CODE_PAGES 16
#define PROCESS_HEAP_PAGES 32
#define PROCESS_PAGES (4 + PROCESS_CODE_PAGES + 2 + PROCESS_HEAP_PAGES)

enum process_state { PROCESS_UNUSED, PROCESS_RUNNING, PROCESS_ZOMBIE, PROCESS_LOADING };
struct process {
    uint64_t pid;
    enum process_state state;
    uintptr_t root;
    uintptr_t pages[PROCESS_PAGES];
    uintptr_t *image_pages; /* ELF-owned pages; NULL for legacy images. */
    unsigned page_count;
    void *kernel_stack;
    uintptr_t kernel_top;
    struct frame *frame;
    uintptr_t data_page;
    uint64_t preemptions;
    uint64_t fault_vector, fault_error, fault_address, fault_rip;
    bool exited;
    int32_t exit_status;
    struct fs_handle files[FS_HANDLES];
};

struct elf_source;
struct elf_plan;
struct process *process_stage_elf(const struct elf_source *,const struct elf_plan *);
void process_discard_elf(struct process *);
bool process_start_elf(struct process *,uint64_t,uint64_t,unsigned);
bool process_image_copy(struct process *,uint64_t,void *,size_t,bool);
bool process_image_readonly(struct process *,uint64_t,uint64_t);
uint64_t process_create_elf(const struct elf_source *,const struct elf_plan *);
void process_init(void);
uint64_t process_create(uint64_t mode, uint64_t cookie);
uint64_t process_create_image(const void *code,size_t size);
struct process *process_find(uint64_t pid);
bool process_kill(uint64_t pid);
bool process_reap(uint64_t pid);
struct frame *process_fault(struct frame *frame);
void process_test(void);
void tss_set_stack(uintptr_t top);
bool scheduler_attach_process(struct process *process);
bool scheduler_stop_process(struct process *process);
bool scheduler_detach_process(struct process *process);
struct process *scheduler_current_process(void);
#endif
