#ifndef MYOS_ELF_LAUNCH_H
#define MYOS_ELF_LAUNCH_H
#include <stdbool.h>
#include <stddef.h>
#define ELF_ARGV_OFFSET 512u
#define ELF_ARGV_SLOTS 9u
#define ELF_NAME_OFFSET (ELF_ARGV_OFFSET+8u*ELF_ARGV_SLOTS)
#define ELF_HOST_RULE_SIZE 300u
/* An explicit HOST=IPv4 destination rule changes routing, never TLS identity.
 * Reject resolver grammar metacharacters and ambiguous numeric addresses. */
static inline bool elf_host_rule(const char *mapping,char *output,size_t capacity)
{
    if(!mapping){if(!capacity)return false;output[0]=0;return true;}
    size_t host=0,label=0;bool last_alnum=false;
    while(host<=253&&mapping[host]&&mapping[host]!='='){
        unsigned char c=mapping[host];
        bool alnum=(c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9');
        if(c=='.'){if(!label||!last_alnum)return false;label=0;}
        else{if(!alnum&&c!='-')return false;if(!label&&!alnum)return false;if(++label>63)return false;}
        last_alnum=alnum;++host;
    }
    if(!host||host>253||!label||!last_alnum||mapping[host]!='=')return false;
    size_t end=host+1;
    for(unsigned part=0;part<4;++part){unsigned value=0,digits=0;size_t first=end;
        while(mapping[end]>='0'&&mapping[end]<='9'){
            if(++digits>3)return false;
            value=value*10+(unsigned)(mapping[end++]-'0');}
        if(!digits||value>255||(digits>1&&mapping[first]=='0'))return false;
        if(part<3){if(mapping[end++]!='.')return false;}else if(mapping[end])return false;
    }
    static const char prefix[]="--host-resolver-rules=MAP ";
    size_t needed=sizeof(prefix)-1+end+1;
    if(capacity<needed)return false;
    size_t used=0;for(size_t i=0;i<sizeof(prefix)-1;++i)output[used++]=prefix[i];
    for(size_t i=0;i<end;++i)output[used++]=i==host?' ':mapping[i];
    output[used]=0;return true;
}
#endif
