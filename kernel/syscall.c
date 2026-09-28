#include "syscall.h"
#define PAGE_ADDRESS 0x000ffffffffff000ull

/* Page tables belong to the kernel. Validate U/S at EVERY level, then copy
 * through the supervisor direct map. No unchecked user VA is dereferenced.
 * INT 0x80 keeps interrupts disabled on this single-CPU kernel, so mappings
 * cannot change between validation/copy and the console write. */
static uintptr_t readable_user_page(struct process *process, uintptr_t address, bool writable)
{
    if (address >= (1ull << 47)) return 0;
    uint64_t *table = (uint64_t *)process->root;
    for (int shift = 39; shift >= 12; shift -= 9) {
        uint64_t entry = table[(address >> shift) & 511];
        if ((entry & 5) != 5 || (writable && !(entry & 2))) return 0;
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
        uintptr_t page = readable_user_page(process, source, false);
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


static bool writable_range(struct process *p,uintptr_t address,size_t length)
{
    if(length && address>UINT64_MAX-(length-1))return false;
    while(length){
        if(!readable_user_page(p,address,true))return false;
        size_t count=PAGE_SIZE-(address&(PAGE_SIZE-1));if(count>length)count=length;
        address+=count;length-=count;
    }
    return true;
}
static void copy_to_user(struct process *p,uintptr_t address,const void *buffer,size_t length)
{
    /* Caller validated the entire range; interrupts remain disabled. */
    const char *source=buffer;
    while(length){
        size_t offset=address&(PAGE_SIZE-1),count=PAGE_SIZE-offset;if(count>length)count=length;
        uintptr_t page=readable_user_page(p,address,true);
        memcpy((void *)(page+offset),source,count);source+=count;address+=count;length-=count;
    }
}
static int user_path(struct process *p,char *path,uintptr_t address)
{
    for(size_t i=0;i<FS_PATH;++i){
        if(address>UINT64_MAX-i || !copy_from_user(p,&path[i],address+i,1))return -14;
        if(!path[i])return 0;
    }
    return -36;
}
static int64_t file_call(struct process *p,struct frame *f)
{
    char path[FS_PATH],buffer[SYS_WRITE_MAX];struct fs_entry info;
    if(f->rax==SYS_FREAD || f->rax==SYS_FWRITE || f->rax==SYS_FCLOSE){
        if(f->rdi<3 || f->rdi>=3+FS_HANDLES)return -9;
        int fd=(int)f->rdi;
        if(f->rax==SYS_FCLOSE)return fs_close(p->files,fd);
        if(f->rdx>sizeof(buffer))return -22;
        if(f->rax==SYS_FWRITE){
            if(!copy_from_user(p,buffer,f->rsi,f->rdx))return -14;
            return fs_write(p->files,fd,buffer,f->rdx);
        }
        if(!writable_range(p,f->rsi,f->rdx))return -14;
        int64_t count=fs_read(p->files,fd,buffer,f->rdx);
        if(count>0)copy_to_user(p,f->rsi,buffer,(size_t)count);
        return count;
    }
    int ret=user_path(p,path,f->rdi);if(ret)return ret;
    switch(f->rax){
    case SYS_FOPEN:
        if(f->rsi>15)return -22;
        return fs_open(p->files,path,(unsigned)f->rsi);
    case SYS_MKDIR:return fs_mkdir(path);
    case SYS_UNLINK:return fs_unlink(path);
    case SYS_READDIR:
    case SYS_STAT:
        if(f->rax==SYS_READDIR && f->rdx>UINT32_MAX)return -22;
        if(!writable_range(p,f->rsi,sizeof(info)))return -14;
        ret=f->rax==SYS_STAT?fs_stat(path,&info):fs_list(path,(unsigned)f->rdx,&info);
        if(ret>=0 && (f->rax==SYS_STAT || ret==1))copy_to_user(p,f->rsi,&info,sizeof(info));
        return ret;
    default:return -38;
    }
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
    case SYS_TICKS:
        frame->rax = ticks;
        break;
    case SYS_EXIT:
        fs_close_all(process->files);
        process->exit_status = (int32_t)frame->rdi;
        process->exited = true;
        process->state = PROCESS_ZOMBIE;
        scheduler_stop_process(process);
        return schedule(frame); /* Never resume this frame or free its stack here. */
    case SYS_YIELD:
        frame->rax = 0;
        return schedule(frame);
    case SYS_VM_MAP:
        frame->rax=(uint64_t)process_vm_map(process,frame->rdi,(size_t)frame->rsi,(unsigned)frame->rdx);
        break;
    case SYS_VM_UNMAP:
        frame->rax=(uint64_t)process_vm_unmap(process,frame->rdi,(size_t)frame->rsi);
        break;
    case SYS_VM_PROTECT:
        frame->rax=(uint64_t)process_vm_protect(process,frame->rdi,(size_t)frame->rsi,(unsigned)frame->rdx);
        break;
    case SYS_THREAD_CREATE:
        frame->rax=(uint64_t)process_thread_create(process,frame->rdi,frame->rsi,
                                                   frame->rdx,frame->r10);
        break;
    case SYS_THREAD_EXIT:
        return process_thread_exit(process,frame,(int32_t)frame->rdi);
    case SYS_THREAD_REAP: {
        int32_t status=0;
        if(!writable_range(process,frame->rsi,sizeof(status))){
            frame->rax=(uint64_t)-14;break;
        }
        int rc=process_thread_reap(process,frame->rdi,&status);
        if(!rc)copy_to_user(process,frame->rsi,&status,sizeof(status));
        frame->rax=(uint64_t)rc;break;
    }
    case SYS_WAIT:
        return process_wait(process,frame,frame->rdi,frame->rdx);
    case SYS_WAKE:
        frame->rax=process_wake(process,frame->rdi,(unsigned)frame->rsi);
        break;
    case SYS_FOPEN: case SYS_FREAD: case SYS_FWRITE: case SYS_FCLOSE:
    case SYS_MKDIR: case SYS_UNLINK: case SYS_READDIR: case SYS_STAT:
        frame->rax = (uint64_t)file_call(process,frame);
        break;
    default:
        frame->rax = (uint64_t)-38;
        break;
    }
    return frame;
}
