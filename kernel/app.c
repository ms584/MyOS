#include "app.h"
#include "fs.h"
#include "process.h"

extern const unsigned char hello_app_start[],hello_app_end[];
struct app_header { char magic[8]; uint32_t size,checksum; } __attribute__((packed));
static uint32_t checksum(const uint8_t *data,size_t size)
{
    uint32_t hash=2166136261u;
    for(size_t i=0;i<size;++i)hash=(hash^data[i])*16777619u;
    return hash;
}
static bool valid(const uint8_t *file,size_t size)
{
    if(size<sizeof(struct app_header))return false;
    const struct app_header *h=(const void *)file;
    static const char magic[8]={'M','Y','O','S','A','P','P','1'};
    for(unsigned i=0;i<8;++i)if(h->magic[i]!=magic[i])return false;
    return h->size>0 && h->size<=PAGE_SIZE && h->size<=FS_FILE_MAX-sizeof(*h) &&
           size==sizeof(*h)+h->size && checksum(file+sizeof(*h),h->size)==h->checksum;
}
void app_seed(void)
{
    struct fs_entry entry;
    if(!fs_stat("/bin/hello",&entry))return;
    size_t size=(size_t)(hello_app_end-hello_app_start);
    check(valid(hello_app_start,size),"seed app format");
    struct fs_handle files[FS_HANDLES]={0};
    int fd=fs_open(files,"/bin/hello",FS_WRITE|FS_CREATE);
    if(fd<0)return; /* A restored filesystem may be full. */
    int64_t written=fs_write(files,fd,hello_app_start,size);
    (void)fs_close(files,fd);
    check(written==(int64_t)size,"seed app write");
    print("App available: /bin/hello (Ring 3)\n");
}
int app_run(const char *path,int32_t *status)
{
    struct fs_entry entry;
    int result=fs_stat(path,&entry);
    if(result<0)return result;
    if(entry.type!=FS_FILE)return -21;
    if(entry.size<sizeof(struct app_header) || entry.size>FS_FILE_MAX)return -8;
    uint8_t *file=kmalloc((size_t)entry.size);
    if(!file)return -12;
    struct fs_handle files[FS_HANDLES]={0};
    int fd=fs_open(files,path,FS_READ);
    if(fd<0){kfree(file);return fd;}
    int64_t length=fs_read(files,fd,file,(size_t)entry.size);
    (void)fs_close(files,fd);
    if(length!=(int64_t)entry.size || !valid(file,(size_t)entry.size)){
        kfree(file);return -8;
    }
    const struct app_header *header=(const void *)file;
    uint64_t pid=process_create_image(file+sizeof(*header),header->size);
    kfree(file);
    if(!pid)return -12;
    uint64_t deadline=ticks+500;
    struct process *p=process_find(pid);
    while(p->state==PROCESS_RUNNING && ticks<deadline)__asm__ volatile("sti; hlt":::"memory");
    if(p->state==PROCESS_RUNNING){(void)process_kill(pid);result=-110;}
    else if(p->exited){*status=p->exit_status;result=0;}
    else result=-8;
    check(process_reap(pid),"app process reap");
    return result;
}
