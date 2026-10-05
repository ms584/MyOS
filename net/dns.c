#include "dns.h"
#include "net.h"
#include "tcp.h"
#include "security.h"
enum {DNS_ROUTE=1,DNS_UDP,DNS_TCP,DNS_DONE};
struct job {
    struct process *owner;uint64_t tid,sequence,deadline,next;
    uint32_t stream,state;uint16_t port,query_size,tx_count,rx_count,rx_target;
    unsigned attempts;uint8_t server[4],mac[6];
    uint8_t query[DNS_QUERY_MAX+2],reply[DNS_REPLY_MAX+2];
    struct dns_job_status status;
};
static struct {struct job *job;uint32_t generation;} jobs[DNS_JOB_LIMIT];
static uint16_t read16(const uint8_t *p){return (uint16_t)((unsigned)p[0]<<8|p[1]);}
static bool equal_ip(const uint8_t *a,const uint8_t *b)
{return a[0]==b[0]&&a[1]==b[1]&&a[2]==b[2]&&a[3]==b[3];}
static struct job *lookup(struct process *owner,uint64_t tid,uint32_t handle)
{
    unsigned slot=handle&31u;if(!slot||slot>DNS_JOB_LIMIT)return 0;
    struct job *j=jobs[slot-1].job;
    return j&&j->owner==owner&&j->tid==tid&&jobs[slot-1].generation==(handle>>5)?j:0;
}
static void finish(struct job *j,int error)
{
    j->state=DNS_DONE;j->status.error=error;
    if(j->stream){(void)tcp_stream_close(j->owner,j->stream);j->stream=0;}
    if(!process_notify(j->owner,j->sequence))
        (void)scheduler_wake(j->owner,j->sequence,~0u);
}
static void drive(struct job *j)
{
    if(j->state==DNS_DONE)return;
    if(ticks>=j->deadline){finish(j,-3);return;}
    if(j->state==DNS_ROUTE){
        if(ticks<j->next)return;
        if(j->attempts++==3){finish(j,-3);return;}
        j->next=ticks+100;
        int ready=net_stream_route(j->server,j->mac);
        if(ready<0){finish(j,-3);return;}
        if(!ready)return;
        j->state=DNS_UDP;j->attempts=0;j->next=ticks;
    }
    if(j->state==DNS_UDP){
        if(ticks<j->next)return;
        if(j->attempts++==3){finish(j,-3);return;}
        j->next=ticks+100;
        if(!net_dns_packet(j->server,j->mac,j->port,j->query+2,j->query_size))finish(j,-3);
        return;
    }
    struct tcp_status status;
    if(tcp_stream_status(j->owner,j->stream,&status,2)||status.error){finish(j,-3);return;}
    if(!(status.ready&4))return;
    size_t full=(size_t)j->query_size+2;
    if(j->tx_count<full){
        int count=tcp_stream_send(j->owner,j->stream,j->query+j->tx_count,full-j->tx_count);
        if(count<0){if(count!=-11)finish(j,-3);return;}
        j->tx_count+=(uint16_t)count;if(j->tx_count<full)return;
    }
    if(!j->rx_target)j->rx_target=2;
    /* Read length and one reply only; every call and IRQ has bounded work. */
    for(unsigned step=0;step<2;++step){
        int count=tcp_stream_recv(j->owner,j->stream,j->reply+j->rx_count,
                                  j->rx_target-j->rx_count,false);
        if(count<=0){if(count!=-11)finish(j,-3);return;}
        j->rx_count+=(uint16_t)count;if(j->rx_count<j->rx_target)return;
        if(j->rx_target==2){unsigned n=read16(j->reply);
            if(n<12||n>DNS_REPLY_MAX){finish(j,-4);return;}
            j->rx_target=(uint16_t)(n+2);continue;}
        int rc=dns_parse_reply(j->reply+2,j->rx_target-2,j->query+2,
                               j->query_size,&j->status.answer);
        finish(j,rc);return;
    }
}
int64_t dns_job_start(struct process *owner,uint64_t tid,uint64_t sequence,const char *name)
{
    if(!owner||owner->state!=PROCESS_RUNNING||!tid)return -1;
    uint8_t server[4];if(!net_dns_server(server))return -101;
    unsigned slot=0;while(slot<DNS_JOB_LIMIT&&jobs[slot].job)++slot;
    if(slot==DNS_JOB_LIMIT)return -11;
    uint32_t nonce;if(!security_entropy_ready())return -38;
    if(!security_random(&nonce,sizeof(nonce)))return -5;
    struct job *j=kmalloc(sizeof(*j));if(!j)return -12;memset(j,0,sizeof(*j));
    int bytes=dns_make_query(name,(uint16_t)nonce,j->query+2,DNS_QUERY_MAX);
    if(bytes<0){kfree(j);return -22;}
    j->query_size=(uint16_t)bytes;j->query[0]=(uint8_t)((unsigned)bytes>>8);j->query[1]=(uint8_t)bytes;
    j->port=(uint16_t)(32768+(nonce>>16)%16384);unsigned attempts=0;
    for(;;){bool used=false;
        for(unsigned i=0;i<DNS_JOB_LIMIT;++i)if(jobs[i].job&&jobs[i].job->port==j->port)used=true;
        if(!used)break;if(++attempts==DNS_JOB_LIMIT){kfree(j);return -11;}
        j->port=(uint16_t)(32768+((j->port-32768+1)%16384));
    }
    j->owner=owner;j->tid=tid;j->sequence=sequence;j->deadline=ticks+1000;
    memcpy(j->server,server,4);j->state=DNS_ROUTE;
    jobs[slot].generation=(jobs[slot].generation+1)&0x7ffffffu;
    if(!jobs[slot].generation)jobs[slot].generation=1;
    jobs[slot].job=j;drive(j);
    return ((uint64_t)jobs[slot].generation<<5)|(slot+1);
}
int dns_job_status(struct process *owner,uint64_t tid,uint32_t handle,struct dns_job_status *status)
{
    struct job *j=lookup(owner,tid,handle);if(!j)return -9;
    if(j->state!=DNS_DONE)return -11;*status=j->status;return 0;
}
int dns_job_close(struct process *owner,uint64_t tid,uint32_t handle)
{
    struct job *j=lookup(owner,tid,handle);if(!j)return -9;
    if(j->stream)(void)tcp_stream_close(owner,j->stream);
    jobs[(handle&31u)-1].job=0;kfree(j);return 0;
}
void dns_job_release_owner(struct process *owner,uint64_t tid)
{
    for(unsigned i=0;i<DNS_JOB_LIMIT;++i){struct job *j=jobs[i].job;
        if(j&&j->owner==owner&&(!tid||j->tid==tid))
            (void)dns_job_close(owner,j->tid,(jobs[i].generation<<5)|(i+1));}
}
bool dns_job_busy(void)
{for(unsigned i=0;i<DNS_JOB_LIMIT;++i)if(jobs[i].job&&jobs[i].job->state!=DNS_DONE)return true;return false;}
void dns_job_tick(void)
{
    if(!dns_job_busy())return;
    for(unsigned i=0;i<8;++i)net_poll();
    for(unsigned i=0;i<DNS_JOB_LIMIT;++i)if(jobs[i].job)drive(jobs[i].job);
}
bool dns_job_receive(const uint8_t source[4],uint16_t src,uint16_t dst,const uint8_t *p,size_t n)
{
    if(src!=53)return false;
    for(unsigned i=0;i<DNS_JOB_LIMIT;++i){struct job *j=jobs[i].job;
        if(!j||j->state!=DNS_UDP||j->port!=dst||!equal_ip(source,j->server))continue;
        int rc=dns_parse_reply(p,n,j->query+2,j->query_size,&j->status.answer);
        if(rc==-4)return true; /* Ignore malformed/mismatched packets; timeout remains. */
        if(rc==-3&&n>=4&&(p[2]&2)){
            int64_t stream=tcp_stream_create(j->owner,j->sequence,j->sequence);
            if(stream<0){finish(j,-3);return true;}
            j->stream=(uint32_t)stream;
            struct tcp_address peer={{j->server[0],j->server[1],j->server[2],j->server[3]},53,0};
            int connected=tcp_stream_connect(j->owner,j->stream,&peer);
            if(connected&&connected!=-115){finish(j,-3);return true;}
            j->state=DNS_TCP;
        }else finish(j,rc);
        return true;
    }
    return false;
}
