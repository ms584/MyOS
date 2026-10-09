/* The native provider's bounded unencrypted VP8/Opus profile. */
#include <string.h>
#include "starboard/configuration.h"
#include "starboard/media.h"
#ifdef MYOS_MEDIA_TRACE
#include <stdio.h>
#include "starboard/log.h"
#endif
static void trim(const char **a,const char **b){while(*a<*b&&(**a==' '||**a=='\t'))++*a;while(*b>*a&&((*b)[-1]==' '||(*b)[-1]=='\t'))--*b;}
static int match(const char *a,const char *b,const char *text){size_t n=(size_t)(b-a);return strlen(text)==n&&!memcmp(a,text,n);}
#ifdef MYOS_MEDIA_VP9
/* VP codec binding: profile0/8-bit, short or full default SDR metadata.
 * Full suffix is 4:2:0, BT709 primaries/transfer/matrix, limited range.
 * Other colour profiles/ranges require explicit conversion support. */
static int vp9_codec(const char *a,const char *b){
 if(match(a,b,"vp9")||match(a,b,"vp9.0"))return 1;
 if(b-a!=13&&b-a!=28)return 0;
 if(b-a==28&&memcmp(a+13,".01.01.01.01.00",15))return 0;
 if(memcmp(a,"vp09.00.",8)||memcmp(a+10,".08",3))return 0;
 int level=(a[8]-'0')*10+a[9]-'0';
 return a[8]>='0'&&a[8]<='9'&&a[9]>='0'&&a[9]<='9'&&
  (level==10||level==11||level==20||level==21||level==30||level==31||
   level==40||level==41||level==50||level==51||level==52||
   level==60||level==61||level==62);
}
#endif
static int number(const char *a,const char *b,unsigned maximum,int fraction){
 unsigned n=0;int digits=0;
 while(a<b&&*a>='0'&&*a<='9'){if(n>maximum)return 0;n=n*10+(unsigned)(*a++-'0');digits=1;}
 if(!digits||n>maximum)return 0;
 if(a<b&&*a=='.'&&fraction){++a;int decimal=0;while(a<b&&*a>='0'&&*a<='9'){if(n==maximum&&*a!='0')return 0;++a;++decimal;}if(!decimal)return 0;}
 return a==b&&n>0;
}
#ifdef MYOS_MEDIA_TRACE
static SbMediaSupportType can_play(const char *mime,const char *key){
#else
SbMediaSupportType SbMediaCanPlayMimeAndKeySystem(const char *mime,const char *key){
#endif
 if(!mime||!key||*key)return kSbMediaSupportTypeNotSupported;
 size_t length=0;while(mime[length])if(++length>512)return kSbMediaSupportTypeNotSupported;
 const char *end=mime+length,*cursor=mime,*first=cursor;
 while(cursor<end&&*cursor!=';')++cursor;
 const char *last=cursor;trim(&first,&last);
 int video=match(first,last,"video/webm"),audio=match(first,last,"audio/webm");
 if(!video&&!audio)return kSbMediaSupportTypeNotSupported;
 int have_codecs=0,have_vp8=0,have_opus=0;
#ifdef MYOS_MEDIA_VP9
 int have_vp9=0,within_vp8=1;
#endif
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
    #ifdef MYOS_MEDIA_VP9
    else if(vp9_codec(a,b)){if(have_vp9++)return kSbMediaSupportTypeNotSupported;}
#endif
    else if(match(a,b,"opus")){if(have_opus++)return kSbMediaSupportTypeNotSupported;}
    else return kSbMediaSupportTypeNotSupported;
    if(value<last){++value;if(value==last)return kSbMediaSupportTypeNotSupported;}
   }
  }
#ifdef MYOS_MEDIA_VP9
  else if(match(name,name_end,"width")){if(!number(value,last,320,0))return kSbMediaSupportTypeNotSupported;within_vp8&=number(value,last,160,0);}
  else if(match(name,name_end,"height")){if(!number(value,last,240,0))return kSbMediaSupportTypeNotSupported;within_vp8&=number(value,last,90,0);}
  else if(match(name,name_end,"framerate")){if(!number(value,last,30,1))return kSbMediaSupportTypeNotSupported;within_vp8&=number(value,last,15,1);}
#else
  else if(match(name,name_end,"width")){if(!number(value,last,160,0))return kSbMediaSupportTypeNotSupported;}
  else if(match(name,name_end,"height")){if(!number(value,last,90,0))return kSbMediaSupportTypeNotSupported;}
  else if(match(name,name_end,"framerate")){if(!number(value,last,15,1))return kSbMediaSupportTypeNotSupported;}
#endif
  else if(match(name,name_end,"channels")){if(!number(value,last,2,0))return kSbMediaSupportTypeNotSupported;}
 }
 #ifdef MYOS_MEDIA_VP9
 if((audio&&have_vp9)||(have_vp9&&have_vp8)||(have_vp8&&!within_vp8))return kSbMediaSupportTypeNotSupported;
#endif
 if(audio&&have_vp8)return kSbMediaSupportTypeNotSupported;
 return have_codecs?kSbMediaSupportTypeProbably:kSbMediaSupportTypeMaybe;
}
bool SbMediaCanChangeType(const char *from,const char *to){(void)from;(void)to;return false;}

#ifdef MYOS_MEDIA_TRACE
/* Diagnostic-only negotiation evidence; never changes supported capabilities.
 * MIME is bounded in the log and DRM/key contents are deliberately omitted. */
SbMediaSupportType SbMediaCanPlayMimeAndKeySystem(const char *mime,const char *key){
 SbMediaSupportType support=can_play(mime,key);
#ifdef MYOS_MEDIA_TRACE
 char message[640];
 snprintf(message,sizeof(message),"MEDIA capability mime=%.512s encrypted=%d support=%d\n",
          mime?mime:"(null)",key&&*key?1:0,(int)support);
 SbLogRaw(message);
#endif
 return support;
}

#endif
