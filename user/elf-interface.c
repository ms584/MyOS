#include <stdint.h>
#include <stddef.h>
#include "system-call.h"
struct address {uint16_t family;uint8_t port[2],ip[4],zero[8];};
struct ifaddrs {struct ifaddrs *next;char *name;unsigned flags,pad;void *address,*netmask,*broadcast,*data;};
struct packet {uint16_t family,protocol;int32_t index;uint16_t hardware;uint8_t type,length,mac[8];};
extern int getifaddrs(struct ifaddrs **),*__errno_location(void);
extern void freeifaddrs(struct ifaddrs *),SbLogRaw(const char *);
extern char *if_indextoname(unsigned,char *);
static int64_t raw(uint64_t operation,uint64_t pointer,uint64_t reserved)
{register uint64_t a __asm__("rax")=MYOS_SYSTEM_CALL;
 __asm__ volatile("int $0x80":"+a"(a):"D"(operation),"S"(pointer),"d"(reserved):"memory","cc");return (int64_t)a;}
static int same_name(const char *name)
{return name&&name[0]=='e'&&name[1]=='t'&&name[2]=='h'&&name[3]=='0'&&!name[4];}
uint64_t _start(void)
{
 struct system_interface_snapshot snapshot;
 if(raw(SYSTEM_INTERFACE,(uintptr_t)&snapshot,0))return 1;
 if(raw(SYSTEM_INTERFACE,(uintptr_t)&snapshot,1)!=-22||raw(SYSTEM_INTERFACE,0,0)!=-14)return 2;
 if(raw(SYSTEM_INTERFACE,(uintptr_t)_start,0)!=-14)return 3;
 if(snapshot.index!=EXPECT_NIC||snapshot.configured!=EXPECT_CONFIGURED)return 4;
 if(snapshot.index&&((snapshot.flags&64)!=EXPECT_CARRIER*64u))return 5;
 for(unsigned i=0;i<sizeof(snapshot.reserved);++i)if(snapshot.reserved[i])return 6;
 char name[16];for(unsigned i=0;i<sizeof(name);++i)name[i]='X';
 if(if_indextoname(0,name)||*__errno_location()!=6||name[0]!='X')return 7;
 if(if_indextoname(0xffffffffu,name)||*__errno_location()!=6||name[0]!='X')return 8;
 if(if_indextoname(1,0)||*__errno_location()!=14)return 9;
 if(snapshot.index){if(if_indextoname(1,name)!=name||!same_name(name)||name[5]!='X')return 10;}
 else if(if_indextoname(1,name)||*__errno_location()!=6)return 11;
 if(getifaddrs(0)!=-1||*__errno_location()!=14)return 12;
 for(unsigned repeat=0;repeat<40;++repeat){
  struct ifaddrs *head=(void *)1;
  if(getifaddrs(&head))return 13;
  if(!snapshot.index){if(head)return 14;freeifaddrs(head);continue;}
  if(!head||!same_name(head->name)||head->flags!=snapshot.flags||!head->address||head->netmask||head->broadcast||head->data)return 15;
  struct packet *packet=head->address;
  if(packet->family!=17||packet->index!=1||packet->hardware!=1||packet->length!=6)return 16;
  for(unsigned i=0;i<6;++i)if(packet->mac[i]!=snapshot.mac[i])return 17;
  struct ifaddrs *inet=head->next;
  if(!snapshot.configured){if(inet)return 18;}
  else{
   if(!inet||inet->next||!same_name(inet->name)||inet->flags!=snapshot.flags||inet->data)return 19;
   struct address *address=inet->address,*mask=inet->netmask,*broadcast=inet->broadcast;
   if(!address||!mask||!broadcast||address->family!=2||mask->family!=2||broadcast->family!=2)return 20;
   for(unsigned i=0;i<4;++i)if(address->ip[i]!=snapshot.address[i]||mask->ip[i]!=snapshot.netmask[i]||broadcast->ip[i]!=snapshot.broadcast[i])return 21;
  }
  freeifaddrs(head);
 }
 freeifaddrs(0);SbLogRaw("native NIC/DHCP/interface chain/ABI/errors/free PASS\n");return 89;
}
