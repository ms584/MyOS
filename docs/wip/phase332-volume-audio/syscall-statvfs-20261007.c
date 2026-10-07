#include "syscall.h"
#include "security.h"
#include "storage.h"
#include "tcp.h"
#include "dns.h"
#include "native-ui.h"
#include "graphics.h"
#include "system-call.h"
#include "net.h"
#include "nic.h"
#include "elf.h"
#include "asset.h"
#include "audio.h"
#include "pcm-call.h"
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
static bool readable_range(struct process *p,uintptr_t address,size_t length)
{
    if(length&&address>UINT64_MAX-(length-1))return false;
    while(length){
        if(!readable_user_page(p,address,false))return false;
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
static int ensure_directory(const char *path)
{
    int rc=fs_mkdir(path);if(rc&&rc!=-17)return rc;
    struct fs_entry entry;rc=fs_stat(path,&entry);
    return rc?rc:entry.type==FS_DIR?0:-20;
}
static int system_path(struct process *p,struct frame *f)
{
    struct system_path_request request;
    if(f->rdx)return -22;
    if(!copy_from_user(p,(void *)&request,f->rsi,sizeof(request)))return -14;
    const char *value=0;char executable[FS_PATH];
    switch(request.id){
    case 0:value="/assets";break;
    case 1:value="/home/cobalt/cache";break;
    case 2:value="/home/cobalt/debug";break;
    case 5:value="/home/cobalt/tmp";break;
    case 8:value="/home/cobalt";break;
    case 6:{
        /* The loader's read-only owned argv0 identifies the actual asset, not
         * a guessed /proc/self/exe or hard-coded libcobalt filename. */
        uint64_t name,boot=USER_BASE+ELF_BOOT_OFFSET;
        if(!copy_from_user(p,(void *)&name,boot+512,8)||name!=boot+536)return -2;
        memcpy(executable,"/assets/",8);unsigned i=0;
        for(;i<=64;++i){char c;if(!copy_from_user(p,&c,name+i,1))return -14;
            executable[8+i]=c;if(!c)break;}
        if(!i||i>64)return -2;value=executable;break;
    }
    default:return request.id<0||request.id>8?-22:-95;
    }
    size_t length=0;while(value[length])++length;
    if(request.length<=0||(uint32_t)request.length<=length)return -22;
    if(!request.buffer||!writable_range(p,request.buffer,length+1))return -14;
    if(request.id==1||request.id==2||request.id==5||request.id==8){
        int rc=ensure_directory("/home/cobalt");if(rc)return rc;
        if(request.id!=8&&(rc=ensure_directory(value)))return rc;
    }
    copy_to_user(p,request.buffer,value,length+1);return 0;
}
#include "pcm-syscall.inc"
static int64_t system_call(struct process *p,struct frame *f)
{
    switch(f->rdi){
    case SYSTEM_AUDIO:return pcm_call(p,f);
    case SYSTEM_LOG:{
        size_t length=f->rdx;if(length>SYSTEM_LOG_LIMIT)return -22;
        if(!readable_range(p,f->rsi,length))return -14;
        char buffer[256];
        for(size_t at=0;at<length;){size_t n=length-at;if(n>sizeof(buffer))n=sizeof(buffer);
            check(copy_from_user(p,buffer,f->rsi+at,n),"system log validated range");
            for(size_t i=0;i<n;++i)if(native_ui_owns(p))serial_put(buffer[i]);else putchar(buffer[i]);
            at+=n;
        }
        return (int64_t)length;
    }
    case SYSTEM_RANDOM:{
        if(f->rdx>256)return -22;
        if(!writable_range(p,f->rsi,f->rdx))return -14;
        if(!f->rdx)return 0;
        if(!security_entropy_ready())return -38;
        unsigned char bytes[256];if(!security_random(bytes,f->rdx))return -5;
        copy_to_user(p,f->rsi,bytes,f->rdx);return 0;
    }
    case SYSTEM_PATH:return system_path(p,f);
    case SYSTEM_INTERFACE:{
        if(f->rdx)return -22;
        if(!writable_range(p,f->rsi,sizeof(struct system_interface_snapshot)))return -14;
        struct system_interface_snapshot snapshot;
        net_interface_snapshot(&snapshot);
        copy_to_user(p,f->rsi,&snapshot,sizeof(snapshot));return 0;
    }
    case SYSTEM_ASSET_ENTRY:{
        if(f->rdx)return -22;
        struct system_asset_entry_request request;
        if(!copy_from_user(p,(char *)&request,f->rsi,sizeof(request)))return -14;
        if(request.position>UINT32_MAX)return -22;
        if(!writable_range(p,request.output,sizeof(struct system_asset_entry)))return -14;
        char path[FS_PATH];int rc=user_path(p,path,request.path);if(rc)return rc;
        const char *directory=path;
        static const char prefix[]="/assets";unsigned i=0;
        for(;prefix[i];++i)if(path[i]!=prefix[i])return -22;
        if(path[i]){if(path[i]!='/')return -22;directory=path+i+1;}else directory="";
        struct system_asset_entry entry={0};
        rc=asset_child_long(directory,(unsigned)request.position,entry.name,&entry.size,&entry.type);
        if(rc==1)copy_to_user(p,request.output,&entry,sizeof(entry));return rc;
    }
    case SYSTEM_TOTAL_BYTES:case SYSTEM_OWNED_BYTES:case SYSTEM_DISCONNECTED:
        if(f->rsi||f->rdx)return -22;
        if(f->rdi==SYSTEM_TOTAL_BYTES)return (int64_t)(pages_total()*PAGE_SIZE);
        if(f->rdi==SYSTEM_OWNED_BYTES)return (int64_t)(((uint64_t)p->page_count+p->vm_page_count)*PAGE_SIZE);
        return !nic_link_up()||!net_configured();
    default:return -22;
    }
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
    case SYS_FS_VOLUME: {
        struct fs_volume_info volume;
        if(f->rdx)return -22;
        if(!writable_range(p,f->rsi,sizeof(volume)))return -14;
        ret=fs_volume_stat(path,&volume);
        if(!ret)copy_to_user(p,f->rsi,&volume,sizeof(volume));return ret;
    }
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

static int64_t dns_call(struct process *p,struct frame *f)
{
    if(f->rdi>DNS_CALL_CONFIGURED)return -22;
    struct process_thread *thread=scheduler_current_thread();
    uint64_t tid=thread?thread->tid:p->pid;
    if(f->rdi==DNS_CALL_CONFIGURED){uint8_t server[4];
        return f->rsi||f->rdx?-22:net_dns_server(server)?1:0;}
    if(f->rdi==DNS_CALL_START){
        if(f->rdx)return -22;
        struct dns_start_request request;
        if(!copy_from_user(p,(char *)&request,f->rsi,sizeof(request)))return -14;
        if(request.reserved)return -22;
        size_t length=0;while(length<sizeof(request.name)&&request.name[length])++length;
        if(length==sizeof(request.name))return -36;
        if((request.sequence&7)||!writable_range(p,request.sequence,8))return -14;
        return dns_job_start(p,tid,request.sequence,request.name);
    }
    if(f->rsi>UINT32_MAX)return -9;
    if(f->rdi==DNS_CALL_CLOSE)return f->rdx?-22:dns_job_close(p,tid,(uint32_t)f->rsi);
    struct dns_job_status status;
    if(!writable_range(p,f->rdx,sizeof(status)))return -14;
    int result=dns_job_status(p,tid,(uint32_t)f->rsi,&status);
    if(!result)copy_to_user(p,f->rdx,&status,sizeof(status));return result;
}

static int64_t tcp_call(struct process *p,struct frame *f)
{
    if(f->rax==SYS_TCP_CREATE)return tcp_stream_create(p,f->rdi,f->rsi);
    if(f->rdi>UINT32_MAX)return -9;
    uint32_t handle=(uint32_t)f->rdi;
    if(f->rax==SYS_TCP_ACCEPT){struct tcp_accept_request request;
        if(f->rdx!=sizeof(request))return -22;
        if(!writable_range(p,f->rsi,sizeof(request))||
           !copy_from_user(p,(char *)&request,f->rsi,sizeof(request)))return -14;
        if(request.reserved)return -22;
        int64_t result=tcp_stream_accept(p,handle,&request);
        if(result>=0)copy_to_user(p,f->rsi,&request,sizeof(request));return result;
    }
    if(f->rax==SYS_TCP_LISTEN)return f->rsi>INT32_MAX?-22:
        tcp_stream_listen(p,handle,(unsigned)f->rsi);
    if(f->rax==SYS_TCP_CONNECT||f->rax==SYS_TCP_BIND){struct tcp_address address;
        if(!copy_from_user(p,(char *)&address,f->rsi,sizeof(address)))return -14;
        return f->rax==SYS_TCP_BIND?tcp_stream_bind(p,handle,&address):
            tcp_stream_connect(p,handle,&address);}
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

/* Bounded stripe scratch keeps self-test BSS below the unchanged BIOS stack
 * reservation. All source rows are validated before the first stripe. */
static uint32_t ui_row[64];
static int64_t ui_call(struct process *p,struct frame *f)
{
    if(f->rax==SYS_UI_VISIBLE){
        if(f->rsi>1||f->rdx)return -22;
        return native_ui_visible(p,f->rdi,f->rsi!=0);
    }
    if(f->rax==SYS_UI_RELEASE)
        return f->rsi||f->rdx?-22:native_ui_release(p,f->rdi);
    if(f->rax==SYS_UI_EVENT){struct native_ui_event event;
        if(f->rdx!=sizeof(event))return -22;
        if(!writable_range(p,f->rsi,sizeof(event)))return -14;
        int rc=native_ui_poll(p,f->rdi,&event);
        if(rc==1)copy_to_user(p,f->rsi,&event,sizeof(event));return rc;
    }
    struct native_ui_info info;
    if(f->rax==SYS_UI_QUERY||f->rax==SYS_UI_ACQUIRE){
        if(f->rdx)return -22;
        if(!writable_range(p,f->rsi,sizeof(info)))return -14;
        int64_t rc;
        if(f->rax==SYS_UI_ACQUIRE){
            if(!f->rdi||(f->rdi&7))return -22;
            if(!writable_range(p,f->rdi,8))return -14;
            if(f->rdi<f->rsi+sizeof(info)&&f->rsi<f->rdi+8)return -22;
            rc=native_ui_acquire(p,f->rdi,&info);
        }else rc=native_ui_query(p,f->rdi,&info);
        if(rc>=0)copy_to_user(p,f->rsi,&info,sizeof(info));return rc;
    }
    if(!f->rdi)return -9;
    int rc=native_ui_query(p,f->rdi,&info);if(rc)return rc;
    struct native_ui_present request;
    if(f->rdx!=sizeof(request))return -22;
    if(!copy_from_user(p,(char *)&request,f->rsi,sizeof(request)))return -14;
    if(!request.width||!request.height||request.width>info.width||request.height>info.height||
       request.stride<request.width*4||request.stride>16384||request.x<0||request.y<0||
       (uint64_t)request.x+request.width>info.width||(uint64_t)request.y+request.height>info.height||
       (request.format!=NATIVE_UI_RGBA8&&request.format!=NATIVE_UI_XRGB8888))return -22;
    uint64_t last=(uint64_t)(request.height-1)*request.stride;
    if(request.pixels>UINT64_MAX-last)return -14;
    /* Validate EVERY accessed row before changing ANY framebuffer pixel.
     * No user mappings can change while this single-CPU syscall keeps IF=0. */
    for(uint32_t y=0;y<request.height;++y)
        if(!readable_range(p,request.pixels+(uint64_t)y*request.stride,request.width*4))return -14;
    struct gfx_surface row={0,1,0,ui_row};
    for(uint32_t y=0;y<request.height;++y){
        for(uint32_t x=0;x<request.width;){
            uint32_t count=request.width-x;if(count>64)count=64;
            if(!copy_from_user(p,(char *)ui_row,request.pixels+(uint64_t)y*request.stride+x*4,count*4))return -14;
            if(request.format==NATIVE_UI_RGBA8)
                for(uint32_t i=0;i<count;++i){uint32_t pixel=ui_row[i];
                    ui_row[i]=((pixel&255)<<16)|(pixel&0xff00)|((pixel>>16)&255);}
            row.width=row.stride=count;
            int result=native_ui_draw(p,f->rdi,&row,request.x+(int)x,request.y+(int)y);
            if(result)return result;
            x+=count;
        }
    }
    return 0;
}

struct frame *syscall_dispatch(struct frame *frame)
{
    struct process *process = scheduler_current_process();
    if (!process || (frame->cs & 3) != 3) {
        frame->rax = (uint64_t)-1; /* EPERM: only a user process may call. */
        return frame;
    }
    process_cleanup_detached(process);
    switch (frame->rax) {
    case SYS_DNS:frame->rax=(uint64_t)dns_call(process,frame);break;
    case SYS_SYSTEM:frame->rax=(uint64_t)system_call(process,frame);break;
    case SYS_MEMORY_INFO:
        frame->rax=frame->rdi==0?pages_total():frame->rdi==1?pages_free():(uint64_t)-22;
        break;
    case SYS_SIGNAL:
        return process_signal_call(process,frame);
    case SYS_WRITE: {
        size_t length = frame->rsi;
        if (length > SYS_WRITE_MAX) { frame->rax = (uint64_t)-22; break; }
        char buffer[SYS_WRITE_MAX];
        if (!copy_from_user(process, buffer, frame->rdi, length)) {
            frame->rax = (uint64_t)-14;
            break;
        }
        /* Validate and copy the entire request BEFORE producing any output. */
        for (size_t i = 0; i < length; ++i)
            if(native_ui_owns(process))serial_put(buffer[i]);else putchar(buffer[i]);
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
    case SYS_MAIN_STACK_INFO: {
        if(frame->rsi||frame->rdx){frame->rax=(uint64_t)-22;break;}
        struct main_stack_info info={process->main_stack_base,process->main_stack_length,
                                    process->main_stack_base-PAGE_SIZE,PAGE_SIZE};
        if(!process->image_pages){info.guard_base=0;info.guard_length=0;}
        if(!info.length){frame->rax=(uint64_t)-95;break;}
        if(!writable_range(process,frame->rdi,sizeof(info))){frame->rax=(uint64_t)-14;break;}
        copy_to_user(process,frame->rdi,&info,sizeof(info));frame->rax=0;
        break;
    }
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
    /* File request checked as a complete fixed-size user structure. */
        frame->rax=(uint64_t)process_vm_unmap(process,frame->rdi,(size_t)frame->rsi);
        break;
    case SYS_VM_PROTECT:
        frame->rax=(uint64_t)process_vm_protect(process,frame->rdi,(size_t)frame->rsi,(unsigned)frame->rdx);
        break;
    case SYS_VM_FILE: {
        struct vm_file_request request;
        if(frame->rsi!=sizeof(request)||frame->rdx)frame->rax=(uint64_t)-22;
        else if(!copy_from_user(process,(void *)&request,frame->rdi,sizeof(request)))frame->rax=(uint64_t)-14;
        else frame->rax=(uint64_t)process_vm_file(process,&request);
        break;
    }
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
        for(size_t i=0;i<pages;++i){
            unsigned char resident=process_vm_resident(process,frame->rdi+i*PAGE_SIZE)?1:0;
            copy_to_user(process,frame->rdx+i,&resident,1);
        }
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
    case SYS_MKDIR: case SYS_UNLINK: case SYS_READDIR: case SYS_STAT: case SYS_FS_VOLUME:
        frame->rax = (uint64_t)file_call(process,frame);
        break;
    case SYS_TCP_CREATE: case SYS_TCP_CONNECT: case SYS_TCP_STATUS:
    case SYS_TCP_SEND: case SYS_TCP_RECV: case SYS_TCP_SHUTDOWN: case SYS_TCP_CLOSE:
    case SYS_TCP_BIND: case SYS_TCP_LISTEN: case SYS_TCP_ACCEPT:
        frame->rax=(uint64_t)tcp_call(process,frame);break;
    case SYS_UI_QUERY: case SYS_UI_ACQUIRE: case SYS_UI_PRESENT:
    case SYS_UI_RELEASE: case SYS_UI_EVENT: case SYS_UI_VISIBLE:
        frame->rax=(uint64_t)ui_call(process,frame);break;
    default:
        frame->rax = (uint64_t)-38;
        break;
    }
    return frame;
}
