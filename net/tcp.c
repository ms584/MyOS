#include "tcp.h"
#include "net.h"

enum { TCP_NEW=1,TCP_ROUTE,TCP_SYN,TCP_OPEN,TCP_FAILED };
enum { FIN=1,SYN=2,RST=4,ACK=16 };
enum { IN=1,OUT=4,ERR=8,HUP=16,RDHUP=0x2000 };
struct stream {
    struct process *owner;
    uint8_t *bytes;
    uint64_t object_sequence,registry_sequence,deadline,close_deadline;
    uint32_t generation,state,error,reported_error;
    uint32_t send_una,send_next,receive_next;
    uint32_t rx_head,rx_count,tx_head,tx_count,inflight;
    struct tcp_address peer;
    uint8_t mac[6];
    uint16_t port,window,mss;
    uint8_t retries,read_shutdown,write_shutdown,peer_fin,fin_sent,fin_ack,closing;
};
static struct stream streams[TCP_STREAM_LIMIT];
static uint16_t next_port=56000;
static uint32_t next_sequence=0x736f636bu;
static uint16_t read16(const uint8_t *p){return (uint16_t)((p[0]<<8)|p[1]);}
static uint32_t read32(const uint8_t *p){return ((uint32_t)read16(p)<<16)|read16(p+2);}
static void write16(uint8_t *p,uint16_t v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
static void write32(uint8_t *p,uint32_t v){write16(p,(uint16_t)(v>>16));write16(p+2,(uint16_t)v);}
static bool before(uint32_t a,uint32_t b){return (int32_t)(a-b)<0;}
static bool same_ip(const uint8_t *a,const uint8_t *b){return !((a[0]^b[0])|(a[1]^b[1])|(a[2]^b[2])|(a[3]^b[3]));}
static void notify(struct stream *s)
{
    if(s->closing||!s->owner)return;
    (void)process_notify(s->owner,s->object_sequence);
    if(s->registry_sequence!=s->object_sequence)
        (void)process_notify(s->owner,s->registry_sequence);
}
static struct stream *lookup(struct process *owner,uint32_t handle)
{
    unsigned slot=handle&63u;
    if(!slot||slot>TCP_STREAM_LIMIT)return 0;
    struct stream *s=&streams[slot-1];
    return s->owner==owner&&s->bytes&&!s->closing&&s->generation==(handle>>6)?s:0;
}
static bool packet(struct stream *s,uint8_t flags,uint32_t sequence,size_t count)
{
    uint8_t p[1480];size_t header=(flags&SYN)?24:20;
    memset(p,0,header);write16(p,s->port);write16(p+2,s->peer.port);
    write32(p+4,sequence);write32(p+8,s->receive_next);p[12]=(uint8_t)((header/4)<<4);p[13]=flags;
    write16(p+14,(uint16_t)(s->read_shutdown?TCP_RX_CAPACITY:TCP_RX_CAPACITY-s->rx_count));
    if(flags&SYN){p[20]=2;p[21]=4;write16(p+22,1460);}
    for(size_t i=0;i<count;++i)p[header+i]=s->bytes[TCP_RX_CAPACITY+(s->tx_head+i)%TCP_TX_CAPACITY];
    return net_stream_packet(s->peer.ip,s->mac,p,header+count);
}
static void fail(struct stream *s,unsigned error)
{
    s->state=TCP_FAILED;s->error=s->reported_error=error;notify(s);
}
static void destroy(struct stream *s)
{
    uint32_t generation=s->generation;kfree(s->bytes);memset(s,0,sizeof(*s));s->generation=generation;
}
static void drive(struct stream *s)
{
    if(s->closing&&ticks>=s->close_deadline){destroy(s);return;}
    if(s->state==TCP_FAILED){if(s->closing)destroy(s);return;}
    if(s->state==TCP_ROUTE){
        if(ticks<s->deadline)return;
        if(s->retries>=3){fail(s,110);return;}
        ++s->retries;s->deadline=ticks+100;
        int ready=net_stream_route(s->peer.ip,s->mac);
        if(ready<0){fail(s,101);return;}
        if(!ready)return;
        s->state=TCP_SYN;s->retries=0;s->deadline=ticks;
    }
    if(s->state==TCP_SYN){
        if(ticks<s->deadline)return;
        if(s->retries>=3){fail(s,110);return;}
        if(!packet(s,SYN,s->send_una,0)){fail(s,101);return;}
        ++s->retries;s->deadline=ticks+150;return;
    }
    if(s->state!=TCP_OPEN)return;
    if(s->inflight){
        if(ticks<s->deadline)return;
        if(s->retries>=5){fail(s,110);return;}
        if(!packet(s,ACK,s->send_una,s->inflight)){fail(s,101);return;}
        ++s->retries;s->deadline=ticks+150;return;
    }
    if(s->tx_count){
        if(!s->window){
            if(ticks>=s->deadline){
                /* A sequence just outside the receive window elicits a window
                 * update without treating unsent data as acknowledged. */
                (void)packet(s,ACK,s->send_next-1,0);s->deadline=ticks+100;
            }
            return;
        }
        size_t n=s->tx_count;if(n>s->mss)n=s->mss;if(n>s->window)n=s->window;
        if(!packet(s,ACK,s->send_next,n)){fail(s,101);return;}
        s->inflight=(uint32_t)n;s->send_next+=(uint32_t)n;s->deadline=ticks+150;s->retries=1;return;
    }
    if(s->write_shutdown&&!s->fin_sent){
        if(!packet(s,FIN|ACK,s->send_next,0)){fail(s,101);return;}
        ++s->send_next;s->fin_sent=1;s->deadline=ticks+150;s->retries=1;
    } else if(s->fin_sent&&!s->fin_ack&&ticks>=s->deadline){
        if(s->retries>=5){fail(s,110);return;}
        (void)packet(s,FIN|ACK,s->send_next-1,0);++s->retries;s->deadline=ticks+150;
    }
    /* Keep the tuple for a bounded TIME_WAIT-like interval on explicit close;
     * process teardown releases it immediately and never retains user pages. */
    if(s->closing&&s->peer_fin&&s->fin_ack&&s->close_deadline>ticks+200)s->close_deadline=ticks+200;
}
bool tcp_stream_busy(void)
{for(unsigned i=0;i<TCP_STREAM_LIMIT;++i)if(streams[i].bytes)return true;return false;}
int64_t tcp_stream_create(struct process *owner,uint64_t object_sequence,uint64_t registry_sequence)
{
    if(!owner||(object_sequence&7)||(registry_sequence&7)||
       !process_user_access(owner,object_sequence,8,PROCESS_PROT_WRITE)||
       !process_user_access(owner,registry_sequence,8,PROCESS_PROT_WRITE))return -14;
    for(unsigned i=0;i<TCP_STREAM_LIMIT;++i){struct stream *s=&streams[i];if(s->bytes)continue;
        uint8_t *bytes=kmalloc(TCP_RX_CAPACITY+TCP_TX_CAPACITY);if(!bytes)return -12;
        uint32_t generation=(s->generation+1)&0x1ffffffu;if(!generation)generation=1;
        memset(s,0,sizeof(*s));s->generation=generation;s->owner=owner;s->bytes=bytes;s->state=TCP_NEW;
        s->object_sequence=object_sequence;s->registry_sequence=registry_sequence;
        /* The reserved range does not overlap the legacy HTTP/TLS client. */
        for(unsigned attempt=0;attempt<5000;++attempt){
            uint16_t port=next_port++;if(next_port>60999)next_port=56000;
            bool used=false;for(unsigned j=0;j<TCP_STREAM_LIMIT;++j)if(streams[j].bytes&&streams[j].port==port)used=true;
            if(!used){s->port=port;break;}
        }
        if(!s->port){destroy(s);return -99;}
        next_sequence+=0x10001u;s->send_una=s->send_next=next_sequence+(uint32_t)ticks;s->mss=536;
        return (int64_t)((generation<<6)|(i+1));
    }return -24;
}
int tcp_stream_connect(struct process *owner,uint32_t handle,const struct tcp_address *peer)
{
    struct stream *s=lookup(owner,handle);if(!s)return -9;
    if(s->state==TCP_OPEN)return -106;
    if(s->state==TCP_ROUTE||s->state==TCP_SYN)return -114;
    if(s->state==TCP_FAILED)return -(int)s->error;
    if(peer->reserved||!peer->port||!peer->ip[0]||peer->ip[0]>=224)return -22;
    if(!net_configured())return -101;
    s->peer=*peer;s->state=TCP_ROUTE;s->deadline=ticks;drive(s);
    return s->state==TCP_FAILED?-(int)s->error:-115;
}
int tcp_stream_status(struct process *owner,uint32_t handle,struct tcp_status *status,bool clear_error)
{
    struct stream *s=lookup(owner,handle);if(!s)return -9;
    memset(status,0,sizeof(*status));status->state=s->state;status->available=s->rx_count;
    status->error=s->reported_error;status->peer=s->peer;
    if(s->state!=TCP_NEW){status->local.port=s->port;net_stream_address(status->local.ip);}
    if(s->rx_count||s->read_shutdown||s->peer_fin)status->ready|=IN;
    if(s->peer_fin)status->ready|=RDHUP;
    if(s->state==TCP_OPEN&&(s->write_shutdown||s->tx_count<TCP_TX_CAPACITY))status->ready|=OUT;
    if(s->state==TCP_FAILED)status->ready|=IN|OUT|ERR|HUP;
    if(s->state==TCP_NEW)status->ready|=OUT|HUP;
    if(s->peer_fin&&s->fin_ack)status->ready|=HUP;
    if(clear_error)s->reported_error=0;
    return 0;
}
int tcp_stream_send(struct process *owner,uint32_t handle,const uint8_t *data,size_t length)
{
    struct stream *s=lookup(owner,handle);if(!s)return -9;
    if(s->error)return -(int)s->error;if(s->state!=TCP_OPEN)return -107;
    if(s->write_shutdown)return -32;if(!length)return 0;
    size_t n=TCP_TX_CAPACITY-s->tx_count;if(!n)return -11;if(n>length)n=length;
    unsigned tail=(s->tx_head+s->tx_count)%TCP_TX_CAPACITY;
    for(size_t i=0;i<n;++i){s->bytes[TCP_RX_CAPACITY+tail]=data[i];tail=(tail+1)%TCP_TX_CAPACITY;}
    s->tx_count+=(uint32_t)n;drive(s);notify(s);return (int)n;
}
int tcp_stream_recv(struct process *owner,uint32_t handle,uint8_t *data,size_t length,bool peek)
{
    struct stream *s=lookup(owner,handle);if(!s)return -9;
    if(s->read_shutdown)return 0;
    if(s->state!=TCP_OPEN&&s->state!=TCP_FAILED)return -107;
    if(!length)return 0;
    if(!s->rx_count)return s->error?-(int)s->error:s->peer_fin?0:-11;
    size_t n=length;if(n>s->rx_count)n=s->rx_count;unsigned head=s->rx_head;
    for(size_t i=0;i<n;++i){data[i]=s->bytes[head];head=(head+1)%TCP_RX_CAPACITY;}
    if(!peek){s->rx_head=head;s->rx_count-=(uint32_t)n;
        if(s->state==TCP_OPEN)(void)packet(s,ACK,s->send_next,0);notify(s);}
    return (int)n;
}
int tcp_stream_shutdown(struct process *owner,uint32_t handle,unsigned how)
{
    struct stream *s=lookup(owner,handle);if(!s)return -9;if(how>2)return -22;
    if(s->state!=TCP_OPEN)return -107;
    if(how==0||how==2){s->read_shutdown=1;s->rx_count=0;(void)packet(s,ACK,s->send_next,0);}
    if(how==1||how==2)s->write_shutdown=1;drive(s);notify(s);return 0;
}
int tcp_stream_close(struct process *owner,uint32_t handle)
{
    struct stream *s=lookup(owner,handle);if(!s)return -9;notify(s);
    if(s->state!=TCP_OPEN){destroy(s);return 0;}
    s->closing=1;s->read_shutdown=s->write_shutdown=1;s->rx_count=0;
    s->object_sequence=s->registry_sequence=0;s->close_deadline=ticks+1000;drive(s);return 0;
}
void tcp_stream_release_owner(struct process *owner)
{
    for(unsigned i=0;i<TCP_STREAM_LIMIT;++i){struct stream *s=&streams[i];if(!s->bytes||s->owner!=owner)continue;
        if(s->state==TCP_OPEN)(void)packet(s,RST|ACK,s->send_next,0);destroy(s);
    }
}
bool tcp_stream_receive(const uint8_t source[4],const uint8_t *p,size_t length)
{
    if(length<20)return false;unsigned header=(p[12]>>4)*4;if(header<20||header>length)return false;
    struct stream *s=0;uint16_t destination=read16(p+2);
    for(unsigned i=0;i<TCP_STREAM_LIMIT;++i)if(streams[i].bytes&&streams[i].port==destination&&
       streams[i].peer.port==read16(p)&&same_ip(streams[i].peer.ip,source)){s=&streams[i];break;}
    if(!s)return false;
    uint8_t flags=p[13];uint32_t seq=read32(p+4),ack=read32(p+8);uint16_t window=read16(p+14);
    if(s->state==TCP_SYN){
        if((flags&(RST|ACK))==(RST|ACK)&&ack==s->send_una+1){fail(s,111);return true;}
        if((flags&(SYN|ACK|RST))!=(SYN|ACK)||ack!=s->send_una+1)return true;
        for(unsigned i=20;i<header;){unsigned kind=p[i++];if(!kind)break;if(kind==1)continue;
            if(i>=header)break;unsigned n=p[i++];if(n<2||n>header-(i-2))break;
            if(kind==2&&n==4){unsigned mss=read16(p+i);if(mss&&mss<1460)s->mss=(uint16_t)mss;else if(mss)s->mss=1460;}
            i+=n-2;
        }
        s->send_una=s->send_next=ack;s->receive_next=seq+1;s->window=window;s->state=TCP_OPEN;
        (void)packet(s,ACK,s->send_next,0);notify(s);return true;
    }
    if(s->state!=TCP_OPEN)return true;
    if(flags&RST){if(seq==s->receive_next)fail(s,104);else (void)packet(s,ACK,s->send_next,0);return true;}
    if((flags&SYN)||!(flags&ACK)||before(ack,s->send_una)||before(s->send_next,ack)){
        (void)packet(s,ACK,s->send_next,0);return true;
    }
    /* Do not accept an out-of-window ACK/window update as progress. */
    size_t n=length-header;uint32_t end=seq+(uint32_t)n;
    if(before(s->receive_next,seq)||before(end,s->receive_next)){
        (void)packet(s,ACK,s->send_next,0);return true;
    }
    bool changed=s->window!=window;s->window=window;
    uint32_t acknowledged=ack-s->send_una;
    if(acknowledged){
        uint32_t data=acknowledged;if(data>s->inflight)data=s->inflight;
        s->tx_head=(s->tx_head+data)%TCP_TX_CAPACITY;s->tx_count-=data;s->inflight-=data;
        if(s->fin_sent&&ack==s->send_next)s->fin_ack=1;
        s->send_una=ack;s->retries=0;s->deadline=ticks+150;changed=true;
    }
    const uint8_t *data=p+header;
    if(before(seq,s->receive_next)){size_t skip=s->receive_next-seq;if(skip>n)skip=n;data+=skip;n-=skip;}
    if(n&&!s->peer_fin){
        size_t accepted=n;if(!s->read_shutdown&&accepted>TCP_RX_CAPACITY-s->rx_count)accepted=TCP_RX_CAPACITY-s->rx_count;
        if(!s->read_shutdown){unsigned tail=(s->rx_head+s->rx_count)%TCP_RX_CAPACITY;
            for(size_t i=0;i<accepted;++i){s->bytes[tail]=data[i];tail=(tail+1)%TCP_RX_CAPACITY;}
            s->rx_count+=(uint32_t)accepted;}
        s->receive_next+=(uint32_t)accepted;changed|=accepted!=0;
    }
    if((flags&FIN)&&!s->peer_fin&&end==s->receive_next){++s->receive_next;s->peer_fin=1;changed=true;}
    if(length>header||(flags&FIN))(void)packet(s,ACK,s->send_next,0);
    if(changed)notify(s);drive(s);return true;
}
void tcp_stream_tick(void)
{
    if(!tcp_stream_busy())return;
    /* NIC RX is polled even while all Ring 3 threads sleep. Bounded IRQ work. */
    for(unsigned i=0;i<8;++i)net_poll();
    for(unsigned i=0;i<TCP_STREAM_LIMIT;++i)if(streams[i].bytes)drive(&streams[i]);
}
