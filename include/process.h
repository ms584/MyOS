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
#define PROCESS_VM_BASE (USER_BASE + 0x20000000ull)
#define PROCESS_VM_END  (USER_BASE + 0x3f000000ull)
#define PROCESS_RUNTIME_BASE PROCESS_VM_END
#define PROCESS_RUNTIME_TLS_BASE (PROCESS_RUNTIME_BASE + 0x10000ull)
#define PROCESS_PROT_READ 1u
#define PROCESS_PROT_WRITE 2u
#define PROCESS_PROT_EXEC 4u
#define PROCESS_THREAD_LIMIT 8u

enum process_state { PROCESS_UNUSED, PROCESS_RUNNING, PROCESS_ZOMBIE, PROCESS_LOADING };
enum process_thread_state { PROCESS_THREAD_UNUSED, PROCESS_THREAD_RUNNING, PROCESS_THREAD_ZOMBIE };
struct process_thread {
    uint64_t tid;
    enum process_thread_state state;
    void *kernel_stack;
    uintptr_t kernel_top;
    struct frame *frame;
    uint64_t fs_base;
    int32_t exit_status;
};
struct process {
    uint64_t pid;
    enum process_state state;
    uintptr_t root;
    uintptr_t pages[PROCESS_PAGES];
    uintptr_t *image_pages; /* ELF-owned pages; NULL for legacy images. */
    unsigned page_count,image_capacity;
    uintptr_t *vm_pages; /* Pages and page tables created by anonymous VM. */
    unsigned vm_page_count,vm_page_capacity;
    void *kernel_stack;
    uintptr_t kernel_top;
    struct frame *frame;
    uintptr_t data_page;
    uint64_t preemptions;
    uint64_t fault_vector, fault_error, fault_address, fault_rip;
    bool exited;
    int32_t exit_status;
    struct fs_handle files[FS_HANDLES];
    struct process_thread threads[PROCESS_THREAD_LIMIT];
    uint64_t next_tid;
};

struct elf_source;
struct elf_plan;
struct process *process_stage_elf(const struct elf_source *,const struct elf_plan *);
void process_discard_elf(struct process *);
bool process_start_elf(struct process *,uint64_t,uint64_t,unsigned);
bool process_image_copy(struct process *,uint64_t,void *,size_t,bool);
bool process_image_readonly(struct process *,uint64_t,uint64_t);
bool process_map_runtime(struct process *);
uint64_t process_create_elf(const struct elf_source *,const struct elf_plan *);
int64_t process_vm_map(struct process *,uint64_t,size_t,unsigned);
int64_t process_vm_map_fixed(struct process *,uint64_t,size_t,unsigned);
int process_vm_unmap(struct process *,uint64_t,size_t);
int process_vm_protect(struct process *,uint64_t,size_t,unsigned);
bool process_vm_mapped(struct process *,uint64_t,size_t);
int process_vm_advise(struct process *,uint64_t,size_t,unsigned);
uint64_t process_errno_location(struct process *);
bool process_user_access(struct process *,uint64_t,size_t,unsigned);
int64_t process_thread_create(struct process *,uint64_t,uint64_t,uint64_t,uint64_t);
struct frame *process_thread_exit(struct process *,struct frame *,int32_t);
int process_thread_reap(struct process *,uint64_t,int32_t *);
struct frame *process_wait(struct process *,struct frame *,uint64_t,uint64_t);
unsigned process_wake(struct process *,uint64_t,unsigned);
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
struct process_thread *scheduler_current_thread(void);
bool scheduler_attach_thread(struct process *,struct process_thread *);
bool scheduler_stop_thread(struct process *,struct process_thread *);
bool scheduler_detach_thread(struct process *,struct process_thread *);
struct frame *scheduler_wait(struct frame *,struct process *,uint64_t,uint64_t);
unsigned scheduler_wake(struct process *,uint64_t,unsigned);
#endif
