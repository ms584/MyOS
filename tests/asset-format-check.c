#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
static unsigned char *disk_bytes;
static size_t disk_size;
static unsigned live_allocations;
static bool fail_alloc;
void *kmalloc(size_t size){if(fail_alloc)return 0;void *p=malloc(size);if(p)++live_allocations;return p;}
void kfree(void *p){if(p){assert(live_allocations);--live_allocations;free(p);}}
bool ata_asset_read(uint32_t sector,void *buffer)
{if((uint64_t)sector*512+512>disk_size)return false;memcpy(buffer,disk_bytes+(size_t)sector*512,512);return true;}
uint32_t ata_asset_sectors(void){return (uint32_t)(disk_size/512);}
void print(const char *text){(void)text;}
void decimal(uint64_t value){(void)value;}
#define putchar myos_kernel_putchar
#include "../fs/asset.c"
#undef putchar
int main(int argc,char **argv)
{
 assert(argc==3);FILE *f=fopen(argv[1],"rb");assert(f);assert(!fseek(f,0,SEEK_END));long n=ftell(f);assert(n>0);
 assert(!fseek(f,0,SEEK_SET));disk_size=(size_t)n;disk_bytes=malloc(disk_size);assert(disk_bytes);
 assert(fread(disk_bytes,1,disk_size,f)==disk_size);assert(!fclose(f));
 struct asset_file file={UINT64_MAX,UINT64_MAX};
 if(!strcmp(argv[2],"bad")){
  assert(asset_open("root.txt",&file)==-8);assert(file.offset==UINT64_MAX&&file.size==UINT64_MAX);
  assert(!catalogue&&!live_allocations);
 }else if(!strcmp(argv[2],"oom")){
  fail_alloc=true;assert(asset_open("root.txt",&file)==-12);
  assert(file.offset==UINT64_MAX&&!live_allocations);fail_alloc=false;
  assert(!asset_open("root.txt",&file));assert(live_allocations==1);
 }else if(!strcmp(argv[2],"v1")){
  assert(!asset_open("root.txt",&file));assert(asset_runtime_open(&file)==-8);
  assert(!catalogue&&!live_allocations);
 }else if(!strcmp(argv[2],"many")){
  assert(!asset_open("root.txt",&file)&&file.size==7);
  assert(mounted.base.count==1024&&live_allocations==1);
  assert(!asset_open("file0000.bin",&file)&&!asset_open("file1022.bin",&file));
  assert(asset_open("file1023.bin",&file)==-2);
 }else{
  assert(!asset_open("root.txt",&file));assert(file.size==7);
  char payload[8]={0};assert(asset_read(&file,0,payload,7));assert(!memcmp(payload,"payload",7));
  assert(!asset_read(&file,7,payload,1));assert(!asset_open("content/fonts/font.bin",&file));
  assert(!asset_directory("content")&&!asset_directory("content/fonts"));
  assert(asset_directory("missing")==-2);
  char name[48];uint64_t size;unsigned type;
  assert(asset_child("",0,name,&size,&type)==1&&type==2&&!strcmp(name,"content"));
  assert(asset_child("",1,name,&size,&type)==1&&type==1&&!strcmp(name,"root.txt"));
  assert(asset_child("",2,name,&size,&type)==0);
  assert(asset_child("content",0,name,&size,&type)==1&&type==1&&!strcmp(name,"cert.pem"));
  assert(asset_child("content",1,name,&size,&type)==1&&type==2&&!strcmp(name,"fonts"));
  assert(asset_child("content",2,name,&size,&type)==1&&type==2&&!strcmp(name,"icu"));
  assert(asset_child("root.txt",0,name,&size,&type)==-20);
  assert(!asset_runtime_open(&file)&&file.size==16);
  assert(live_allocations==1);
  for(unsigned i=0;i<40;++i)assert(!asset_open("root.txt",&file)&&live_allocations==1);
 }
 if(catalogue){kfree(catalogue);catalogue=0;}assert(!live_allocations);
 free(disk_bytes);puts("PASS: asset v2 host parser/namespace/bounds/identity extent/cache/OOM; NOT native.");return 0;
}
