#include "runtime-pack.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void write32(unsigned char *p,uint32_t n){for(unsigned i=0;i<4;++i)p[i]=(unsigned char)(n>>(i*8));}
static unsigned char *load(const char *path,size_t *size)
{FILE *f=fopen(path,"rb");assert(f);assert(!fseek(f,0,SEEK_END));long n=ftell(f);assert(n>0);
 assert(!fseek(f,0,SEEK_SET));unsigned char *p=malloc((size_t)n);assert(p);
 assert(fread(p,1,(size_t)n,f)==(size_t)n);assert(!fclose(f));*size=(size_t)n;return p;}
int main(int argc,char **argv)
{
 assert(argc==3||argc==4);size_t n,size;unsigned char *src=load(argv[1],&n),*raw=load(argv[2],&size);
 unsigned char *guard=malloc(size+32),*saved=malloc(n);assert(guard&&saved);
 memset(guard,0x55,size+32);memcpy(saved,src,n);
 assert(runtime_unpack(src,n,guard+16,size));assert(!memcmp(raw,guard+16,size));
 for(size_t i=0;i<16;++i)assert(guard[i]==0x55&&guard[size+16+i]==0x55);
 assert(!runtime_unpack(0,n,guard+16,size)&&!runtime_unpack(src,n,0,size));
 assert(!runtime_unpack(src,n,guard+16,0)&&!runtime_unpack(src,n,guard+16,65537));
 size_t stride=argc==4?n/32+1:1;
 for(size_t i=0;i<n;i+=stride){memcpy(src,saved,n);src[i]^=0x80;
  (void)runtime_unpack(src,n,guard+16,size);
  for(size_t j=0;j<16;++j)assert(guard[j]==0x55&&guard[size+16+j]==0x55);}
 for(size_t i=16;i<n;i+=stride){memcpy(src,saved,n);write32(src+8,(uint32_t)(i-16));
  assert(!runtime_unpack(src,i,guard+16,size));}
 memcpy(src,saved,n);src[12]^=1;assert(!runtime_unpack(src,n,guard+16,size));
 /* Literal and backreference input exhaustion, zero/forward distance,
  * output overflow, output underfill. Header sizes deliberately agree. */
 unsigned char bad[20]={'M','R','T','1',4,0,0,0,4,0,0,0,0,0,0,0,0x80,0,0,0};
 assert(!runtime_unpack(bad,20,guard+16,4));bad[17]=1;assert(!runtime_unpack(bad,20,guard+16,4));
 bad[16]=127;assert(!runtime_unpack(bad,20,guard+16,4));
 puts(argc==3?"PASS: target decoder exact raw bytes, CRC corruption, all single-byte mutations/canaries, every truncation, invalid bounds/backreferences; ASan/UBSan.":"PASS: boundary payload exact bytes and sampled mutations/truncations; ASan/UBSan.");
 free(saved);free(guard);free(raw);free(src);return 0;
}
