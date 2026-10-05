#include "runtime-pack.h"
#include <stdint.h>
static uint32_t read32(const unsigned char *p)
{return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
bool runtime_unpack(const void *source,size_t bytes,void *destination,size_t capacity)
{
 const unsigned char *src=source;unsigned char *dst=destination;
 if(!src||!dst||bytes<16||!capacity||capacity>RUNTIME_PACK_LIMIT)return false;
 if(src[0]!='M'||src[1]!='R'||src[2]!='T'||src[3]!='1'||read32(src+4)!=capacity||
    read32(src+8)!=bytes-16)return false;
 size_t input=16,output=0;
 while(input<bytes){
  unsigned token=src[input++];size_t count;
  if(token<128){count=token+1;
   if(count>bytes-input||count>capacity-output)return false;
   for(size_t i=0;i<count;++i)dst[output+i]=src[input+i];input+=count;output+=count;
  }else{count=(token&127)+3;
   if(bytes-input<2||count>capacity-output)return false;
   size_t distance=(unsigned)src[input]|(unsigned)src[input+1]<<8;input+=2;
   if(!distance||distance>output)return false;
   for(size_t i=0;i<count;++i)dst[output+i]=dst[output+i-distance];output+=count;
  }
 }
 if(output!=capacity)return false;
 uint32_t crc=~0u;
 for(size_t i=0;i<capacity;++i){crc^=dst[i];
  for(unsigned bit=0;bit<8;++bit)crc=(crc>>1)^((0u-(crc&1u))&0xedb88320u);}
 return (crc^~0u)==read32(src+12);
}
