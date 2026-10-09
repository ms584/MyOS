#ifndef MYOS_PROCESS_H
#define MYOS_PROCESS_H
#include "os.h"
#include "fs.h"
#include "thread-limit.h"
#include "vm-area.h"
#include "vm-file-call.h"

#define USER_BASE 0x8000000000ull
#define USER_DATA (USER_BASE + 0x1000)
#define USER_STACK (USER_BASE + 0x3000)
#define PROCESS_LIMIT MYOS_PROCESS_LIMIT
#define PROCESS_CODE_PAGES 16
#define PROCESS_HEAP_PAGES 32
#define PROCESS_PAGES (4 + PROCESS_CODE_PAGES + 2 + PROCESS_HEAP_PAGES)
#define PROCESS_VM_BASE (USER_BASE + 0x20000000ull)
#define PROCESS_VM_END  (USER_BASE + 0x3f000000ull)
/* Near-image VM pool, above all loader/runtime/TLS/stack reservations. */
#define PROCESS_VM_MID_BASE (USER_BASE + 0x40000000ull)
#define PROCESS_VM_MID_END  (USER_BASE + 0x100000000ull)
#define PROCESS_VM_HIGH_BASE (USER_BASE + 0x100000000ull)
#define PROCESS_VM_HIGH_END (USER_BASE + 0x4100000000ull)
#define PROCESS_RUNTIME_BASE PROCESS_VM_END
#define PROCESS_RUNTIME_TLS_BASE (PROCESS_RUNTIME_BASE + 0x10000ull)
#define PROCESS_ELF_TLS_BASE (PROCESS_RUNTIME_BASE + 0x40000ull)
_Static_assert(PROCESS_RUNTIME_TLS_BASE+(MYOS_THREAD_LIMIT+1ull)*PAGE_SIZE<=PROCESS_ELF_TLS_BASE,"runtime/native TLS separation including guard");
#define PROCESS_PROT_READ 1u
#define PROCESS_PROT_WRITE 2u
#define PROCESS_PROT_EXEC 4u
#define PROCESS_THREAD_LIMIT MYOS_WORKER_LIMIT

enum process_state { PROCESS_UNUSED, PROCESS_RUNNING, PROCESS_ZOMBIE, PROCESS_LOADING };
enum process_thread_state { PROCESS_THREAD_UNUSED, PROCESS_THREAD_RUNNING, PROCESS_THREAD_ZOMBIE };
struct process_thread {
    uint64_t tid;
    uint64_t cpu_nanos;
    enum process_thread_state state;
    void *kernel_stack;
    uintptr_t kernel_top;
    struct frame *frame;
    uint64_t fs_base;
    int32_t exit_status;
    bool detached;
    uint64_t user_stack;
    size_t user_stack_length;
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
    struct vm_areas *vm_areas;
    unsigned vm_failure_reports;
    void *kernel_stack;
    uintptr_t kernel_top;
    struct frame *frame;
    uintptr_t data_page;
    uint64_t main_stack_base,main_stack_length;
    uint64_t main_fs_base,tls_offset,tls_size,tls_stride;
    void *tls_template;
    uint64_t preemptions;
    uint64_t cpu_nanos,main_cpu_nanos;
    uint64_t fault_vector, fault_error, fault_address, fault_rip;
    bool exited;
    bool main_thread_exited;
    int32_t exit_status;
    struct fs_handle *files; /* Process-owned bounded table; freed on reap. */
    struct process_thread threads[PROCESS_THREAD_LIMIT];
    uint64_t next_tid;
    struct process_signal *signals;
};

struct elf_source;
struct elf_plan;
struct process *process_stage_elf(const struct elf_source *,const struct elf_plan *);
struct process *process_stage_elf_pair(const struct elf_source *const [2],const struct elf_plan *const [2]);
void process_discard_elf(struct process *);
struct elf_start;
bool process_start_elf(struct process *,const struct elf_start *);
bool process_start_elf_pair(struct process *,const struct elf_start *);
bool process_image_copy(struct process *,uint64_t,void *,size_t,bool);
bool process_copy_user(struct process *,uint64_t,void *,size_t,bool);
struct frame *process_signal_call(struct process *,struct frame *);
struct frame *process_signal_deliver(struct frame *);
bool process_signal_fault(struct process *,unsigned);
bool process_signal_pending(struct process *);
void process_signal_reset_thread(struct process *,struct process_thread *,bool);
void process_signal_release(struct process *);
bool process_image_readonly(struct process *,uint64_t,uint64_t);
bool process_map_runtime(struct process *);
bool process_install_tls(struct process *,const struct elf_plan *);
uint64_t process_create_elf(const struct elf_source *,const struct elf_plan *);
int64_t process_vm_map(struct process *,uint64_t,size_t,unsigned);
int64_t process_vm_map_hint(struct process *,uint64_t,size_t,unsigned);
int64_t process_vm_map_fixed(struct process *,uint64_t,size_t,unsigned);
int64_t process_vm_file(struct process *,const struct vm_file_request *);
int process_vm_unmap(struct process *,uint64_t,size_t);
int process_vm_protect(struct process *,uint64_t,size_t,unsigned);
bool process_vm_mapped(struct process *,uint64_t,size_t);
bool process_vm_resident(struct process *,uint64_t);
int process_vm_advise(struct process *,uint64_t,size_t,unsigned);
uint64_t process_errno_location(struct process *);
bool process_user_access(struct process *,uint64_t,size_t,unsigned);
int64_t process_thread_create(struct process *,uint64_t,uint64_t,uint64_t,uint64_t);
struct frame *process_thread_exit(struct process *,struct frame *,int32_t);
int process_thread_reap(struct process *,uint64_t,int32_t *);
int process_thread_detach(struct process *,uint64_t,uint64_t,size_t);
void process_cleanup_detached(struct process *);
struct frame *process_wait(struct process *,struct frame *,uint64_t,uint64_t);
unsigned process_wake(struct process *,uint64_t,unsigned);
bool process_notify(struct process *,uint64_t);
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
bool scheduler_interrupt(struct process *,struct process_thread *);
#endif
