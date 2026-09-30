/* Transport state-machine tests. The NIC/IP/checksum boundary is exercised
 * separately by test_elf_tcp.py inside MyOS, not simulated as browser evidence. */
#include "tcp.h"
#include "net.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
static unsigned allocations,packets;
volatile uint64_t ticks;
static uint8_t last[1480];static size_t last_size;
static const uint8_t target[4]={10,0,2,2};
void *kmalloc(size_t n){void *p=malloc(n);if(p)++allocations;return p;}
void kfree(void *p){if(p){assert(allocations);--allocations;free(p);}}
bool process_user_access(struct process *p,uint64_t address,size_t n,unsigned prot)
{(void)p;(void)prot;return address>=4096&&n==8;}
bool process_notify(struct process *p,uint64_t address)
{(void)p;++*(uint64_t *)(uintptr_t)address;return true;}
bool net_configured(void){return true;}
int net_stream_route(const uint8_t ip[4],uint8_t mac[6])
{(void)ip;memset(mac,1,6);return 1;}
void net_stream_address(uint8_t ip[4]){memcpy(ip,target,4);ip[3]=15;}
bool net_stream_packet(const uint8_t ip[4],const uint8_t mac[6],uint8_t *p,size_t n)
{(void)ip;(void)mac;assert(n<=sizeof(last));memcpy(last,p,n);last_size=n;++packets;return true;}
void net_poll(void){}
static uint16_t rd16(const uint8_t *p){return (uint16_t)((p[0]<<8)|p[1]);}
static uint32_t rd32(const uint8_t *p){return ((uint32_t)rd16(p)<<16)|rd16(p+2);}
static void wr16(uint8_t *p,uint16_t v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
static void wr32(uint8_t *p,uint32_t v){wr16(p,(uint16_t)(v>>16));wr16(p+2,(uint16_t)v);}
struct connection { uint32_t handle,local_sequence,remote_sequence;uint16_t port;uint64_t sequence,registry; };
static void inject(struct connection *c,uint32_t seq,uint32_t ack,unsigned flags,unsigned window,const void *data,size_t n)
{
    uint8_t packet[1480]={0};assert(n<=1460);wr16(packet,8080);wr16(packet+2,c->port);
    wr32(packet+4,seq);wr32(packet+8,ack);packet[12]=0x50;packet[13]=(uint8_t)flags;wr16(packet+14,(uint16_t)window);
    if(n)memcpy(packet+20,data,n);assert(tcp_stream_receive(target,packet,20+n));
}
static void connect_new(struct process *p,struct connection *c,unsigned window)
{
    int64_t handle=tcp_stream_create(p,(uintptr_t)&c->sequence,(uintptr_t)&c->registry);assert(handle>0);c->handle=(uint32_t)handle;
    struct tcp_address peer={{10,0,2,2},8080,0};assert(tcp_stream_connect(p,c->handle,&peer)==-115);
    assert(last[13]==2);c->port=rd16(last);c->local_sequence=rd32(last+4)+1;c->remote_sequence=0xffffff01u;
    inject(c,c->remote_sequence-1,c->local_sequence,18,window,0,0);
    struct tcp_status status;assert(!tcp_stream_status(p,c->handle,&status,false)&&status.state==4);
}
int main(void)
{
    struct process owner={.state=PROCESS_RUNNING},other={.state=PROCESS_RUNNING};
    struct connection a={0},b={0};connect_new(&owner,&a,8192);connect_new(&owner,&b,8192);assert(a.port!=b.port);
    struct tcp_status status;assert(tcp_stream_status(&other,a.handle,&status,false)==-9);
    uint8_t bytes[8192],out[8192];for(unsigned i=0;i<sizeof(bytes);++i)bytes[i]=(uint8_t)i;
    assert(tcp_stream_send(&owner,a.handle,bytes,600)==600);assert(last_size==556);uint32_t initial=rd32(last+4);
    /* Future ACKs cannot remove queued data. Partial ACKs retransmit only the
     * remaining sequence interval, before advancing to the next segment. */
    unsigned count=packets;inject(&a,a.remote_sequence,initial+9999,16,8192,0,0);assert(packets==count+1);
    inject(&a,a.remote_sequence,initial+100,16,8192,0,0);
    ticks+=150;tcp_stream_tick();assert(rd32(last+4)==initial+100&&last_size==456);
    assert(!memcmp(last+20,bytes+100,436));
    inject(&a,a.remote_sequence,initial+536,16,8192,0,0);assert(rd32(last+4)==initial+536&&last_size==84);
    inject(&a,a.remote_sequence,initial+600,16,8192,0,0);a.local_sequence+=600;
    /* Wraparound, out-of-order and overlap must not duplicate delivered data. */
    inject(&a,a.remote_sequence+10,a.local_sequence,16,8192,bytes,400);
    assert(!tcp_stream_status(&owner,a.handle,&status,false)&&!status.available);
    inject(&a,a.remote_sequence,a.local_sequence,16,8192,bytes,400);a.remote_sequence+=400;
    inject(&a,a.remote_sequence-400,a.local_sequence,16,8192,bytes,400);
    assert(!tcp_stream_status(&owner,a.handle,&status,false)&&status.available==400);
    assert(tcp_stream_recv(&owner,a.handle,out,400,true)==400&&!memcmp(out,bytes,400));
    assert(tcp_stream_recv(&owner,a.handle,out,400,false)==400&&!memcmp(out,bytes,400));
    assert(tcp_stream_recv(&owner,a.handle,out,1,false)==-11);
    /* Bounded window; accept only the prefix that fits, then advertise reopened
     * space after application consumption. FIN cannot precede missing data. */
    for(unsigned i=0;i<6;++i){inject(&a,a.remote_sequence,a.local_sequence,16,8192,bytes,1460);
        a.remote_sequence+=i==5?892:1460;}
    assert(!tcp_stream_status(&owner,a.handle,&status,false)&&status.available==8192);
    assert(rd16(last+14)==0);
    inject(&a,a.remote_sequence+1,a.local_sequence,17,8192,0,0);
    assert(!tcp_stream_status(&owner,a.handle,&status,false)&&!(status.ready&0x2000));
    assert(tcp_stream_recv(&owner,a.handle,out,8192,false)==8192&&rd16(last+14)==8192);
    inject(&a,a.remote_sequence,a.local_sequence,17,8192,0,0);++a.remote_sequence;
    assert(tcp_stream_recv(&owner,a.handle,out,1,false)==0);
    assert(!tcp_stream_status(&owner,a.handle,&status,false)&&(status.ready&0x2000));
    /* Invalid reset is challenged; the valid reset affects this tuple only. */
    inject(&b,b.remote_sequence+1,0,4,0,0,0);
    assert(!tcp_stream_status(&owner,b.handle,&status,false)&&!status.error);
    inject(&b,b.remote_sequence,0,4,0,0,0);
    assert(!tcp_stream_status(&owner,b.handle,&status,true)&&status.error==104);
    assert(!tcp_stream_status(&owner,b.handle,&status,false)&&!status.error);
    assert(!tcp_stream_status(&owner,b.handle,&status,2)&&status.error==104);
    assert(!tcp_stream_status(&owner,b.handle,&status,false)&&!status.error);
    assert(tcp_stream_status(&owner,b.handle,&status,3)==-22&&!status.error);
    assert(tcp_stream_send(&owner,b.handle,bytes,1)==-104);
    assert(!tcp_stream_close(&owner,b.handle));uint32_t old=b.handle;connect_new(&owner,&b,0);
    assert(b.handle!=old&&tcp_stream_status(&owner,old,&status,false)==-9);
    count=packets;assert(tcp_stream_send(&owner,b.handle,bytes,4096)==4096);assert(last_size==20&&packets>=count);
    assert(tcp_stream_send(&owner,b.handle,bytes,1)==-11);
    assert(!tcp_stream_status(&owner,b.handle,&status,false)&&!(status.ready&4));
    inject(&b,b.remote_sequence,b.local_sequence,16,1000,0,0);assert(last_size==556);
    /* Teardown releases all streams, including closed-but-flushing handles. */
    assert(!tcp_stream_close(&owner,a.handle));tcp_stream_release_owner(&owner);assert(!allocations&&!tcp_stream_busy());
    uint64_t sequence=0,registry=0;uint32_t handles[TCP_STREAM_LIMIT];
    for(unsigned i=0;i<TCP_STREAM_LIMIT;++i){int64_t h=tcp_stream_create(&owner,(uintptr_t)&sequence,(uintptr_t)&registry);assert(h>0);handles[i]=(uint32_t)h;}
    assert(tcp_stream_create(&owner,(uintptr_t)&sequence,(uintptr_t)&registry)==-24);
    assert(tcp_stream_create(&owner,1,(uintptr_t)&registry)==-14);
    for(unsigned i=0;i<TCP_STREAM_LIMIT;++i)assert(!tcp_stream_close(&owner,handles[i]));assert(!allocations);
    struct connection timeout={0};int64_t handle=tcp_stream_create(&owner,(uintptr_t)&timeout.sequence,(uintptr_t)&timeout.registry);
    assert(handle>0);struct tcp_address peer={{10,0,2,2},8080,0};assert(tcp_stream_connect(&owner,(uint32_t)handle,&peer)==-115);
    for(unsigned i=0;i<3;++i){ticks+=150;tcp_stream_tick();}
    assert(!tcp_stream_status(&owner,(uint32_t)handle,&status,false)&&status.error==110);
    tcp_stream_release_owner(&owner);assert(!allocations&&!tcp_stream_busy());return 0;
}
