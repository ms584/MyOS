#define main backing_unit_main
#include "shm-backing-unit.c"
#undef main
#include "fs-snapshot.h"
static unsigned heap_blocks;
static bool deny_heap;
void *kmalloc(size_t size){if(deny_heap)return 0;void *p=malloc(size);if(p)++heap_blocks;return p;}
void kfree(void *p){if(p){assert(heap_blocks);--heap_blocks;free(p);}}
void check(bool condition,const char *message){if(!condition)fprintf(stderr,"%s\n",message);assert(condition);}
void print(const char *s){(void)s;}
int asset_open(const char *s,struct asset_file *f){(void)s;(void)f;return -2;}
int asset_directory(const char *s){(void)s;return -2;}
int asset_child(const char *s,unsigned i,char n[48],uint64_t *z,unsigned *t){(void)s;(void)i;(void)n;(void)z;(void)t;return -2;}
bool asset_read(void *f,uint64_t o,void *b,size_t n){(void)f;(void)o;(void)b;(void)n;return false;}
int asset_stat(const char *s,uint64_t *z){(void)s;(void)z;return -2;}
static void identity_shared_tests(void)
{
 struct fs_handle files[FS_HANDLES]={0};struct fs_entry entry;
 unsigned base_heap=heap_blocks,base_nodes=fs_node_count();assert(!live);
 assert(!fs_mkdir("/home/dir TEzLw=="));
 int auto_dir=fs_open(files,"/home/dir TEzLw==",FS_READ|FS_AUTO_DIRECTORY);
 assert(auto_dir>=3&&!fs_fstat(files,auto_dir,&entry)&&entry.type==FS_DIR);
 assert(fs_open(files,"/home/dir TEzLw==",FS_WRITE|FS_AUTO_DIRECTORY)==-22);
 assert(!fs_close(files,auto_dir));
 int dir=fs_open(files,"/home/dir TEzLw==",FS_READ|FS_DIRECTORY);assert(dir>=3);
 assert(fs_read(files,dir,0,0)==-21&&fs_seek(files,dir,0,0)==-29);
 int source=fs_open_at(files,dir,"source",FS_READ|FS_WRITE|FS_CREATE);assert(source>=3);
 assert(fs_write(files,source,"source",6)==6&&!fs_close(files,source));
 int auto_file=fs_open(files,"/home/dir TEzLw==/source",FS_READ|FS_AUTO_DIRECTORY);
 assert(auto_file>=3&&!fs_fstat(files,auto_file,&entry)&&entry.type==FS_FILE&&!fs_close(files,auto_file));
 deny_heap=true;assert(!fs_rename("/home/dir TEzLw==","/home/moved TEzLw=="));deny_heap=false;
 assert(fs_stat("/home/dir TEzLw==",&entry)==-2);
 assert(!fs_stat_at(files,dir,"source",&entry)&&entry.size==6);
 assert(fs_list_at(files,dir,".",0,&entry)==1&&!strcmp(entry.name,"source"));
 source=fs_open_at(files,dir,"source",FS_READ);assert(source>=3);
 int target=fs_open_at(files,dir,"target",FS_READ|FS_WRITE|FS_CREATE);assert(target>=3);
 assert(fs_write(files,target,"old",3)==3&&fs_seek(files,target,0,0)==0);
 deny_heap=true;assert(!fs_rename_at(files,dir,"source",dir,"target"));deny_heap=false;
 assert(fs_stat_at(files,dir,"source",&entry)==-2&&!fs_stat_at(files,dir,"target",&entry)&&entry.size==6);
 char bytes[8]={0};assert(fs_read(files,target,bytes,8)==3&&!memcmp(bytes,"old",3));
 assert(fs_read(files,source,bytes,8)==6&&!memcmp(bytes,"source",6));
 assert(!fs_fstat(files,target,&entry)&&entry.size==3);
 assert(!fs_close(files,target)&&!fs_close(files,source));
 assert(fs_rename("/home/moved TEzLw==","/home/moved TEzLw==/child")==-22);
 assert(fs_rename("/","/home/root")==-16);
 assert(fs_rename("/assets/read-only","/home/nope")==-30);
 assert(fs_unlink_at(files,dir,"target",1)==-20);
 assert(fs_unlink_at(files,dir,".",0)==-21);
 assert(fs_unlink_at(files,dir,"target",2)==-22);
 assert(fs_unlink_at(files,2,"target",0)==-9);
 target=fs_open_at(files,dir,"target",FS_READ);assert(target>=3);
 assert(fs_unlink_at(files,target,"target",0)==-20);
 deny_heap=true;assert(!fs_unlink_at(files,dir,"target",0));deny_heap=false;
 assert(fs_stat_at(files,dir,"target",&entry)==-2);
 assert(!fs_fstat(files,target,&entry)&&entry.size==6&&!fs_close(files,target));
 assert(!fs_close(files,dir));
 assert(fs_unlink_at(files,2,"/home/moved TEzLw==",0)==-21);
 assert(!fs_unlink_at(files,2,"/home/moved TEzLw==",1));
 assert(fs_unlink_at(files,2,"/assets/read-only",0)==-30);
 /* A replaced empty directory's open handle retains its original inode. */
 assert(!fs_mkdir("/home/first")&&!fs_mkdir("/home/second"));
 dir=fs_open(files,"/home/second",FS_READ|FS_DIRECTORY);assert(dir>=3);
 assert(!fs_rename("/home/first","/home/second")&&!fs_fstat(files,dir,&entry)&&entry.type==FS_DIR);
 assert(!fs_close(files,dir)&&!fs_unlink("/home/second"));
 /* Both mapped and open replaced shared inodes keep their own frames. */
 source=fs_open(files,"/dev/shm/first",FS_READ|FS_WRITE|FS_CREATE);
 target=fs_open(files,"/dev/shm/second",FS_READ|FS_WRITE|FS_CREATE);assert(source>=3&&target>=3);
 assert(fs_write(files,source,"new",3)==3&&fs_write(files,target,"old",3)==3&&live==2);
 uintptr_t alias=fs_shared_page(files,target,0);assert(alias);
 assert(fs_rename("/dev/shm/first","/home/cross")==-18&&
        fs_rename("/dev","/home/dev")==-18&&fs_rename("/dev/shm","/dev/other")==-18);
 assert(!fs_stat("/dev/shm/first",&entry)&&fs_stat("/home/cross",&entry)==-2);
 deny_heap=true;assert(!fs_rename("/dev/shm/first","/dev/shm/second"));deny_heap=false;
 assert(!fs_fstat(files,target,&entry)&&entry.reserved==2&&entry.size==3);
 assert(!memcmp((void *)alias,"old",3)&&!fs_close(files,target)&&live==2);
 assert(!fs_unlink("/dev/shm/second")&&!fs_close(files,source)&&live==1);
 assert(!memcmp((void *)alias,"old",3));page_free(alias);assert(!live);
 source=fs_open(files,"/home/ordinary",FS_WRITE|FS_CREATE);assert(source>=3&&!fs_close(files,source));
 assert(fs_rename("/home/ordinary","/dev/shm/ordinary")==-18&&!fs_unlink("/home/ordinary"));
 assert(heap_blocks==base_heap&&fs_node_count()==base_nodes);
 puts("PASS merged inode/shared rename: moved dir bases, replaced open inodes/frames, no-allocation commit, namespace EXDEV, cycles/root/readonly guards, exact heap/pages/nodes");
}
static void inode_lock_tests(void)
{
 struct fs_handle a[FS_HANDLES]={0},b[FS_HANDLES]={0};
 unsigned base_heap=heap_blocks,base_nodes=fs_node_count();
 int first=fs_open(a,"/home/locked",FS_READ|FS_WRITE|FS_CREATE);
 int second=fs_open(b,"/home/locked",FS_READ|FS_WRITE);assert(first>=3&&second>=3);
 struct record_lock_abi lock={RECORD_WRITE,0,0xa5a5a5a5,0,0,777,0xa5a5a5a5};
 assert(!fs_record_lock(a,first,11,RECORD_SET,&lock));
 struct record_lock_abi query={RECORD_READ,0,0,0,0,999,0};
 assert(!fs_record_lock(b,second,22,RECORD_GET,&query)&&query.type==RECORD_WRITE&&query.pid==11);
 query.type=RECORD_WRITE;assert(fs_record_lock(b,second,22,RECORD_SET,&query)==-11);
 assert(!fs_rename("/home/locked","/home/moved-lock"));
 query.type=RECORD_READ;assert(!fs_record_lock(b,second,22,RECORD_GET,&query)&&query.pid==11);
 int other=fs_open(a,"/home/replacement",FS_READ|FS_WRITE|FS_CREATE);assert(other>=3);
 assert(!fs_rename("/home/replacement","/home/moved-lock"));
 query=(struct record_lock_abi){RECORD_WRITE,0,0,0,0,0,0};
 assert(!fs_record_lock(a,other,11,RECORD_GET,&query)&&query.type==RECORD_UNLOCK);
 query=(struct record_lock_abi){RECORD_WRITE,0,0,0,0,0,0};
 assert(!fs_record_lock(a,other,11,RECORD_SET,&query));
 assert(!fs_close(a,first));query.type=RECORD_WRITE;
 assert(!fs_record_lock(b,second,22,RECORD_GET,&query)&&query.type==RECORD_UNLOCK);
 assert(!fs_close(b,second));
 /* Normalization uses actual fd offset/size. Negative length locks the bytes
  * preceding l_start; zero length reaches future EOF, not current file size. */
 int same=fs_open(b,"/home/moved-lock",FS_READ|FS_WRITE);assert(same>=3);
 assert(fs_seek(a,other,100,0)==100);
 query=(struct record_lock_abi){RECORD_UNLOCK,0,0,0,0,0,0};assert(!fs_record_lock(a,other,11,RECORD_SET,&query));
 query=(struct record_lock_abi){RECORD_WRITE,1,0,0,-20,0,0};assert(!fs_record_lock(a,other,11,RECORD_SET,&query));
 query=(struct record_lock_abi){RECORD_READ,0,0,90,1,0,0};
 assert(!fs_record_lock(b,same,22,RECORD_GET,&query)&&query.start==80&&query.length==20&&query.pid==11);
 query=(struct record_lock_abi){RECORD_WRITE,0,0,-1,1,0,0};struct record_lock_abi before=query;
 assert(fs_record_lock(a,other,11,RECORD_SET,&query)==-22&&!memcmp(&query,&before,sizeof(query)));
 query=(struct record_lock_abi){RECORD_WRITE,1,0,INT64_MAX,1,0,0};
 assert(fs_record_lock(a,other,11,RECORD_SET,&query)==-75);
 /* Closing ANY fd on that inode releases that process's record locks. */
 int duplicate=fs_open(a,"/home/moved-lock",FS_READ);assert(duplicate>=3&&!fs_close(a,duplicate));
 query=(struct record_lock_abi){RECORD_READ,0,0,90,1,0,0};
 assert(!fs_record_lock(b,same,22,RECORD_GET,&query)&&query.type==RECORD_UNLOCK);
 int readonly=fs_open(a,"/home/moved-lock",FS_READ);assert(readonly>=3);
 query=(struct record_lock_abi){RECORD_WRITE,0,0,0,0,0,0};assert(fs_record_lock(a,readonly,11,RECORD_SET,&query)==-9);
 query.type=RECORD_READ;assert(!fs_record_lock(a,readonly,11,RECORD_SET,&query));
 assert(!fs_record_release(a,other));query.type=RECORD_WRITE;
 assert(!fs_record_lock(b,same,22,RECORD_GET,&query)&&query.type==RECORD_UNLOCK);
 assert(!fs_close(a,readonly)&&!fs_close(a,other)&&!fs_close(b,same)&&!fs_unlink("/home/moved-lock"));
 assert(heap_blocks==base_heap&&fs_node_count()==base_nodes&&!live);
 puts("PASS real RAMFS record locks: inode/owner conflicts survive rename/replacement, close-any-fd release, SEEK_CUR/negative length/overflow/rights, exact teardown; runtime dup-close hook still required");
}
int main(void)
{
 backing_unit_main();fs_init();assert(heap_blocks==1&&!live);
 struct fs_handle table[FS_HANDLES]={0};struct fs_entry info;
 assert(!fs_mkdir("/dev/shm"));
 identity_shared_tests();
 inode_lock_tests();
 int fd=fs_open(table,"/dev/shm/cache TEzLw==",FS_READ|FS_WRITE|FS_CREATE);assert(fd>=3&&heap_blocks==2);
 assert(!fs_truncate(table,fd,262144)&&live==64);
 assert(!fs_fstat(table,fd,&info)&&info.size==262144&&info.reserved==2);
 assert(fs_write(table,fd,"test",4)==4);uintptr_t alias=fs_shared_page(table,fd,0);assert(alias);
 ((char *)alias)[0]='X';assert(fs_seek(table,fd,0,0)==0);
 char bytes[4];assert(fs_read(table,fd,bytes,4)==4&&bytes[0]=='X');
 assert(fs_truncate(table,fd,0)==-16&&fs_open(table,"/dev/shm/cache TEzLw==",FS_WRITE|FS_TRUNC)==-16);
 unsigned rights=0;assert(!fs_shared_rights(table,fd,&rights)&&rights==3);
 allocations_before_failure=1;assert(fs_truncate(table,fd,1048576)==-12&&live==64);
 assert(!fs_fstat(table,fd,&info)&&info.size==262144);allocations_before_failure=-1;
 for(unsigned version=1;version<=2;++version){
  size_t stride=version==1?FS_V1_SLOT_BYTES:FS_V2_SLOT_BYTES,max=version==1?FS_V1_FILE_MAX:FS_FILE_MAX;
  unsigned char *snapshot=calloc(FS_NODES,stride);assert(snapshot);
  assert(fs_snapshot_export(snapshot,stride,max)&&fs_snapshot_valid(snapshot,stride,max));
  for(unsigned i=1;i<FS_NODES;++i){struct fs_disk_meta *meta=(void *)(snapshot+i*stride);
   assert(!meta->used||strcmp(meta->name,"shm"));assert(!meta->used||strcmp(meta->name,"cache TEzLw=="));}
  assert(!fs_snapshot_import(snapshot,stride,max)); /* Open inode transaction guard. */
  free(snapshot);
 }
 assert(!fs_unlink("/dev/shm/cache TEzLw==")&&!fs_close(table,fd)&&heap_blocks==1&&live==1);
 assert(((char *)alias)[0]=='X');page_free(alias);assert(!live);
 fd=fs_open(table,"/home/normal",FS_READ|FS_WRITE|FS_CREATE);assert(fd>=3);
 assert(fs_truncate(table,fd,FS_FILE_MAX+1)==-27&&!fs_truncate(table,fd,FS_FILE_MAX));
 assert(fs_seek(table,fd,FS_FILE_MAX,0)==FS_FILE_MAX&&fs_write(table,fd,"x",1)==-28);
 assert(fs_shared_rights(table,fd,&rights)==-95&&!fs_shared_page(table,fd,0));
 assert(!fs_close(table,fd)&&!fs_unlink("/home/normal")&&heap_blocks==1);
 deny_heap=true;assert(fs_open(table,"/dev/shm/failure",FS_WRITE|FS_CREATE)==-12);deny_heap=false;
 assert(fs_stat("/dev/shm/failure",&info)==-2&&heap_blocks==1&&!live);
 size_t stride=FS_V2_SLOT_BYTES;unsigned char *snapshot=calloc(FS_NODES,stride);assert(snapshot);
 assert(fs_snapshot_export(snapshot,stride,FS_FILE_MAX));
 assert(fs_snapshot_import(snapshot,stride,FS_FILE_MAX));assert(heap_blocks==1&&!live);
 assert(fs_stat("/dev/shm",&info)==-2); /* Ephemeral mount not serialized. */
 assert(fs_snapshot_export(snapshot,stride,FS_FILE_MAX));
 struct fs_disk_meta *motd=0;for(unsigned i=1;i<FS_NODES;++i){struct fs_disk_meta *m=(void *)(snapshot+i*stride);if(m->used&&!strcmp(m->name,"motd")){motd=m;break;}}
 assert(motd&&motd->size==sizeof("Welcome to MyOS RAMFS.\n")-1);free(snapshot);
 assert(!fs_unlink("/etc/motd")&&!heap_blocks&&!live);
 puts("PASS real RAMFS/shared integration: ephemeral v1/v2 export/import, normal64KiB bounds, fd/mapping coherence/lifetime, mapped O_TRUNC rejection and allocation rollback; host PMM boundary mock");
}
