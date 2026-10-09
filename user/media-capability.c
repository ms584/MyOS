/* The native provider's bounded unencrypted VP8/Opus profile. */
#include <string.h>
#include "starboard/configuration.h"
#include "starboard/media.h"
static void trim(const char **a,const char **b){while(*a<*b&&(**a==' '||**a=='\t'))++*a;while(*b>*a&&((*b)[-1]==' '||(*b)[-1]=='\t'))--*b;}
static int match(const char *a,const char *b,const char *text){size_t n=(size_t)(b-a);return strlen(text)==n&&!memcmp(a,text,n);}
static int number(const char *a,const char *b,unsigned maximum,int fraction){
 unsigned n=0;int digits=0;
 while(a<b&&*a>='0'&&*a<='9'){if(n>maximum)return 0;n=n*10+(unsigned)(*a++-'0');digits=1;}
 if(!digits||n>maximum)return 0;
 if(a<b&&*a=='.'&&fraction){++a;int decimal=0;while(a<b&&*a>='0'&&*a<='9'){if(n==maximum&&*a!='0')return 0;++a;++decimal;}if(!decimal)return 0;}
 return a==b&&n>0;
}
SbMediaSupportType SbMediaCanPlayMimeAndKeySystem(const char *mime,const char *key){
 if(!mime||!key||*key)return kSbMediaSupportTypeNotSupported;
 size_t length=0;while(mime[length])if(++length>512)return kSbMediaSupportTypeNotSupported;
 const char *end=mime+length,*cursor=mime,*first=cursor;
 while(cursor<end&&*cursor!=';')++cursor;
 const char *last=cursor;trim(&first,&last);
 int video=match(first,last,"video/webm"),audio=match(first,last,"audio/webm");
 if(!video&&!audio)return kSbMediaSupportTypeNotSupported;
 int have_codecs=0,have_vp8=0,have_opus=0;
 while(cursor<end){
  ++cursor;const char *name=cursor;
  while(cursor<end&&*cursor!='='&&*cursor!=';')++cursor;
  if(cursor==end||*cursor!='=')return kSbMediaSupportTypeNotSupported;
  const char *name_end=cursor++;trim(&name,&name_end);const char *value=cursor;
  if(cursor<end&&*cursor=='"'){
   value=++cursor;while(cursor<end&&*cursor!='"')++cursor;
   if(cursor==end)return kSbMediaSupportTypeNotSupported;
   last=cursor++;while(cursor<end&&(*cursor==' '||*cursor=='\t'))++cursor;
   if(cursor<end&&*cursor!=';')return kSbMediaSupportTypeNotSupported;
  }else{while(cursor<end&&*cursor!=';')++cursor;last=cursor;}
  trim(&value,&last);
  if(match(name,name_end,"codecs")){
   if(have_codecs++||value==last)return kSbMediaSupportTypeNotSupported;
   while(value<last){
    const char *a=value;while(value<last&&*value!=',')++value;
    const char *b=value;trim(&a,&b);
    if(match(a,b,"vp8")||match(a,b,"vp8.0")){if(have_vp8++)return kSbMediaSupportTypeNotSupported;}
    else if(match(a,b,"opus")){if(have_opus++)return kSbMediaSupportTypeNotSupported;}
    else return kSbMediaSupportTypeNotSupported;
    if(value<last){++value;if(value==last)return kSbMediaSupportTypeNotSupported;}
   }
  }else if(match(name,name_end,"width")){if(!number(value,last,160,0))return kSbMediaSupportTypeNotSupported;}
  else if(match(name,name_end,"height")){if(!number(value,last,90,0))return kSbMediaSupportTypeNotSupported;}
  else if(match(name,name_end,"framerate")){if(!number(value,last,15,1))return kSbMediaSupportTypeNotSupported;}
  else if(match(name,name_end,"channels")){if(!number(value,last,2,0))return kSbMediaSupportTypeNotSupported;}
 }
 if(audio&&have_vp8)return kSbMediaSupportTypeNotSupported;
 return have_codecs?kSbMediaSupportTypeProbably:kSbMediaSupportTypeMaybe;
}
bool SbMediaCanChangeType(const char *from,const char *to){(void)from;(void)to;return false;}
