#include "syscall.h"
#include "security.h"
#include "storage.h"
#include "tcp.h"
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
    if(f->rax==SYS_FREAD || f->rax==SYS_FWRITE || f->rax==SYS_FCLOSE ||
       f->rax==SYS_FSEEK || f->rax==SYS_FSTAT || f->rax==SYS_FTRUNCATE ||
       f->rax==SYS_FREAD_LARGE || f->rax==SYS_FWRITE_LARGE || f->rax==SYS_FSYNC){
        if(f->rdi<3 || f->rdi>=3+FS_HANDLES)return -9;
        int fd=(int)f->rdi;
        if(f->rax==SYS_FCLOSE)return fs_close(p->files,fd);
        if(f->rax==SYS_FSEEK)return fs_seek(p->files,fd,(int64_t)f->rsi,(int)f->rdx);
        if(f->rax==SYS_FTRUNCATE)return fs_truncate(p->files,fd,(size_t)f->rsi);
        if(f->rax==SYS_FSTAT){if(!writable_range(p,f->rsi,sizeof(info)))return -14;
            int rc=fs_fstat(p->files,fd,&info);if(!rc)copy_to_user(p,f->rsi,&info,sizeof(info));return rc;}
        if(f->rax==SYS_FSYNC){
            int rc=fs_fstat(p->files,fd,&info);if(rc)return rc;
            if(info.reserved&1)return 0; /* Immutable asset disk already committed. */
            return storage_ready()?storage_save():-95;
        }
        if((f->rax==SYS_FREAD||f->rax==SYS_FWRITE)&&f->rdx>sizeof(buffer))return -22;
        if(f->rax==SYS_FWRITE||f->rax==SYS_FWRITE_LARGE){
            size_t total=0,length=(size_t)f->rdx;
            if(length>INT64_MAX|| (length&&f->rsi>UINT64_MAX-(length-1)))return -22;
            /* Validate every page before changing the file or its offset. */
            size_t checked=0;
            while(checked<length){
                if(!readable_user_page(p,f->rsi+checked,false))return -14;
                size_t step=PAGE_SIZE-((f->rsi+checked)&(PAGE_SIZE-1));
                if(step>length-checked)step=length-checked;
                checked+=step;
            }
            if(!length)return fs_write(p->files,fd,buffer,0);
            if(f->rax==SYS_FWRITE_LARGE&&f->r10){
                int64_t end=fs_seek(p->files,fd,0,2);if(end<0)return end;
            }
            while(total<length){size_t count=length-total;if(count>sizeof(buffer))count=sizeof(buffer);
                if(!copy_from_user(p,buffer,f->rsi+total,count))return total?(int64_t)total:-14;
                int64_t written=fs_write(p->files,fd,buffer,count);
                if(written<0)return total?(int64_t)total:written;
                total+=(size_t)written;if((size_t)written<count)break;
            }
            return (int64_t)total;
        }
        if(f->rdx>INT64_MAX)return -22;
        if(!writable_range(p,f->rsi,f->rdx))return -14;
        if(!f->rdx)return fs_read(p->files,fd,buffer,0);
        size_t total=0,length=(size_t)f->rdx;
        while(total<length){size_t request=length-total;if(request>sizeof(buffer))request=sizeof(buffer);
            int64_t count=fs_read(p->files,fd,buffer,request);
            if(count<0)return total?(int64_t)total:count;
            if(count)copy_to_user(p,f->rsi+total,buffer,(size_t)count);
            total+=(size_t)count;if((size_t)count<request)break;
        }
        return (int64_t)total;
    }
    int ret=user_path(p,path,f->rdi);if(ret)return ret;
    switch(f->rax){
    case SYS_FOPEN:
        if(f->rsi>31)return -22;
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

static int64_t tcp_call(struct process *p,struct frame *f)
{
    if(f->rax==SYS_TCP_CREATE)return tcp_stream_create(p,f->rdi,f->rsi);
    if(f->rdi>UINT32_MAX)return -9;
    uint32_t handle=(uint32_t)f->rdi;
    if(f->rax==SYS_TCP_CONNECT){struct tcp_address address;
        if(!copy_from_user(p,(char *)&address,f->rsi,sizeof(address)))return -14;
        return tcp_stream_connect(p,handle,&address);}
    if(f->rax==SYS_TCP_STATUS){struct tcp_status status;
        if(f->rdx>2)return -22;
        if(!writable_range(p,f->rsi,sizeof(status)))return -14;
        int rc=tcp_stream_status(p,handle,&status,(unsigned)f->rdx);
        if(!rc)copy_to_user(p,f->rsi,&status,sizeof(status));return rc;}
    if(f->rax==SYS_TCP_SHUTDOWN)return f->rsi>2?-22:tcp_stream_shutdown(p,handle,(unsigned)f->rsi);
    if(f->rax==SYS_TCP_CLOSE)return tcp_stream_close(p,handle);
    uint8_t buffer[1024];size_t count=f->rdx;if(count>sizeof(buffer))count=sizeof(buffer);
    if(f->rdx>INT64_MAX)return -22;
    if(f->rax==SYS_TCP_SEND){
        if(f->r10)return -22;
        if(!copy_from_user(p,(char *)buffer,f->rsi,count))return -14;
        return tcp_stream_send(p,handle,buffer,count);}
    if(f->r10>1)return -22;
    if(!writable_range(p,f->rsi,count))return -14;
    int rc=tcp_stream_recv(p,handle,buffer,count,f->r10!=0);
    if(rc>0)copy_to_user(p,f->rsi,buffer,(size_t)rc);return rc;
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
    case SYS_GETTID: {
        struct process_thread *thread=scheduler_current_thread();
        frame->rax=thread?thread->tid:process->pid;
        break;
    }
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
    case SYS_THREAD_DETACH:
        frame->rax=(uint64_t)process_thread_detach(process,frame->rdi,frame->rsi,
                                                  (size_t)frame->rdx);
        break;
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
    case SYS_VM_QUERY: {
        size_t length=(size_t)frame->rsi;
        if(!length||length>SIZE_MAX-(PAGE_SIZE-1)){frame->rax=(uint64_t)-22;break;}
        length=(length+PAGE_SIZE-1)&~(size_t)(PAGE_SIZE-1);
        size_t pages=length/PAGE_SIZE;
        if(!writable_range(process,frame->rdx,pages)){
            frame->rax=(uint64_t)-14;break;
        }
        if(!process_vm_mapped(process,frame->rdi,length)){
            frame->rax=(uint64_t)-12;break;
        }
        unsigned char resident=1;
        for(size_t i=0;i<pages;++i)copy_to_user(process,frame->rdx+i,&resident,1);
        frame->rax=0;break;
    }
    case SYS_VM_ADVISE:
        frame->rax=(uint64_t)process_vm_advise(process,frame->rdi,(size_t)frame->rsi,
                                               (unsigned)frame->rdx);
        break;
    case SYS_ERRNO_LOCATION:
        frame->rax=process_errno_location(process);
        break;
    case SYS_VM_MAP_FIXED:
        frame->rax=(uint64_t)process_vm_map_fixed(process,frame->rdi,(size_t)frame->rsi,
                                                  (unsigned)frame->rdx);
        break;
    case SYS_CLOCK_NANOS:
        if(frame->rdi==1)frame->rax=ticks*10000000ull;
        else if(frame->rdi==0)
            frame->rax=security_time_ready()?security_unix_time_ns():(uint64_t)-61;
        else frame->rax=(uint64_t)-22;
        break;
    case SYS_FOPEN: case SYS_FREAD: case SYS_FWRITE: case SYS_FCLOSE:
    case SYS_FSEEK: case SYS_FSTAT: case SYS_FTRUNCATE:
    case SYS_FREAD_LARGE: case SYS_FWRITE_LARGE: case SYS_FSYNC:
    case SYS_MKDIR: case SYS_UNLINK: case SYS_READDIR: case SYS_STAT:
        frame->rax = (uint64_t)file_call(process,frame);
        break;
    case SYS_TCP_CREATE: case SYS_TCP_CONNECT: case SYS_TCP_STATUS:
    case SYS_TCP_SEND: case SYS_TCP_RECV: case SYS_TCP_SHUTDOWN: case SYS_TCP_CLOSE:
        frame->rax=(uint64_t)tcp_call(process,frame);break;
    default:
        frame->rax = (uint64_t)-38;
        break;
    }
    return frame;
}
