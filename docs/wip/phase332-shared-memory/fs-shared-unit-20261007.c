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
int main(void)
{
 backing_unit_main();fs_init();assert(heap_blocks==1&&!live);
 struct fs_handle table[FS_HANDLES]={0};struct fs_entry info;
 assert(!fs_mkdir("/dev/shm"));
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
