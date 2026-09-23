#include "fs.h"
#ifndef PROCESS_TESTS
#define PROCESS_TESTS 0
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
    check(fs_unlink("/home/test/file")==-16&&fs_unlink("/home/test")==-39,"busy and nonempty");
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
    char longpath[FS_PATH+1];memset(longpath,'a',sizeof(longpath));longpath[0]='/';longpath[FS_PATH]=0;
    check(fs_mkdir(longpath)==-36,"path length bound");longpath[33]=0;
    check(fs_mkdir(longpath)==-36,"component length bound");
    a=fs_open(files,"/home/full",FS_WRITE|FS_CREATE);
    char *block=kmalloc(FS_FILE_MAX);check(block!=0,"test buffer");memset(block,0xa5,FS_FILE_MAX);
    check(fs_write(files,a,block,FS_FILE_MAX)==FS_FILE_MAX&&fs_write(files,a,"!",1)==-28,"file size capacity");
    check(!fs_stat("/home/full",&info)&&info.size==FS_FILE_MAX,"oversized write atomic");kfree(block);
    for(unsigned i=1;i<FS_HANDLES;++i)check(fs_open(files,"/home/full",FS_READ)==(int)i+3,"descriptor capacity");
    check(fs_open(files,"/home/absent",FS_CREATE|FS_WRITE)==-24&&fs_stat("/home/absent",&info)==-2,"descriptor exhaustion no side effects");
    fs_close_all(files);check(!fs_unlink("/home/full"),"descriptor cleanup");
    unsigned created=0;char path[]="/home/n00";
    for(unsigned i=0;i<FS_NODES;++i){path[7]='0'+i/10;path[8]='0'+i%10;int r=fs_mkdir(path);if(r==-28)break;check(r==0,"node capacity create");++created;}
    check(fs_node_count()==FS_NODES,"node capacity reached");
    for(unsigned i=0;i<created;++i){path[7]='0'+i/10;path[8]='0'+i%10;check(!fs_unlink(path),"node cleanup");}
    void *allocations[256];unsigned allocated=0;
    while(allocated<256 && (allocations[allocated]=kmalloc(FS_FILE_MAX)))++allocated;
    check(allocated<256,"heap exhaustion reached");
    check(fs_open(files,"/home/oom",FS_WRITE|FS_CREATE)==-12&&fs_node_count()==before,"allocation failure leaves no node");
    while(allocated)kfree(allocations[--allocated]);
    a=fs_open(files,"/home/oom",FS_WRITE|FS_CREATE);check(a==3,"allocation recovery");
    fs_close_all(files);check(!fs_unlink("/home/oom"),"allocation recovery cleanup");
#endif
    check(fs_node_count()==before,"filesystem resources restored");
    print("PASS: RAMFS paths, binary I/O, offsets, permissions and cleanup\n");
}
