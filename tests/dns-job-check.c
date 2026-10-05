#include "dns.h"
#include "tcp.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
volatile uint64_t ticks;
static size_t allocated;
static unsigned notifications,tcp_closed;
static bool configured=true,entropy=true,oom;
static bool notify_ok=true;static unsigned invalid_wakes;
static unsigned char query[DNS_QUERY_MAX],tcp_reply[DNS_REPLY_MAX+2],tcp_sent[DNS_QUERY_MAX+2];
static size_t query_size,tcp_size,tcp_read,tcp_written;
static uint16_t port;
static const uint8_t server[4]={192,0,2,53};
void *kmalloc(size_t n){if(oom)return 0;void *p=malloc(n);if(p)++allocated;return p;}
void kfree(void *p){if(p){assert(allocated);--allocated;free(p);}}
bool security_entropy_ready(void){return entropy;}
bool security_random(void *p,size_t n){assert(n==4);uint32_t nonce=0x12345678;memcpy(p,&nonce,4);return true;}
bool process_notify(struct process *p,uint64_t sequence)
{assert(p->state==PROCESS_RUNNING);if(!notify_ok)return false;
 ++notifications;++*(uint64_t *)(uintptr_t)sequence;return true;}
unsigned scheduler_wake(struct process *p,uint64_t sequence,unsigned count)
{assert(p&&sequence&&count==~0u);++invalid_wakes;return 1;}
bool net_dns_server(uint8_t out[4]){if(!configured)return false;memcpy(out,server,4);return true;}
int net_stream_route(const uint8_t target[4],uint8_t mac[6])
{assert(!memcmp(target,server,4));memset(mac,3,6);return 1;}
bool net_dns_packet(const uint8_t target[4],const uint8_t mac[6],uint16_t source,const uint8_t *p,size_t n)
{assert(!memcmp(target,server,4)&&mac[0]==3&&n<=sizeof(query));
 memcpy(query,p,n);query_size=n;port=source;return true;}
void net_poll(void){}
int64_t tcp_stream_create(struct process *p,uint64_t a,uint64_t b)
{assert(p&&a==b);tcp_read=tcp_written=0;return 1234;}
int tcp_stream_connect(struct process *p,uint32_t handle,const struct tcp_address *address)
{assert(p&&handle==1234&&!memcmp(address->ip,server,4)&&address->port==53);return -115;}
int tcp_stream_status(struct process *p,uint32_t handle,struct tcp_status *status,unsigned mode)
{assert(p&&handle==1234&&mode==2);memset(status,0,sizeof(*status));status->ready=4;return 0;}
int tcp_stream_send(struct process *p,uint32_t handle,const uint8_t *bytes,size_t n)
{assert(p&&handle==1234);if(n>7)n=7;assert(tcp_written+n<=sizeof(tcp_sent));
 memcpy(tcp_sent+tcp_written,bytes,n);tcp_written+=n;return (int)n;}
int tcp_stream_recv(struct process *p,uint32_t handle,uint8_t *bytes,size_t n,bool peek)
{assert(p&&handle==1234&&!peek);if(tcp_read==tcp_size)return -11;
 if(n>3)n=3;if(n>tcp_size-tcp_read)n=tcp_size-tcp_read;
 memcpy(bytes,tcp_reply+tcp_read,n);tcp_read+=n;return (int)n;}
