#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "elf-launch.h"
int main(void)
{
    char result[ELF_HOST_RULE_SIZE];
    assert(ELF_NAME_OFFSET==584&&ELF_ARGV_SLOTS==9);
    assert(elf_host_rule(0,result,sizeof(result))&&!result[0]);
    const char *valid="--host-resolver-rules=MAP cobalt.example.com 10.0.2.2";
    assert(elf_host_rule("cobalt.example.com=10.0.2.2",result,sizeof(result)));
    assert(!strcmp(result,valid));
    assert(elf_host_rule("cobalt.example.com=10.0.2.2",result,strlen(valid)+1));
    assert(!elf_host_rule("cobalt.example.com=10.0.2.2",result,strlen(valid)));
    const char *bad[]={"","=1.2.3.4","a=","a=1.2.3","a=1.2.3.4.5",
        "a=256.2.3.4","a=01.2.3.4","a=1.2.3.4:443","a=1.2.3.4,EXCLUDE *",
        "a b=1.2.3.4","*=1.2.3.4","a..b=1.2.3.4","-a=1.2.3.4",
        "a-=1.2.3.4","a.=1.2.3.4","a/b=1.2.3.4","a=::1","a=1.2.3.-4"};
    for(size_t i=0;i<sizeof(bad)/sizeof(*bad);++i)
        assert(!elf_host_rule(bad[i],result,sizeof(result)));
    char mapping[280];memset(mapping,'a',64);strcpy(mapping+64,"=1.2.3.4");
    assert(!elf_host_rule(mapping,result,sizeof(result)));
    unsigned used=0;for(unsigned label=0;label<4;++label){
        unsigned length=label==3?61:63;
        memset(mapping+used,'a',length);used+=length;
        if(label<3)mapping[used++]='.';
    }
    assert(used==253);strcpy(mapping+used,"=255.255.255.255");
    assert(elf_host_rule(mapping,result,sizeof(result)));
    mapping[252]='.';assert(!elf_host_rule(mapping,result,sizeof(result)));
    puts("PASS: explicit bounded native launch route; grammar/IPv4/capacity/layout");
}
