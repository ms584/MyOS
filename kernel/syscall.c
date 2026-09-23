#include "syscall.h"
#define PAGE_ADDRESS 0x000ffffffffff000ull

/* Page tables belong to the kernel. Validate U/S at EVERY level, then copy
 * through the supervisor direct map. No unchecked user VA is dereferenced.
 * INT 0x80 keeps interrupts disabled on this single-CPU kernel, so mappings
 * cannot change between validation/copy and the console write. */
static uintptr_t readable_user_page(struct process *process, uintptr_t address)
{
    if (address >= (1ull << 47)) return 0;
    uint64_t *table = (uint64_t *)process->root;
    for (int shift = 39; shift >= 12; shift -= 9) {
        uint64_t entry = table[(address >> shift) & 511];
        if ((entry & 5) != 5) return 0;
        if (shift > 12 && (entry & 128)) return 0;
        uintptr_t physical = entry & PAGE_ADDRESS;
        if (!physical || physical >= 256ull * 1024 * 1024) return 0;
        if (shift == 12) return physical;
        table = (uint64_t *)physical;
    }
    return 0;
}

static bool copy_from_user(struct process *process, char *destination,
                           uintptr_t source, size_t length)
{
    if (length && source > UINT64_MAX - (length - 1)) return false;
    while (length) {
        uintptr_t page = readable_user_page(process, source);
        if (!page) return false;
        size_t offset = source & (PAGE_SIZE - 1);
        size_t count = PAGE_SIZE - offset;
        if (count > length) count = length;
        memcpy(destination, (void *)(page + offset), count);
        destination += count;
        source += count;
        length -= count;
    }
    return true;
}

struct frame *syscall_dispatch(struct frame *frame)
{
    struct process *process = scheduler_current_process();
    if (!process || (frame->cs & 3) != 3) {
        frame->rax = (uint64_t)-1; /* EPERM: only a user process may call. */
        return frame;
    }
    switch (frame->rax) {
    case SYS_WRITE: {
        size_t length = frame->rsi;
        if (length > SYS_WRITE_MAX) { frame->rax = (uint64_t)-22; break; }
        char buffer[SYS_WRITE_MAX];
        if (!copy_from_user(process, buffer, frame->rdi, length)) {
            frame->rax = (uint64_t)-14;
            break;
        }
        /* Validate and copy the entire request BEFORE producing any output. */
        for (size_t i = 0; i < length; ++i) putchar(buffer[i]);
        frame->rax = length;
        break;
    }
    case SYS_GETPID:
        frame->rax = process->pid;
        break;
    case SYS_EXIT:
        process->exit_status = (int32_t)frame->rdi;
        process->exited = true;
        process->state = PROCESS_ZOMBIE;
        scheduler_stop_process(process);
        return schedule(frame); /* Never resume this frame or free its stack here. */
    case SYS_YIELD:
        frame->rax = 0;
        return schedule(frame);
    default:
        frame->rax = (uint64_t)-38;
        break;
    }
    return frame;
}
