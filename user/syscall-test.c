#include "syscall.h"
#ifndef PROCESS_TESTS
#define PROCESS_TESTS 0
#endif

static struct process *wait_exit(uint64_t pid)
{
    struct process *process = process_find(pid);
    check(process != 0, "syscall process creation");
    uint64_t deadline = ticks + 400;
    while (process->state == PROCESS_RUNNING && ticks < deadline)
        __asm__ volatile ("hlt" : : : "memory");
    check(process->state == PROCESS_ZOMBIE && process->exited,
          "sys_exit completed without a user fault");
    return process;
}

void syscall_test(void)
{
    size_t before = pages_free();
    uint64_t result = SYS_GETPID;
    __asm__ volatile ("int $0x80" : "+a"(result) : : "memory");
    check((int64_t)result == -1, "kernel callers rejected by syscall API");
#if PROCESS_TESTS
    uint64_t pid = process_create(14, 0);
    struct process *process = wait_exit(pid);
    uint64_t *data = (uint64_t *)process->data_page;
    check(process->exit_status == 0, "syscall contract failure");
    check(data[6] == pid && data[7] == 29, "syscall results and register preservation");
    check(process_reap(pid) && pages_free() == before, "syscall process reclaimed");
    pid = process_create(16, 0);
    process = wait_exit(pid);
    check(process->exit_status == -7, "signed exit status");
    check(process_reap(pid) && pages_free() == before, "negative exit reclaimed");
    print("PASS: syscall ABI, 29 user checks, pointer validation, signed exit\n");
#endif
    uint64_t demo_pid = process_create(15, 0);
    struct process *demo = wait_exit(demo_pid);
    check(demo->exit_status == 42 && ((uint64_t *)demo->data_page)[6] == demo_pid,
          "user write/getpid/yield/exit demo");
    check(process_reap(demo_pid) && pages_free() == before, "demo process reclaimed");
    print("PASS: user write/getpid/yield/exit; status=42, resources reclaimed\n");
}
