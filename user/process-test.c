#include "process.h"
#ifndef PROCESS_TESTS
#define PROCESS_TESTS 0
#endif

static uint64_t progress(struct process *process)
{
    return *(volatile uint64_t *)process->data_page;
}

static void wait_ticks(uint64_t count)
{
    uint64_t until = ticks + count;
    while (ticks < until) __asm__ volatile ("hlt");
}

#if PROCESS_TESTS
static void allocation_failure_test(void)
{
    uint64_t flags = irq_save();
    size_t before = pages_free();
    uintptr_t list = 0;
    while (pages_free() > 3) {
        uintptr_t page = page_alloc();
        check(page != 0, "process OOM setup");
        *(uintptr_t *)page = list;
        list = page;
    }
    check(!process_create(0, 0), "process PMM OOM result");
    check(pages_free() == 3, "partial process pages reclaimed");
    while (list) {
        uintptr_t next = *(uintptr_t *)list;
        page_free(list);
        list = next;
    }
    check(pages_free() == before, "PMM after process OOM");
    void *blocks = 0, *block;
    while ((block = kmalloc(4096))) { *(void **)block = blocks; blocks = block; }
    check(!process_create(0, 0), "process heap OOM result");
    check(pages_free() == before, "process heap OOM page cleanup");
    while (blocks) { void *next = *(void **)blocks; kfree(blocks); blocks = next; }
    irq_restore(flags);
    print("PASS: process partial-allocation rollback\n");
}
#endif

void process_test(void)
{
    size_t initial_pages = pages_free();
    uint64_t flags = irq_save();
    uint64_t first = process_create(0, 0x1111222233334444ull);
    uint64_t second = process_create(0, 0xaaaabbbbccccddddull);
    check(first && second && first != second, "distinct process IDs");
    struct process *a = process_find(first), *b = process_find(second);
    check(a->root != b->root && a->data_page != b->data_page, "private process pages");
    check(!progress(a) && !progress(b), "new process data zeroed");
    irq_restore(flags);
    wait_ticks(50);
    check(progress(a) && progress(b), "user execution");
    check(((uint64_t *)a->data_page)[1] == 0x2b && ((uint64_t *)b->data_page)[1] == 0x2b,
          "actual Ring 3 CS");
    check(((uint64_t *)a->data_page)[2] == 0x1111222233334444ull
          && ((uint64_t *)b->data_page)[2] == 0xaaaabbbbccccddddull, "same VA private data");
    check(((uint64_t *)a->data_page)[4] == 0x1111222233334444ull
          && ((uint64_t *)b->data_page)[4] == 0xaaaabbbbccccddddull
          && ((uint64_t *)a->data_page)[5] == USER_STACK + PAGE_SIZE - 8
          && ((uint64_t *)b->data_page)[5] == USER_STACK + PAGE_SIZE - 8
          && a->pages[6] != b->pages[6], "private user stacks");
    check(a->preemptions >= 2 && b->preemptions >= 2, "timer returns from Ring 3");
    check(!process_reap(first), "cannot reap a running process");
    print("PASS: Ring 3 processes, private CR3/data/stack, timer preemption\n");

#if PROCESS_TESTS
    uint64_t third = process_create(0, 3), fourth = process_create(0, 4);
    check(third && fourth && !process_create(0, 5), "process table capacity");
    check(process_kill(third) && process_reap(third), "third process cleanup");
    check(process_kill(fourth) && process_reap(fourth), "fourth process cleanup");
    check(!process_find(third) && !process_reap(third), "stale process ID rejected");
    allocation_failure_test();
    const unsigned vectors[] = {0,14,14,13,13,14,14,14,6,13,14,13,14,7};
    const unsigned errors[]  = {0, 5, 7, 0, 0, 7,21, 4,0,0x102,5,0,21,0};
    const uintptr_t addresses[] = {0,0x10000,0x10000,0,0,USER_BASE,USER_STACK,
                                   USER_BASE+0x2000,0,0,0x100000000ull,0,0x10000,0};
    for (unsigned mode = 1; mode <= 13; ++mode) {
        size_t before = pages_free();
        uint64_t progress_before = progress(a);
        uint64_t pid = process_create(mode, mode);
        struct process *victim = process_find(pid);
        check(victim != 0, "fault process creation");
        uint64_t deadline = ticks + 200;
        while (victim->state == PROCESS_RUNNING && ticks < deadline)
            __asm__ volatile ("hlt" : : : "memory");
        check(victim->state == PROCESS_ZOMBIE, "fault process contained");
        check(victim->fault_vector == vectors[mode] && victim->fault_error == errors[mode],
              "user exception vector/error");
        if (vectors[mode] == 14) check(victim->fault_address == addresses[mode], "user fault CR2");
        check(process_reap(pid), "fault process reap");
        check(pages_free() == before, "process pages reclaimed");
        wait_ticks(10);
        check(progress(a) > progress_before, "other process survives fault");
    }
    print("PASS: 13 user fault cases contained; kernel and peer survived\n");
    check(process_kill(first) && process_reap(first), "first process cleanup");
    check(process_kill(second) && process_reap(second), "second process cleanup");
    check(pages_free() == initial_pages, "all process pages recovered");
    for (unsigned i = 0; i < 16; ++i) {
        uint64_t pid = process_create(0, i);
        check(pid && process_kill(pid) && process_reap(pid), "process slot recycling");
    }
    check(pages_free() == initial_pages, "repeated process cleanup");
    first = process_create(0, 1); second = process_create(0, 2);
    check(first && second, "restart user demos");
    print("PASS: process kill/reap, capacity, PID and resource reuse\n");
#else
    (void)initial_pages;
#endif
    print("User processes running: "); decimal(first); print(", "); decimal(second); print(" (Ring 3)\n");
}
