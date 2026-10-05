/* PRIVATE interface batch. Layout confirmed against musl ifaddrs.h at
 * Cobalt3befd332486952f6603ae3baf2a8713878d6e200; no invented loopback device.
 */
#include "system-call.h"
struct ifaddrs_abi {
    struct ifaddrs_abi *next;char *name;uint32_t flags,pad;
    void *address,*netmask,*broadcast,*data;
};
_Static_assert(sizeof(struct ifaddrs_abi)==56,"musl ifaddrs LP64 layout");
_Static_assert(offsetof(struct ifaddrs_abi,address)==24,"musl ifa_addr");
struct packet_address_abi {
    uint16_t family,protocol;int32_t index;uint16_t hardware;
    uint8_t type,length,address[8];
};
_Static_assert(sizeof(struct packet_address_abi)==20,"sockaddr_ll layout");
struct interface_record {
    struct ifaddrs_abi item;char name[16];
    union {struct packet_address_abi packet;struct sockaddr_in_abi inet;} address;
    struct sockaddr_in_abi netmask,broadcast;
};
static int interface_snapshot(struct system_interface_snapshot *snapshot)
{int64_t rc=call3(MYOS_SYSTEM_CALL,SYSTEM_INTERFACE,(uintptr_t)snapshot,0);
 return rc<0?fail(rc):0;}
static struct interface_record *interface_record(const struct system_interface_snapshot *snapshot)
{
    struct interface_record *record=calloc(1,sizeof(*record));
    if(!record)return 0;
    const char name[]="eth0";
    for(unsigned i=0;i<sizeof(name);++i)record->name[i]=name[i];
    record->item.name=record->name;record->item.flags=snapshot->flags;
    record->item.address=&record->address;return record;
}
void freeifaddrs(struct ifaddrs_abi *head)
{while(head){struct ifaddrs_abi *next=head->next;free(head);head=next;}}
int getifaddrs(struct ifaddrs_abi **result)
{
    if(!result)return set_error(EFAULT);
    struct system_interface_snapshot snapshot;
    if(interface_snapshot(&snapshot))return -1;
    if(!snapshot.index){*result=0;return 0;}
    struct interface_record *packet=interface_record(&snapshot);
    if(!packet)return set_error(ENOMEM);
    packet->address.packet.family=17;packet->address.packet.index=(int32_t)snapshot.index;
    packet->address.packet.hardware=1;packet->address.packet.length=6;
    for(unsigned i=0;i<6;++i)packet->address.packet.address[i]=snapshot.mac[i];
    if(snapshot.configured){
        struct interface_record *inet=interface_record(&snapshot);
        if(!inet){free(packet);return set_error(ENOMEM);}
        inet->address.inet.family=2;inet->netmask.family=2;inet->broadcast.family=2;
        for(unsigned i=0;i<4;++i){
            inet->address.inet.ip[i]=snapshot.address[i];
            inet->netmask.ip[i]=snapshot.netmask[i];
            inet->broadcast.ip[i]=snapshot.broadcast[i];
        }
        inet->item.netmask=&inet->netmask;inet->item.broadcast=&inet->broadcast;
        packet->item.next=&inet->item;
    }
    *result=&packet->item;return 0;
}
char *if_indextoname(unsigned index,char *name)
{
    if(!name){set_error(EFAULT);return 0;}
    struct system_interface_snapshot snapshot;
    if(interface_snapshot(&snapshot))return 0;
    if(!index||index!=snapshot.index){set_error(6);return 0;} /* ENXIO */
    const char value[]="eth0";
    for(unsigned i=0;i<sizeof(value);++i)name[i]=value[i];return name;
}
