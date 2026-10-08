#include "fs-snapshot.h"
#ifndef PROCESS_TESTS
#define PROCESS_TESTS 0
#endif
#if PROCESS_TESTS
static void shm_inode_pool_test(void)
{
    unsigned before=fs_node_count();size_t heap_before=heap_allocated(),pages_before=pages_free();
    check(!fs_mkdir("/dev/shm"),"shared inode test root");
    unsigned ordinary=0;char path[]="/home/p00";
    for(unsigned i=0;i<FS_NODES;++i){
        path[7]='0'+i/10;path[8]='0'+i%10;
        int rc=fs_mkdir(path);if(rc==-28)break;
        check(!rc,"persistent inode capacity create");++ordinary;
    }
    check(fs_node_count()==FS_NODES,"persistent pool full before SHM");
    struct fs_handle files[FS_HANDLES]={0};char shared[]="/dev/shm/s00";
    for(unsigned i=0;i<FS_SHM_NODES;++i){
        shared[10]='0'+i/10;shared[11]='0'+i%10;
        int fd=fs_open(files,shared,FS_READ|FS_WRITE|FS_CREATE);
        check(fd==3&&!fs_close(files,fd),"SHM independent of full persistent pool");
    }
    check(fs_node_count()==FS_LIVE_NODES,"both inode pools full");
    check(fs_open(files,"/dev/shm/extra",FS_WRITE|FS_CREATE)==-28&&
          fs_mkdir("/home/extra")==-28,"independent inode exhaustion");
    struct fs_volume_info root,shm;
    check(!fs_volume_stat("/",&root)&&!fs_volume_stat("/dev/shm",&shm)&&
          root.files==FS_NODES&&!root.files_free&&shm.files==FS_SHM_NODES&&
          !shm.files_free&&root.filesystem_id!=shm.filesystem_id,"namespace inode statistics");
    void *slots=kmalloc((size_t)FS_V1_SLOT_BYTES*FS_NODES);
    check(slots&&fs_snapshot_export(slots,FS_V1_SLOT_BYTES,FS_V1_FILE_MAX)&&
          fs_snapshot_valid(slots,FS_V1_SLOT_BYTES,FS_V1_FILE_MAX),"snapshot64 skips ephemeral pool");
    int fd=fs_open(files,"/dev/shm/s00",FS_READ|FS_WRITE);
    check(fd==3&&fs_write(files,fd,"x",1)==1&&!fs_unlink("/dev/shm/s00"),"unlinked high inode remains pinned");
    check(!fs_snapshot_import(slots,FS_V1_SLOT_BYTES,FS_V1_FILE_MAX),"snapshot import rejects high pinned inode");
    check(!fs_close(files,fd)&&!fs_volume_stat("/dev/shm",&shm)&&shm.files_free==1,"last close frees high inode");
    fd=fs_open(files,"/dev/shm/reused",FS_WRITE|FS_CREATE);
    check(fd==3&&!fs_close(files,fd),"high inode reuse");
    check(fs_snapshot_import(slots,FS_V1_SLOT_BYTES,FS_V1_FILE_MAX),"snapshot import releases closed ephemeral nodes");
    kfree(slots);
    for(unsigned i=0;i<ordinary;++i){path[7]='0'+i/10;path[8]='0'+i%10;check(!fs_unlink(path),"persistent pool cleanup");}
    check(fs_node_count()==before&&heap_allocated()==heap_before&&pages_free()==pages_before,"inode pools exact resource recovery");
    print("PASS: independent SHM inode pool, snapshot64 compatibility and exact resources\n");
}
#endif
void fs_test(void)
{
    unsigned before=fs_node_count();
    struct fs_handle files[FS_HANDLES]={0};struct fs_entry info;
    char buffer[16]={0};
    check(!fs_mkdir("/home/test"),"mkdir");
    check(fs_mkdir("/home/test")==-17,"duplicate directory");
    int a=fs_open(files,"/home/test/file",FS_READ|FS_WRITE|FS_CREATE);
    check(a==3&&fs_write(files,a,"abc\0def",7)==7,"binary write");
    int b=fs_open(files,"/home//test/./file",FS_READ);
    check(b==4&&fs_read(files,b,buffer,3)==3&&buffer[0]=='a',"independent offset");
    check(fs_read(files,b,buffer,8)==4&&buffer[0]==0&&buffer[3]=='f',"binary read");
    check(fs_read(files,b,buffer,8)==0,"EOF");
    check(fs_write(files,b,"x",1)==-9,"read-only handle");
    check(fs_unlink("/home/test")==-39,"nonempty directory");
    check(!fs_stat("/home/test/../test/file",&info)&&info.size==7&&info.type==FS_FILE,"stat and parent path");
    check(fs_list("/home/test",0,&info)==1&&fs_list("/home/test",1,&info)==0,"directory iteration");
    check(!fs_close(files,a)&&fs_close(files,a)==-9,"close and invalid handle");
    a=fs_open(files,"/home/test/file",FS_WRITE|FS_TRUNC);
    check(a==3&&!fs_stat("/home/test/file",&info)&&info.size==0,"truncate");
    check(fs_read(files,a,buffer,1)==-9,"write-only handle");
    check(fs_write(files,a,"XY",2)==2,"write after truncate");
    check(fs_read(files,b,buffer,1)==0,"old offset past truncated EOF");
    fs_close_all(files);
    check(!fs_unlink("/home/test/file")&&!fs_unlink("/home/test"),"remove file and directory");
    check(fs_unlink("/")==-16&&fs_open(files,"/etc",FS_READ)==-21,"root and directory guards");
    check(fs_open(files,"relative",FS_READ)==-22&&fs_open(files,"/missing/a",FS_CREATE|FS_WRITE)==-2,"path validation");
    check(fs_open(files,"/home/bad",FS_READ|FS_TRUNC)==-22,"invalid flags");
    check(fs_open(files,"/home/trailing/",FS_WRITE|FS_CREATE)==-20,"file trailing slash");
#if PROCESS_TESTS
    check(!fs_mkdir("/home/TEzLw=="),"Cobalt Base64 cache component");
    a=fs_open(files,"/home/TEzLw==/Experiment Config",FS_READ|FS_WRITE|FS_CREATE);
    check(a==3&&fs_write(files,a,"config",6)==6,"Cobalt config name with space");
    check(!fs_close(files,a)&&!fs_stat("/home/TEzLw==/Experiment Config",&info)&&info.size==6,"Cobalt config stat");
    check(fs_mkdir("/home/control\n")==-22&&fs_mkdir("/home/del\177")==-22&&fs_mkdir("/home/nonascii\200")==-22,"filename controls and non-ASCII rejected");
    void *name_slots=kmalloc((size_t)FS_V1_SLOT_BYTES*FS_NODES);
    check(name_slots&&fs_snapshot_export(name_slots,FS_V1_SLOT_BYTES,FS_V1_FILE_MAX)&&fs_snapshot_valid(name_slots,FS_V1_SLOT_BYTES,FS_V1_FILE_MAX),"extended names snapshot valid");
    check(fs_snapshot_import(name_slots,FS_V1_SLOT_BYTES,FS_V1_FILE_MAX)&&!fs_stat("/home/TEzLw==/Experiment Config",&info)&&info.size==6,"extended names snapshot restored");
    struct fs_disk_meta *name_meta=(struct fs_disk_meta *)((uint8_t *)name_slots+FS_V1_SLOT_BYTES);
    char original=name_meta->name[0];name_meta->name[0]='/';
    check(!fs_snapshot_valid(name_slots,FS_V1_SLOT_BYTES,FS_V1_FILE_MAX),"snapshot embedded separator rejected");name_meta->name[0]=original;
    kfree(name_slots);
    check(!fs_unlink("/home/TEzLw==/Experiment Config")&&!fs_unlink("/home/TEzLw=="),"Cobalt name cleanup");
    char longpath[FS_PATH+1];memset(longpath,'a',sizeof(longpath));longpath[0]='/';longpath[FS_PATH]=0;
    check(fs_mkdir(longpath)==-36,"path length bound");longpath[33]=0;
    check(fs_mkdir(longpath)==-36,"component length bound");
    a=fs_open(files,"/home/full",FS_WRITE|FS_CREATE);
    char *block=kmalloc(FS_FILE_MAX);check(block!=0,"test buffer");memset(block,0xa5,FS_FILE_MAX);
    check(fs_write(files,a,block,FS_FILE_MAX)==FS_FILE_MAX&&fs_write(files,a,"!",1)==-28,"file size capacity");
    check(!fs_stat("/home/full",&info)&&info.size==FS_FILE_MAX,"oversized write atomic");
    void *old_slots=kmalloc((size_t)FS_V1_SLOT_BYTES*FS_NODES);
    check(old_slots&&!fs_snapshot_export(old_slots,FS_V1_SLOT_BYTES,FS_V1_FILE_MAX),"v1 save rejects large file without truncation");
    kfree(old_slots);kfree(block);
    for(unsigned i=1;i<FS_HANDLES;++i)check(fs_open(files,"/home/full",FS_READ)==(int)i+3,"descriptor capacity");
    check(fs_open(files,"/home/absent",FS_CREATE|FS_WRITE)==-24&&fs_stat("/home/absent",&info)==-2,"descriptor exhaustion no side effects");
    fs_close_all(files);check(!fs_unlink("/home/full"),"descriptor cleanup");
    unsigned created=0;char path[]="/home/n00";
    for(unsigned i=0;i<FS_NODES;++i){path[7]='0'+i/10;path[8]='0'+i%10;int r=fs_mkdir(path);if(r==-28)break;check(r==0,"node capacity create");++created;}
    check(fs_node_count()==FS_NODES,"node capacity reached");
    for(unsigned i=0;i<created;++i){path[7]='0'+i/10;path[8]='0'+i%10;check(!fs_unlink(path),"node cleanup");}
    void *allocations[512];unsigned allocated=0;
    while(allocated<512 && (allocations[allocated]=kmalloc(65536)))++allocated;
    while(allocated<512 && (allocations[allocated]=kmalloc(PAGE_SIZE)))++allocated;
    check(allocated<512,"heap exhaustion reached");
    check(fs_open(files,"/home/oom",FS_WRITE|FS_CREATE)==-12&&fs_node_count()==before,"allocation failure leaves no node");
    while(allocated)kfree(allocations[--allocated]);
    a=fs_open(files,"/home/oom",FS_WRITE|FS_CREATE);check(a==3,"allocation recovery");
    fs_close_all(files);check(!fs_unlink("/home/oom"),"allocation recovery cleanup");
    shm_inode_pool_test();
#endif
    check(fs_node_count()==before,"filesystem resources restored");
    print("PASS: RAMFS paths, binary I/O, offsets, permissions and cleanup\n");
}
