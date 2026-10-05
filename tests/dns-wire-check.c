#include "dns-wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void w16(unsigned char *p,unsigned n){p[0]=(unsigned char)(n>>8);p[1]=(unsigned char)n;}
static unsigned char q[271],reply[512];
static size_t question(void)
{int n=dns_make_query("ExAmPlE.test.",0x1234,q,sizeof(q));assert(n>0);return (size_t)n;}
static size_t base(unsigned records)
{size_t n=question();memcpy(reply,q,n);w16(reply+2,0x8180);w16(reply+6,records);return n;}
static size_t a_record(size_t at,unsigned last)
{static const unsigned char rr[]={0xc0,12,0,1,0,1,0,0,0,60,0,4,192,0,2,1};
 memcpy(reply+at,rr,sizeof(rr));reply[at+15]=(unsigned char)last;return at+sizeof(rr);}
int main(void)
{
 struct dns_answer result,saved;memset(&saved,0x55,sizeof(saved));
 size_t begin=base(3),n=a_record(a_record(a_record(begin,1),2),1);
 result=saved;assert(!dns_parse_reply(reply,n,q,question(),&result));
 assert(result.count==2&&result.addresses[0][3]==1&&result.addresses[1][3]==2);
 assert(!strcmp(result.canonical,"example.test"));
 for(size_t i=0;i<n;++i){result=saved;
  assert(dns_parse_reply(reply,i,q,question(),&result)<0);
  assert(!memcmp(&result,&saved,sizeof(saved)));}
 unsigned char original[512];memcpy(original,reply,n);
 for(size_t i=0;i<n;++i)for(unsigned bit=0;bit<8;++bit){
  memcpy(reply,original,n);reply[i]^=(unsigned char)(1u<<bit);result=saved;
  int rc=dns_parse_reply(reply,n,q,question(),&result);
  if(rc)assert(!memcmp(&result,&saved,sizeof(saved)));
 }
 memcpy(reply,original,n);reply[0]^=1;assert(dns_parse_reply(reply,n,q,question(),&result)==-4);
 begin=base(1);n=a_record(begin,1);reply[begin]=0xc0;reply[begin+1]=(unsigned char)begin;
 assert(dns_parse_reply(reply,n,q,question(),&result)==-4);
 begin=base(1);n=a_record(begin,1);reply[begin+1]=0xff;
 assert(dns_parse_reply(reply,n,q,question(),&result)==-4);
 /* CNAME question -> alias.test, then compressed A owner to that RDATA. */
 begin=base(2);size_t at=begin;
 const unsigned char cname[]={0xc0,12,0,5,0,1,0,0,0,60,0,12,5,'a','l','i','a','s',4,'t','e','s','t',0};
 memcpy(reply+at,cname,sizeof(cname));size_t target=at+12;at+=sizeof(cname);
 n=a_record(at,7);w16(reply+at,0xc000u+(unsigned)target);
 result=saved;assert(!dns_parse_reply(reply,n,q,question(),&result));
 assert(result.count==1&&result.addresses[0][3]==7&&!strcmp(result.canonical,"alias.test"));
 /* A compressed name must not escape its declared CNAME RDATA length. */
 w16(reply+begin+10,11);assert(dns_parse_reply(reply,n,q,question(),&result)==-4);
 begin=base(0);w16(reply+2,0x8183);assert(dns_parse_reply(reply,begin,q,question(),&result)==-2);
 w16(reply+2,0x8182);assert(dns_parse_reply(reply,begin,q,question(),&result)==-3);
 w16(reply+2,0x8180);assert(dns_parse_reply(reply,begin,q,question(),&result)==-5);
 w16(reply+2,0x8380);assert(dns_parse_reply(reply,begin,q,question(),&result)==-3);
 begin=base(9);n=begin;for(unsigned i=0;i<9;++i)n=a_record(n,i);
 assert(dns_parse_reply(reply,n,q,question(),&result)==-12);
 char name[256];memset(name,'a',63);name[63]='.';memset(name+64,'b',63);
 name[127]='.';memset(name+128,'c',63);name[191]='.';memset(name+192,'d',61);name[253]=0;
 assert(dns_make_query(name,1,q,sizeof(q))==271);
 name[253]='d';name[254]=0;assert(dns_make_query(name,1,q,sizeof(q))<0);
 assert(dns_make_query(".",1,q,sizeof(q))==17);
 assert(dns_make_query("a..b",1,q,sizeof(q))<0);
 assert(dns_make_query("",1,q,sizeof(q))<0);
 assert(dns_make_query("example.test",1,q,16)<0);
 /* Bounded deterministic hostile frames under sanitizers, no claimed answer. */
 uint32_t rng=33;question();
 for(unsigned run=0;run<20000;++run){n=run%513;
  for(size_t i=0;i<n;++i){rng=rng*1664525u+1013904223u;reply[i]=(unsigned char)(rng>>24);}
  result=saved;int rc=dns_parse_reply(reply,n,q,question(),&result);
  if(rc)assert(!memcmp(&result,&saved,sizeof(saved)));
 }
 puts("PASS: DNS wire A/dedup/CNAME/compression/question/RCODE/TC, output atomicity, every truncation/bit mutation, bounds, 20000 hostile frames; host only.");
 return 0;
}
