#include "fs-snapshot.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
static unsigned allocations;
static bool deny_allocation;
void *kmalloc(size_t n){if(deny_allocation)return 0;void *p=malloc(n);if(p)++allocations;return p;}
void kfree(void *p){if(p){assert(allocations);--allocations;free(p);}}
void check(bool ok,const char *message){if(!ok)fprintf(stderr,"%s\n",message);assert(ok);}
void print(const char *message){(void)message;}
int asset_open(const char *s,struct asset_file *f){(void)s;(void)f;return -2;}
int asset_directory(const char *s){(void)s;return -2;}
int asset_child(const char *s,unsigned p,char n[48],uint64_t *z,unsigned *t)
{(void)s;(void)p;(void)n;(void)z;(void)t;return -2;}
bool asset_read(void *f,uint64_t o,void *b,size_t n){(void)f;(void)o;(void)b;(void)n;return false;}
int asset_stat(const char *s,uint64_t *z){(void)s;(void)z;return -2;}
int main(void)
{
 fs_init();assert(allocations==1);unsigned baseline=fs_node_count();
 struct fs_handle files[FS_HANDLES]={0};struct fs_entry info;
 assert(!fs_mkdir("/home/TEzLw=="));
 const char *path="/home/TEzLw==/Experiment Config";
 int fd=fs_open(files,path,FS_READ|FS_WRITE|FS_CREATE);assert(fd==3);
 assert(fs_write(files,fd,"config",6)==6&&!fs_close(files,fd));
 assert(!fs_stat(path,&info)&&info.size==6&&info.type==FS_FILE);
 for(unsigned c=32;c<127;++c){
  if(c=='/')continue;char component[]="/home/pX";component[7]=(char)c;
  assert(!fs_mkdir(component)&&!fs_stat(component,&info)&&info.type==FS_DIR);
  assert(!fs_unlink(component));
 }
 for(unsigned c=1;c<256;++c){
  if(c>=32&&c<127)continue;char invalid[]="/home/nX";invalid[7]=(char)c;
  assert(fs_mkdir(invalid)==-22&&fs_node_count()==baseline+2);
 }
 char bound[40]="/home/";memset(bound+6,'a',32);bound[38]=0;
 assert(fs_mkdir(bound)==-36);bound[37]=0;assert(!fs_mkdir(bound)&&!fs_unlink(bound));
 for(unsigned version=1;version<=2;++version){
  size_t stride=version==1?FS_V1_SLOT_BYTES:FS_V2_SLOT_BYTES;
  size_t limit=version==1?FS_V1_FILE_MAX:FS_FILE_MAX;
  void *slots=malloc(stride*FS_NODES);assert(slots);
  assert(fs_snapshot_export(slots,stride,limit)&&fs_snapshot_valid(slots,stride,limit));
  struct fs_disk_meta *meta=(struct fs_disk_meta *)((unsigned char *)slots+stride);
  char first=meta->name[0];meta->name[0]='/';assert(!fs_snapshot_valid(slots,stride,limit));
  meta->name[0]='\n';assert(!fs_snapshot_valid(slots,stride,limit));
  meta->name[0]=(char)0x80;assert(!fs_snapshot_valid(slots,stride,limit));meta->name[0]=first;
  deny_allocation=true;assert(!fs_snapshot_import(slots,stride,limit));deny_allocation=false;
  assert(!fs_stat(path,&info)&&info.size==6&&allocations==2);
  assert(fs_snapshot_import(slots,stride,limit));free(slots);
  fd=fs_open(files,path,FS_READ);assert(fd==3);char bytes[8]={0};
  assert(fs_read(files,fd,bytes,sizeof(bytes))==6&&!memcmp(bytes,"config",6));
  assert(!fs_close(files,fd)&&allocations==2);
 }
 assert(!fs_unlink(path)&&!fs_unlink("/home/TEzLw==")&&fs_node_count()==baseline);
 assert(!fs_unlink("/etc/motd")&&!allocations);
 puts("PASS real RAMFS printable names, Cobalt cache/config, v1/v2 snapshots, invalid controls/separator/bounds and OOM rollback");
}
