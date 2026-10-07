#include <stdint.h>
#include <stddef.h>
#include "fs-operation-call.h"
struct entry {char name[32];uint32_t type,reserved;uint64_t size;};
static int64_t raw(unsigned operation,uint64_t a,uint64_t b,uint64_t c)
{register uint64_t result __asm__("rax")=operation,extra __asm__("r10")=0;
 __asm__ volatile("int $0x80":"+a"(result):"D"(a),"S"(b),"d"(c),"r"(extra):"memory","cc");return (int64_t)result;}
static void message(const char *s){size_t n=0;while(s[n])++n;(void)raw(1,(uintptr_t)s,n,0);}
static int64_t operation(struct fs_operation_request *r){return raw(56,(uintptr_t)r,sizeof(*r),0);}
static int open_at(int directory,const char *path,unsigned flags)
{struct fs_operation_request r={.operation=FS_CALL_OPEN_AT,.directory=directory,.path=(uintptr_t)path,.flags=flags};return (int)operation(&r);}
static int rename_at(int old_directory,const char *old_path,int new_directory,const char *new_path)
{struct fs_operation_request r={.operation=FS_CALL_RENAME_AT,.directory=old_directory,.path=(uintptr_t)old_path,
 .second_directory=new_directory,.second_path=(uintptr_t)new_path};return (int)operation(&r);}
static int stat_at(int directory,const char *path,struct entry *entry)
{struct fs_operation_request r={.operation=FS_CALL_STAT_AT,.directory=directory,.path=(uintptr_t)path,
 .buffer=(uintptr_t)entry,.length=sizeof(*entry)};return (int)operation(&r);}
#define CHECK(x) do{if(!(x)){message("native FS identity failed: " #x "\n");return __LINE__;}}while(0)
#ifndef HOLD_CASE
#define HOLD_CASE 0
#endif
uint64_t _start(void)
{
#if HOLD_CASE
 int directory=open_at(-100,"/etc",33);CHECK(directory>=3);
 int file=open_at(directory,"motd",1);CHECK(file>=3);
 message("native FS identity resources held\n");
 if(HOLD_CASE==2)__builtin_trap();
 for(;;)(void)raw(4,0,0,0);
#else
 CHECK(!raw(9,(uintptr_t)"/home/id-src",0,0)&&!raw(9,(uintptr_t)"/home/id-dst",0,0));
 int base=open_at(-100,"/home/id-src",33),destination=open_at(-100,"/home/id-dst",33);
 CHECK(base>=3&&destination>=3);
 int source=open_at(base,"item",7),old=open_at(destination,"item",7),alias=open_at(base,".",33);
 CHECK(source>=3&&old>=3&&alias>=3);
 CHECK(raw(7,source,(uintptr_t)"source",6)==6&&raw(7,old,(uintptr_t)"old",3)==3);
 CHECK(!rename_at(-100,"/home/id-src",-100,"/home/id-moved"));
 struct entry info={0};CHECK(stat_at(-100,"/home/id-src/item",&info)==-2);
 CHECK(!stat_at(base,"item",&info)&&info.size==6);
 struct fs_operation_request list={.operation=FS_CALL_LIST_AT,.directory=alias,.path=(uintptr_t)".",
 .buffer=(uintptr_t)&info,.length=sizeof(info)};
 CHECK(operation(&list)==1&&info.name[0]=='i'&&info.size==6);
 list.value=1;info.size=123;CHECK(!operation(&list)&&info.size==123);
 CHECK(stat_at(source,"x",&info)==-20&&stat_at(-1,"item",&info)==-9);
 CHECK(!rename_at(alias,"item",destination,"item"));
 CHECK(stat_at(alias,"item",&info)==-2&&!stat_at(destination,"item",&info)&&info.size==6);
 CHECK(!raw(30,old,(uintptr_t)&info,0)&&info.size==3);
 char bytes[8]={0};CHECK(!raw(29,old,0,0)&&raw(6,old,(uintptr_t)bytes,8)==3&&bytes[0]=='o');
 CHECK(!raw(29,source,0,0)&&raw(6,source,(uintptr_t)bytes,8)==6&&bytes[0]=='s');
 CHECK(rename_at(-100,"/home/id-dst",-100,"/home/id-dst/child")==-22);
 CHECK(rename_at(destination,"item",-100,"/assets/item")==-30);
 /* A rejected user pointer must not partially mutate namespace or output. */
 CHECK(rename_at(destination,"item",-100,(const char *)1)==-14);
 CHECK(!stat_at(destination,"item",&info)&&info.size==6);
 struct fs_operation_request query={.operation=FS_CALL_STAT_AT,.directory=destination,.path=(uintptr_t)"item",
 .buffer=(uintptr_t)_start,.length=sizeof(info)};CHECK(operation(&query)==-14);
 int64_t mapped=raw(14,0,8192,3);CHECK(mapped>0);unsigned char *guard=(void *)(uintptr_t)mapped;
 for(unsigned i=0;i<8192;++i)guard[i]=0xa5;
 CHECK(!raw(16,(uint64_t)mapped+4096,4096,1));
 query.buffer=(uint64_t)mapped+4096-24;CHECK(operation(&query)==-14);
 for(unsigned i=4096-24;i<4096+24;++i)CHECK(guard[i]==0xa5);
 CHECK(raw(56,(uintptr_t)&query,sizeof(query)-1,0)==-22);
 query.operation=99;CHECK(operation(&query)==-22);CHECK(raw(56,1,sizeof(query),0)==-14);
 CHECK(!raw(15,(uint64_t)mapped,8192,0));
 /* Capacity failure is checked before creation, even for relative paths. */
 int fills[8],count=0;
 while(count<8){int n=open_at(destination,"item",1);if(n<0){CHECK(n==-24);break;}fills[count++]=n;}
 CHECK(open_at(alias,"not-created",7)==-24&&stat_at(alias,"not-created",&info)==-2);
 for(int i=0;i<count;++i)CHECK(!raw(8,fills[i],0,0));
 CHECK(!raw(8,base,0,0));int reused=open_at(destination,"item",1);CHECK(reused==base);
 CHECK(stat_at(base,"item",&info)==-20);CHECK(!stat_at(alias,".",&info)&&info.type==2);
 for(int fd=3;fd<11;++fd){int64_t rc=raw(8,fd,0,0);CHECK(rc==0||rc==-9);}
 CHECK(!raw(10,(uintptr_t)"/home/id-dst/item",0,0));
 CHECK(!raw(10,(uintptr_t)"/home/id-moved",0,0)&&!raw(10,(uintptr_t)"/home/id-dst",0,0));
 message("native inode-pinned directory/relative lookup/atomic rename/RW rejection PASS\n");return 89;
#endif
}
