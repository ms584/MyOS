#include "net.h"
#include "nic.h"

/* Deliberately one outstanding request. Each receive buffer is bounded and
 * copied out of the DMA ring before the device can reuse it. */
#define FRAME 1514u
#define IP_PAYLOAD 1480u
static uint8_t tx[FRAME],rx[FRAME],udp_peer[4],tcp_peer[4];
static size_t rx_size;
static uint16_t rx_type,rx_source,rx_destination;
static uint8_t ip[4],mask[4],gateway[4],dns[4],server[4],peer_mac[6];
static uint8_t peer_ip[4];
static bool configured,have_peer,waiting;
static bool udp_filter,tcp_active;
static uint16_t tcp_port;
static uint32_t sequence=0x4d594f53u,acknowledge;
static uint16_t source_port=49152,ip_ident=1;
static uint8_t tcp_flags;
static uint32_t tcp_ack,tcp_seq;
static uint8_t tcp_data[1460];
static size_t tcp_length;
static bool tcp_event;
static bool stream_mode,stream_closed,stream_error;
static uint8_t stream_bytes[8192];
static size_t stream_head,stream_tail,stream_used;

static uint16_t rd16(const uint8_t *p){return ((uint16_t)p[0]<<8)|p[1];}
static uint32_t rd32(const uint8_t *p){return ((uint32_t)rd16(p)<<16)|rd16(p+2);}
static void wr16(uint8_t *p,uint16_t v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
static void wr32(uint8_t *p,uint32_t v){wr16(p,(uint16_t)(v>>16));wr16(p+2,(uint16_t)v);}
static bool same(const uint8_t *a,const uint8_t *b,size_t n){for(size_t i=0;i<n;++i)if(a[i]!=b[i])return false;return true;}
static uint32_t sum(const uint8_t *data,size_t n,uint32_t initial){for(size_t i=0;i<n;i+=2)initial+=((uint16_t)data[i]<<8)|(i+1<n?data[i+1]:0);return initial;}
static uint16_t finish(uint32_t value){while(value>>16)value=(value&0xffff)+(value>>16);return (uint16_t)~value;}
static uint16_t checksum(const uint8_t *data,size_t n){return finish(sum(data,n,0));}
static uint16_t transport_sum(const uint8_t *,const uint8_t *,uint8_t,const uint8_t *,size_t);
static bool frame(const uint8_t *,uint16_t,size_t);
static bool tcp_packet(const uint8_t target[4],uint16_t,uint8_t,const uint8_t *,size_t);
static void show_ip(const uint8_t *a){for(unsigned i=0;i<4;++i){if(i)putchar('.');decimal(a[i]);}}
bool net_parse_ip(const char *s,uint8_t out[4]){
    if(!s)return false;
    for(unsigned i=0;i<4;++i){unsigned v=0,digits=0;while(*s>='0'&&*s<='9'&&digits<3){v=v*10+(unsigned)(*s++-'0');++digits;}
        if(!digits||v>255)return false;out[i]=(uint8_t)v;if(i<3){if(*s++!='.')return false;}}
    return *s==0;
}
static bool timeout(uint64_t end){return ticks>=end;}
static void idle(void){__asm__ volatile("sti; hlt":::"memory");}
static void receive(const uint8_t *f,size_t n){
    if(n<14)return;
    uint16_t type=rd16(f+12);
    if(type==0x0806 && n>=42){
        if(!configured||!same(f+38,ip,4)||rd16(f+14)!=1||rd16(f+16)!=0x0800||f[18]!=6||f[19]!=4)return;
        if(rd16(f+20)==1){
            uint8_t sender[6],sender_ip[4];memcpy(sender,f+22,6);memcpy(sender_ip,f+28,4);
            memset(tx+14,0,28);wr16(tx+14,1);wr16(tx+16,0x0800);tx[18]=6;tx[19]=4;wr16(tx+20,2);
            memcpy(tx+22,nic_mac(),6);memcpy(tx+28,ip,4);memcpy(tx+32,sender,6);memcpy(tx+38,sender_ip,4);
            (void)frame(sender,0x0806,28);return;
        }
        if(rd16(f+20)==2){memcpy(peer_mac,f+22,6);memcpy(peer_ip,f+28,4);have_peer=true;}
        return;
    }
    if(type!=0x0800||n<34)return;
    const uint8_t *h=f+14;unsigned ihl=(h[0]&15)*4;
    if((h[0]>>4)!=4||ihl<20||14+ihl>n||checksum(h,ihl))return;
    size_t total=rd16(h+2);if(total<ihl||total>n-14||(rd16(h+6)&0x3fff))return;
    static const uint8_t broadcast[4]={255,255,255,255};
    if(configured&&!same(h+16,ip,4)&&!same(h+16,broadcast,4))return;
    const uint8_t *payload=h+ihl;size_t plen=total-ihl;
    if(h[9]==17 && plen>=8){
        uint16_t length=rd16(payload+4);if(length<8||length>plen)return;
        uint16_t src=rd16(payload),dst=rd16(payload+2);
        if(rd16(payload+6)&&transport_sum(h+12,h+16,17,payload,length))return;
        if(udp_filter&&!same(h+12,udp_peer,4))return;
        if(waiting && dst==source_port && (size_t)(length-8)<=sizeof(rx)){
            memcpy(rx,payload+8,length-8);rx_size=length-8;rx_source=src;rx_destination=dst;rx_type=17;waiting=false;
        }
    }else if(h[9]==6 && plen>=20){
        unsigned thl=(payload[12]>>4)*4;if(thl<20||thl>plen)return;
        uint16_t dst=rd16(payload+2);
        if(!tcp_active||dst!=source_port||rd16(payload)!=tcp_port||!same(h+12,tcp_peer,4)||
           transport_sum(h+12,h+16,6,payload,plen))return;
        tcp_flags=payload[13];tcp_ack=rd32(payload+8);tcp_seq=rd32(payload+4);
        tcp_length=plen-thl;if(tcp_length>sizeof(tcp_data))tcp_length=sizeof(tcp_data);
        memcpy(tcp_data,payload+thl,tcp_length);tcp_event=true;
        if(stream_mode){
            if(tcp_flags&4)stream_error=true;
            if(tcp_length){
                if(tcp_seq==acknowledge&&tcp_length<=sizeof(stream_bytes)-stream_used){
                    for(size_t j=0;j<tcp_length;++j){
                        stream_bytes[stream_head]=tcp_data[j];
                        stream_head=(stream_head+1)%sizeof(stream_bytes);
                    }
                    stream_used+=tcp_length;acknowledge+=(uint32_t)tcp_length;
                }
                (void)tcp_packet(tcp_peer,tcp_port,0x10,0,0);
            }
            if((tcp_flags&1)&&tcp_seq+tcp_length==acknowledge){
                ++acknowledge;stream_closed=true;
                (void)tcp_packet(tcp_peer,tcp_port,0x10,0,0);
            }
        }
    }
}
void net_poll(void){(void)nic_poll(receive,1);}
static bool frame(const uint8_t *mac,uint16_t type,size_t payload){
    if(payload+14>FRAME)return false;
    memcpy(tx,mac,6);memcpy(tx+6,nic_mac(),6);wr16(tx+12,type);
    return nic_send(tx,payload+14);
}
static bool route(const uint8_t target[4]){
    uint8_t next[4];bool local=true;
    for(unsigned i=0;i<4;++i)if((ip[i]&mask[i])!=(target[i]&mask[i]))local=false;
    memcpy(next,local?target:gateway,4);
    if(have_peer&&same(peer_ip,next,4))return true;
    have_peer=false;
    for(unsigned attempt=0;attempt<3;++attempt){
        memset(tx+14,0,28);wr16(tx+14,1);wr16(tx+16,0x0800);tx[18]=6;tx[19]=4;wr16(tx+20,1);
        memcpy(tx+22,nic_mac(),6);memcpy(tx+28,ip,4);memcpy(tx+38,next,4);
        static const uint8_t broadcast[6]={255,255,255,255,255,255};
        if(!frame(broadcast,0x0806,28))return false;
        uint64_t end=ticks+100;while(!timeout(end)){net_poll();if(have_peer&&same(peer_ip,next,4))return true;idle();}
    }
    return false;
}
static bool ipv4(const uint8_t target[4],const uint8_t mac[6],uint8_t protocol,size_t bytes,const uint8_t source[4]){
    if(bytes>IP_PAYLOAD)return false;
    uint8_t *h=tx+14;memset(h,0,20);h[0]=0x45;wr16(h+2,(uint16_t)(20+bytes));wr16(h+4,ip_ident++);
    wr16(h+6,0x4000);h[8]=64;h[9]=protocol;memcpy(h+12,source,4);memcpy(h+16,target,4);
    wr16(h+10,checksum(h,20));return frame(mac,0x0800,20+bytes);
}
static uint16_t transport_sum(const uint8_t *src,const uint8_t *dst,uint8_t protocol,const uint8_t *data,size_t length){
    uint32_t s=sum(src,4,0);s=sum(dst,4,s);s+=protocol+length;s=sum(data,length,s);return finish(s);
}
static bool udp(const uint8_t source[4],const uint8_t target[4],const uint8_t mac[6],uint16_t src,uint16_t dst,const uint8_t *data,size_t length){
    if(length>IP_PAYLOAD-8)return false;
    uint8_t *u=tx+34;wr16(u,src);wr16(u+2,dst);wr16(u+4,(uint16_t)(8+length));wr16(u+6,0);
    memcpy(u+8,data,length);uint16_t c=transport_sum(source,target,17,u,8+length);wr16(u+6,c?c:0xffff);
    return ipv4(target,mac,17,8+length,source);
}
static bool wait_udp(uint16_t port,uint64_t deadline){
    while(!timeout(deadline)){net_poll();if(!waiting&&rx_type==17&&rx_source==port)return true;idle();}waiting=false;return false;
}
static bool dhcp_message(uint8_t kind,const uint8_t request[4]){
    uint8_t payload[300];memset(payload,0,sizeof(payload));payload[0]=1;payload[1]=1;payload[2]=6;
    wr32(payload+4,0x4d594f53);wr16(payload+10,0x8000);memcpy(payload+28,nic_mac(),6);
    payload[236]=99;payload[237]=130;payload[238]=83;payload[239]=99;
    unsigned n=240;payload[n++]=53;payload[n++]=1;payload[n++]=kind;
    if(kind==3){payload[n++]=50;payload[n++]=4;memcpy(payload+n,request,4);n+=4;
        payload[n++]=54;payload[n++]=4;memcpy(payload+n,server,4);n+=4;}
    payload[n++]=55;payload[n++]=3;payload[n++]=1;payload[n++]=3;payload[n++]=6;payload[n++]=255;
    static const uint8_t zero[4]={0},broadcast_ip[4]={255,255,255,255};
    static const uint8_t broadcast_mac[6]={255,255,255,255,255,255};
    source_port=68;rx_type=0;waiting=true;udp_filter=false;
    return udp(zero,broadcast_ip,broadcast_mac,68,67,payload,n);
}
static uint8_t dhcp_options(uint8_t address[4]){
    if(rx_size<240||rx[0]!=2||rd32(rx+4)!=0x4d594f53||
       !same(rx+28,nic_mac(),6)||rd32(rx+236)!=0x63825363)return 0;
    memcpy(address,rx+16,4);uint8_t type=0;
    for(size_t i=240;i<rx_size;){uint8_t option=rx[i++];if(option==255)break;if(option==0)continue;
        if(i>=rx_size||i+1+rx[i]>rx_size)return 0;uint8_t len=rx[i++];
        if(option==53&&len==1)type=rx[i];
        else if(option==1&&len==4)memcpy(mask,rx+i,4);
        else if(option==3&&len>=4)memcpy(gateway,rx+i,4);
        else if(option==6&&len>=4)memcpy(dns,rx+i,4);
        else if(option==54&&len==4)memcpy(server,rx+i,4);
        i+=len;
    }
    return type;
}
bool net_configured(void){return configured&&nic_ready();}
bool net_up(void){
    configured=false;have_peer=false;if(!nic_ready())return false;
    uint8_t offered[4],assigned[4];
    for(unsigned attempt=0;attempt<3;++attempt){
        if(!dhcp_message(1,0))return false;
        if(!wait_udp(67,ticks+150)||dhcp_options(offered)!=2)continue;
        if(!dhcp_message(3,offered))return false;
        if(!wait_udp(67,ticks+150)||dhcp_options(assigned)!=5||!same(offered,assigned,4))continue;
        memcpy(ip,assigned,4);configured=true;return true;
    }
    return false;
}
void net_status(void){
    if(!nic_ready()){print("Network: no RTL8139 NIC\n");return;}
    print("Network: RTL8139, ");if(!configured){print("no DHCP lease\n");return;}
    print("IP ");show_ip(ip);print(" gateway ");show_ip(gateway);print(" DNS ");show_ip(dns);print("\n");
}
bool net_resolve(const char *name,uint8_t out[4]){
    if(net_parse_ip(name,out))return true;
    if(!configured||!name)return false;
    size_t name_len=0;while(name[name_len])++name_len;if(!name_len||name_len>240)return false;
    uint8_t query[280];memset(query,0,sizeof(query));wr16(query,0x4d59);wr16(query+2,0x0100);wr16(query+4,1);
    size_t q=12,start=0;
    for(size_t i=0;i<=name_len;++i)if(name[i]=='.'||!name[i]){
        size_t size=i-start;if(!size||size>63||q+size+6>sizeof(query))return false;
        query[q++]=(uint8_t)size;memcpy(query+q,name+start,size);q+=size;start=i+1;
    }
    query[q++]=0;wr16(query+q,1);q+=2;wr16(query+q,1);q+=2;
    if(!route(dns))return false;
    source_port=49153;rx_type=0;waiting=true;udp_filter=true;memcpy(udp_peer,dns,4);
    for(unsigned attempt=0;attempt<3;++attempt){
        if(!udp(ip,dns,peer_mac,source_port,53,query,q))return false;
        if(!wait_udp(53,ticks+100)){waiting=true;continue;}
        if(rx_size<12||rd16(rx)!=0x4d59||(rd16(rx+2)&0x800f)!=0x8000||rd16(rx+4)!=1)return false;
        size_t p=12;while(p<rx_size&&rx[p]){if((rx[p]&0xc0)||p+1+rx[p]>rx_size)return false;p+=1+rx[p];}
        if(p+5>rx_size)return false;p+=5;
        for(unsigned answer=0;answer<rd16(rx+6)&&answer<16;++answer){
            if(p>=rx_size)return false;
            if((rx[p]&0xc0)==0xc0)p+=2;else{while(p<rx_size&&rx[p]){if(p+1+rx[p]>rx_size)return false;p+=1+rx[p];}++p;}
            if(p+10>rx_size)return false;
            uint16_t type=rd16(rx+p),klass=rd16(rx+p+2),size=rd16(rx+p+8);p+=10;
            if(p+size>rx_size)return false;
            if(type==1&&klass==1&&size==4){memcpy(out,rx+p,4);return true;}p+=size;
        }
        return false;
    }
    return false;
}
bool net_udp_echo(const uint8_t target[4],uint16_t port,const char *message){
    if(!configured||!route(target))return false;
    size_t n=0;while(message[n]&&n<512)++n;if(message[n])return false;
    source_port=49154;rx_type=0;waiting=true;udp_filter=true;memcpy(udp_peer,target,4);
    for(unsigned attempt=0;attempt<3;++attempt){
        if(!udp(ip,target,peer_mac,source_port,port,(const uint8_t *)message,n))return false;
        if(wait_udp(port,ticks+100)){print("UDP reply: ");for(size_t i=0;i<rx_size&&i<512;++i)putchar(rx[i]>=32&&rx[i]<=126?(char)rx[i]:'.');print("\n");return true;}
        waiting=true;
    }
    return false;
}
static bool tcp_packet(const uint8_t target[4],uint16_t port,uint8_t flags,const uint8_t *data,size_t n){
    if(n>IP_PAYLOAD-20)return false;
    uint8_t *t=tx+34;memset(t,0,20);wr16(t,source_port);wr16(t+2,port);wr32(t+4,sequence);wr32(t+8,acknowledge);
    t[12]=0x50;t[13]=flags;
    size_t window=stream_mode?sizeof(stream_bytes)-stream_used:4096;
    if(window>4096)window=4096;
    wr16(t+14,(uint16_t)window);memcpy(t+20,data,n);
    wr16(t+16,transport_sum(ip,target,6,t,20+n));return ipv4(target,peer_mac,6,20+n,ip);
}
static bool wait_tcp(uint64_t deadline){while(!timeout(deadline)){net_poll();if(tcp_event)return true;idle();}return false;}
bool net_tcp_open(const uint8_t target[4],uint16_t port){
    if(tcp_active||!configured||!port||!route(target))return false;
    tcp_active=true;stream_mode=false;stream_closed=stream_error=false;
    stream_head=stream_tail=stream_used=0;tcp_port=port;
    memcpy(tcp_peer,target,4);
    source_port++;if(source_port<49155)source_port=49155;
    sequence+=1009;acknowledge=0;tcp_event=false;
    for(unsigned attempt=0;attempt<3;++attempt){
        if(!tcp_packet(target,port,2,0,0))break;
        uint64_t end=ticks+150;
        while(wait_tcp(end)){
            tcp_event=false;
            if(tcp_flags&4)goto fail;
            if((tcp_flags&0x12)==0x12&&tcp_ack==sequence+1){
                acknowledge=tcp_seq+1;++sequence;
                stream_mode=true;
                if(tcp_packet(target,port,0x10,0,0))return true;
                goto fail;
            }
        }
    }
fail:
    stream_mode=tcp_active=false;return false;
}
int net_tcp_write(const uint8_t *data,size_t size){
    if(!stream_mode||!data||stream_closed||stream_error)return -1;
    size_t sent=0;
    while(sent<size){
        size_t n=size-sent;if(n>1460)n=1460;
        bool acknowledged=false;
        for(unsigned attempt=0;attempt<3&&!acknowledged;++attempt){
            tcp_event=false;
            if(!tcp_packet(tcp_peer,tcp_port,0x18,data+sent,n))return -1;
            uint64_t end=ticks+150;
            while(!timeout(end)){
                net_poll();
                if(stream_error)return -1;
                if(tcp_event){
                    tcp_event=false;
                    if(tcp_ack==sequence+n){acknowledged=true;break;}
                }
                idle();
            }
        }
        if(!acknowledged)return -1;
        sequence+=(uint32_t)n;sent+=n;
    }
    return (int)sent;
}
int net_tcp_read(uint8_t *data,size_t capacity){
    if(!stream_mode||!data||!capacity)return -1;
    uint64_t end=ticks+500;
    while(!stream_used&&!stream_closed&&!stream_error&&!timeout(end)){
        net_poll();if(!stream_used&&!stream_closed&&!stream_error)idle();
    }
    if(stream_error)return -1;
    if(!stream_used)return stream_closed?0:-1;
    size_t n=capacity;if(n>stream_used)n=stream_used;
    for(size_t i=0;i<n;++i){
        data[i]=stream_bytes[stream_tail];
        stream_tail=(stream_tail+1)%sizeof(stream_bytes);
    }
    stream_used-=n;
    (void)tcp_packet(tcp_peer,tcp_port,0x10,0,0);
    return (int)n;
}
void net_tcp_close(void){
    if(stream_mode&&!stream_error){
        (void)tcp_packet(tcp_peer,tcp_port,0x11,0,0);
        ++sequence;
    }
    stream_mode=tcp_active=false;tcp_event=false;
    stream_head=stream_tail=stream_used=0;
}
static bool http_fetch_inner(const uint8_t target[4],uint16_t port,const char *host,const char *path,
                             net_http_sink sink,void *context){
    if(!configured||!route(target)||!host||!path||!sink)return false;
    tcp_active=true;tcp_port=port;memcpy(tcp_peer,target,4);
    source_port++;if(source_port<49155)source_port=49155;
    sequence+=1009;acknowledge=0;tcp_event=false;
    bool connected=false;
    for(unsigned attempt=0;attempt<3&&!connected;++attempt){
        if(!tcp_packet(target,port,2,0,0))return false;
        uint64_t end=ticks+150;
        while(wait_tcp(end)){tcp_event=false;
            if(tcp_flags&4)return false;
            if((tcp_flags&0x12)==0x12&&tcp_ack==sequence+1){acknowledge=tcp_seq+1;++sequence;connected=true;break;}
        }
    }
    if(!connected||!tcp_packet(target,port,0x10,0,0))return false;
    uint8_t request[480];size_t n=0;
    const char *parts[]={"GET ",path," HTTP/1.0\r\nHost: ",host,"\r\nConnection: close\r\n\r\n"};
    for(unsigned i=0;i<5;++i)for(const char *p=parts[i];*p;++p){if(n==sizeof(request))return false;request[n++]=(uint8_t)*p;}
    bool sent=false;for(unsigned attempt=0;attempt<3&&!sent;++attempt){
        tcp_event=false;if(!tcp_packet(target,port,0x18,request,n))return false;
        uint64_t end=ticks+150;
        while(wait_tcp(end)){if(tcp_flags&4)return false;
            if(tcp_ack==sequence+n){sent=true;if(!tcp_length&&!(tcp_flags&1))tcp_event=false;break;}
            tcp_event=false;
        }
    }
    if(!sent)return false;sequence+=(uint32_t)n;
    unsigned packets=0;bool done=false;uint64_t end=ticks+500;
    while(!timeout(end)&&packets<128){
        if(!wait_tcp(end))break;tcp_event=false;if(tcp_flags&4)break;
        if(tcp_length&&tcp_seq==acknowledge){
            if(!sink(context,tcp_data,tcp_length))break;
            acknowledge+=(uint32_t)tcp_length;packets++;end=ticks+200;
            if(!tcp_packet(target,port,0x10,0,0))break;
        }else if(tcp_length)(void)tcp_packet(target,port,0x10,0,0);
        if(tcp_flags&1){acknowledge++;(void)tcp_packet(target,port,0x10,0,0);done=true;break;}
    }
    (void)tcp_packet(target,port,0x11,0,0);
    return done;
}
bool net_http_fetch(const uint8_t target[4],uint16_t port,const char *host,const char *path,
                    net_http_sink sink,void *context){
    bool result=http_fetch_inner(target,port,host,path,sink,context);
    tcp_active=false;tcp_event=false;
    return result;
}
static bool console_sink(void *context,const uint8_t *data,size_t length){
    (void)context;
    for(size_t i=0;i<length;++i)putchar((data[i]>=32&&data[i]<=126)||data[i]=='\n'||data[i]=='\r'?(char)data[i]:'.');
    return true;
}
bool net_http_get(const uint8_t target[4],uint16_t port,const char *host,const char *path){
    return net_http_fetch(target,port,host,path,console_sink,0);
}
