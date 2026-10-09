#include "media-webm.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
struct reader {
 const struct myos_webm_source *source;uint64_t pos,end[16],scale,cluster;
 unsigned depth,webm;unsigned char *packet;
 unsigned track,kind,video,audio;int in_entry;uint64_t entry_end,cluster_end;
 myos_webm_packet deliver;void *context;
};
static int take(struct reader *r,void *p,size_t n){
 if(n>r->source->length-r->pos||(r->depth&&n>r->end[r->depth-1]-r->pos))return -1;
 if(r->source->read(r->source->context,p,n))return -1;
 r->pos+=n;return 0;
}
static int vint(struct reader *r,uint64_t *out,int keep,unsigned max){
 unsigned char b;if(take(r,&b,1)||!b)return -1;
 unsigned n=1,mask=128;while(!(b&mask)){++n;mask>>=1;}
 if(n>max)return -1;
 uint64_t x=keep?b:(b&(mask-1));
 for(unsigned i=1;i<n;++i){if(take(r,&b,1))return -1;x=(x<<8)|b;}
 if(!keep&&x==((UINT64_C(1)<<(7*n))-1))*out=UINT64_MAX;
 else *out=x;return 0;
}
static int skip(struct reader *r,uint64_t n){
 unsigned char buffer[1024];
 while(n){size_t k=n>sizeof(buffer)?sizeof(buffer):(size_t)n;if(take(r,buffer,k))return -1;n-=k;}
 return 0;
}
static int integer(struct reader *r,uint64_t n,uint64_t *v){
 if(!n||n>8)return -1;*v=0;
 while(n--){unsigned char b;if(take(r,&b,1))return -1;*v=(*v<<8)|b;}return 0;
}
static int master(uint64_t id){
 return id==0x1a45dfa3||id==0x18538067||id==0x1549a966||id==0x1654ae6b||
 id==0xae||id==0xe0||id==0xe1||id==0x1f43b675||id==0xa0;
}
static int parse(struct reader *r){
 r->end[0]=r->source->length;r->depth=1;r->scale=1000000;
 while(r->depth){
  while(r->depth&&r->pos==r->end[r->depth-1]){
   --r->depth;
   /* TrackEntry is parsed before any media clusters. Track fields are local
    * until the entry ends; audio/video numbers are committed below on CodecID. */
  }
  if(!r->depth)break;
  if(r->in_entry&&r->pos>=r->entry_end)r->in_entry=0;
  uint64_t id,n;
  if(vint(r,&id,1,4)||vint(r,&n,0,8))return -1;
  uint64_t parent=r->end[r->depth-1];
  if(r->pos>parent)return -1;
  if(n==UINT64_MAX){if(id!=0x18538067)return -1;n=parent-r->pos;}
  if(n>parent-r->pos)return -1;
  if(master(id)){
   if(r->depth==16)return -1;
   if(id==0xae){r->track=r->kind=0;r->in_entry=1;r->entry_end=r->pos+n;}
   if(id==0x1f43b675){if(!r->webm||!r->video||!r->audio)return -1;r->in_entry=0;r->cluster=0;r->cluster_end=r->pos+n;}
   r->end[r->depth++]=r->pos+n;continue;
  }
  if(id==0x4282){
   unsigned char name[4];if(n!=4||take(r,name,4)||memcmp(name,"webm",4))return -1;r->webm=1;
  }else if(id==0x2ad7b1){
   if(integer(r,n,&r->scale)||!r->scale||r->scale>1000000000)return -1;
  }else if(id==0xd7&&r->in_entry){
   uint64_t number;if(integer(r,n,&number)||!number||number>126)return -1;r->track=(unsigned)number;
  }else if(id==0x86&&r->in_entry){
   unsigned char name[16];
   if(!r->track||n>sizeof(name)||take(r,name,(size_t)n))return -1;
   if(n==5&&!memcmp(name,"V_VP8",5)){if(r->video)return -1;r->video=r->track;r->kind=1;}
   else if(n==6&&!memcmp(name,"A_OPUS",6)){if(r->audio)return -1;r->audio=r->track;r->kind=2;}
   else return -1;
  }else if(id==0xe7){
   if(integer(r,n,&r->cluster))return -1;
  }else if(id==0xa3||id==0xa1){
   if(!r->cluster_end||r->pos>=r->cluster_end||n<5||n>MYOS_WEBM_PACKET_CAP||take(r,r->packet,(size_t)n))return -1;
   /* This deliberately bounded subset accepts one-byte track VINT and no lacing. */
   unsigned char *p=r->packet;
   if(!(p[0]&128)||(p[3]&6))return -1;
   unsigned track=p[0]&127;int type=track==r->video?1:track==r->audio?2:0;
   if(!type)return -1;
   int rel=(int16_t)(((unsigned)p[1]<<8)|p[2]);
   if(r->cluster>INT64_MAX/1000000000)return -1;
   int64_t ticks=(int64_t)r->cluster+rel;
   if(ticks>INT64_MAX/(int64_t)r->scale||ticks<INT64_MIN/(int64_t)r->scale)return -1;
   int64_t us=ticks*(int64_t)r->scale/1000;
   if(r->deliver(r->context,type,p+4,(size_t)n-4,us,(p[3]&128)!=0))return -3;
  }else if(id==0x6d80)return -1; /* ContentEncodings unsupported. */
  else if(skip(r,n))return -1;
 }
 return r->webm&&r->video&&r->audio?0:-1;
}
int myos_webm_demux(const struct myos_webm_source *s,myos_webm_packet deliver,void *context){
 if(!s||!s->read||!deliver||!s->length)return -1;
 struct reader r={0};r.source=s;r.deliver=deliver;r.context=context;
 r.packet=malloc(MYOS_WEBM_PACKET_CAP);if(!r.packet)return -2;
 int result=parse(&r);free(r.packet);return result;
}