int tcp_stream_close(struct process *p,uint32_t handle)
{assert(p&&handle==1234);++tcp_closed;return 0;}
static size_t reply(unsigned char out[512],bool truncated)
{
 memcpy(out,query,query_size);out[2]=truncated?0x83:0x81;out[3]=0x80;
 if(truncated)return query_size;
 out[6]=0;out[7]=1;
 const unsigned char record[]={0xc0,12,0,1,0,1,0,0,0,60,0,4,192,0,2,17};
 memcpy(out+query_size,record,sizeof(record));return query_size+sizeof(record);
}
int main(void)
{
 struct process owner={.state=PROCESS_RUNNING},foreign={.state=PROCESS_RUNNING};
 uint64_t seq=0;struct dns_job_status status,saved;memset(&saved,0x55,sizeof(saved));
 unsigned char response[512];ticks=1;
 int64_t h=dns_job_start(&owner,1,(uintptr_t)&seq,"example.test");assert(h>0&&allocated==1);
 status=saved;assert(dns_job_status(&foreign,1,(uint32_t)h,&status)==-9&&!memcmp(&status,&saved,sizeof(status)));
 assert(dns_job_status(&owner,2,(uint32_t)h,&status)==-9);
 assert(dns_job_close(&foreign,1,(uint32_t)h)==-9);
 assert(dns_job_close(&owner,2,(uint32_t)h)==-9);
 assert(dns_job_status(&owner,1,(uint32_t)h,&status)==-11);
 size_t n=reply(response,false);uint8_t wrong[4]={192,0,2,54};
 assert(!dns_job_receive(wrong,53,port,response,n));
 assert(!dns_job_receive(server,54,port,response,n));
 response[0]^=1;assert(dns_job_receive(server,53,port,response,n));
 assert(dns_job_status(&owner,1,(uint32_t)h,&status)==-11&&!notifications);
 response[0]^=1;assert(dns_job_receive(server,53,port,response,n));
 assert(seq==1&&notifications==1&&!dns_job_status(&owner,1,(uint32_t)h,&status));
 assert(!status.error&&status.answer.count==1&&status.answer.addresses[0][3]==17);
 assert(!dns_job_close(&owner,1,(uint32_t)h)&&!allocated&&!dns_job_busy());
 assert(dns_job_status(&owner,1,(uint32_t)h,&status)==-9);
 int64_t newer=dns_job_start(&owner,1,(uintptr_t)&seq,"example.test");assert(newer>0&&newer!=h);
 assert(dns_job_close(&owner,1,(uint32_t)h)==-9);
 n=reply(response,true);assert(dns_job_receive(server,53,port,response,n));
 size_t full=reply(tcp_reply+2,false);tcp_reply[0]=(unsigned char)(full>>8);tcp_reply[1]=(unsigned char)full;tcp_size=full+2;
 for(unsigned i=0;i<500&&dns_job_status(&owner,1,(uint32_t)newer,&status)==-11;++i){++ticks;dns_job_tick();}
 assert(!dns_job_status(&owner,1,(uint32_t)newer,&status)&&!status.error&&status.answer.count==1);
 assert(tcp_written==query_size+2&&tcp_sent[0]==0&&tcp_sent[1]==query_size&&!memcmp(tcp_sent+2,query,query_size));
 assert(tcp_closed==1&&!dns_job_close(&owner,1,(uint32_t)newer)&&!allocated);
 /* Pool saturation, independent completion, thread-only cleanup, owner cleanup. */
 uint32_t handles[DNS_JOB_LIMIT];uint16_t ports[DNS_JOB_LIMIT];
 for(unsigned i=0;i<DNS_JOB_LIMIT;++i){h=dns_job_start(&owner,i+1,(uintptr_t)&seq,"example.test");
  assert(h>0);handles[i]=(uint32_t)h;ports[i]=port;
  for(unsigned j=0;j<i;++j)assert(ports[i]!=ports[j]);}
 assert(allocated==DNS_JOB_LIMIT&&dns_job_start(&owner,99,(uintptr_t)&seq,"example.test")==-11);
 n=reply(response,false);
 for(unsigned i=DNS_JOB_LIMIT;i;--i){assert(dns_job_receive(server,53,ports[i-1],response,n));
  assert(!dns_job_status(&owner,i,handles[i-1],&status)&&!status.error);}
 dns_job_release_owner(&owner,7);assert(allocated==DNS_JOB_LIMIT-1);
 assert(dns_job_status(&owner,7,handles[6],&status)==-9);
 dns_job_release_owner(&foreign,0);assert(allocated==DNS_JOB_LIMIT-1);
 dns_job_release_owner(&owner,0);assert(!allocated&&!dns_job_busy());
 h=dns_job_start(&owner,1,(uintptr_t)&seq,"timeout.test");assert(h>0);
 ticks+=1000;dns_job_tick();assert(!dns_job_status(&owner,1,(uint32_t)h,&status)&&status.error==-3);
 dns_job_release_owner(&owner,0);assert(!allocated);
 h=dns_job_start(&owner,1,(uintptr_t)&seq,"timeout.test");assert(h>0);
 notify_ok=false;uint64_t previous=seq;ticks+=1000;dns_job_tick();notify_ok=true;
 assert(seq==previous&&invalid_wakes==1&&!dns_job_status(&owner,1,(uint32_t)h,&status)&&status.error==-3);
 dns_job_release_owner(&owner,0);assert(!allocated);
 configured=false;assert(dns_job_start(&owner,1,(uintptr_t)&seq,"example.test")==-101);configured=true;
 entropy=false;assert(dns_job_start(&owner,1,(uintptr_t)&seq,"example.test")==-38);entropy=true;
 oom=true;assert(dns_job_start(&owner,1,(uintptr_t)&seq,"example.test")==-12);oom=false;
 assert(dns_job_start(&owner,1,(uintptr_t)&seq,"a..b")==-22&&!allocated);
 const char done[]="PASS: host DNS owner/thread/generation/pool/out-of-order/timeout/cleanup plus TCP partial framing/fallback; not native network proof.\n";
 assert(write(1,done,sizeof(done)-1)==sizeof(done)-1);return 0;
}
