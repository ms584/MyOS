"""Exercise production BOT validation and write bounds with a hostile transport.
No hardware I/O; irq helpers are replaced only in this native test translation.
"""
from pathlib import Path
import subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
SOURCE=r"""
#define MYOS_OS_H
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
static uint64_t irq_save(void){return 0;}
static void irq_restore(uint64_t f){(void)f;}
static uint64_t reported_opcode,reported_numbers[2];
static unsigned numbers;
static void print(const char *p){if(!strcmp(p,"USB BOT failure: opcode="))numbers=0;}
static void hex(uint64_t n){reported_opcode=n;}
static void decimal(uint64_t n){if(numbers<2)reported_numbers[numbers++]=n;}
#include "drivers/usb-storage.c"
static struct cbw request;
static unsigned stage,calls,writes,flushes;
static uint32_t last_write;
static bool bad_tag,bad_magic,bad_residue,bad_phase,transport_error;
static unsigned transport_error_phase;
static void little(uint8_t *p,uint32_t n){memcpy(p,&n,4);}
bool usb_bulk_transfer(void *h,bool input,void *buffer,unsigned length){
    assert(h==(void *)1);++calls;
    if(transport_error||(transport_error_phase&&stage==transport_error_phase-1))return false;
    if(!stage){
        assert(!input&&length==31);
        memcpy(&request,buffer,31);
        assert(request.magic==0x43425355&&request.lun==0&&request.cdb_length<=16);
        if(request.cdb[0]==0x2a){++writes;last_write=be32(request.cdb+2);}
        if(request.cdb[0]==0x35)++flushes;
        stage=request.length?1:2;return true;
    }
    if(stage==1){
        assert(length==request.length&&input==!!(request.flags&0x80));
        if(input){
            uint8_t *b=buffer;memset(b,0,length);
            if(request.cdb[0]==0x25){put_be32(b,131071);put_be32(b+4,512);}
            if(request.cdb[0]==0x28){
                uint32_t lba=be32(request.cdb+2);
                if(!lba){
                    b[510]=0x55;b[511]=0xaa;b[446]=0x80;b[450]=0xef;
                    little(b+454,2048);little(b+458,65536);
                    b[466]=0xda;little(b+470,DATA_LBA);little(b+474,DATA_SECTORS);
                }else if(lba==DATA_LBA)memcpy(b,"MYOS-DATA-v2",13);
            }
        }
        stage=2;return true;
    }
    assert(input&&length==13);
    struct csw response={bad_magic?0:0x53425355,request.tag+(bad_tag?1:0),bad_residue?1:0,bad_phase?2:0};
    memcpy(buffer,&response,13);stage=0;return true;
}
int main(void){
    uint8_t sector[512]={0};
    assert(!usb_storage_write(1,sector)&&!calls);
    usb_storage_attach((void *)1);
    assert(usb_storage_present()&&usb_storage_sectors()==DATA_SECTORS);
    assert(!writes&&flushes==1); /* Probe must never issue WRITE(10). */
    unsigned before=calls;
    assert(!usb_storage_write(0,sector));
    assert(!usb_storage_write(DATA_SECTORS,sector));
    assert(!usb_storage_write(UINT32_MAX,sector));
    assert(!usb_storage_write(1,NULL));
    assert(!usb_storage_read(DATA_SECTORS,sector)&&calls==before);
    assert(usb_storage_write(1,sector)&&last_write==DATA_LBA+1);
    assert(usb_storage_write(DATA_SECTORS-1,sector)&&last_write==DATA_LBA+DATA_SECTORS-1);
    assert(writes==2&&usb_storage_flush()&&flushes==2);
    for(unsigned fault=0;fault<7;++fault){
        bad_tag=bad_magic=bad_residue=bad_phase=transport_error=false;
        transport_error_phase=0;stage=0;ready=false;live=false;usb_storage_attach((void *)1);
        assert(usb_storage_present());
        if(!fault)bad_tag=true;
        if(fault==1)bad_magic=true;
        if(fault==2)bad_residue=true;
        if(fault==3)bad_phase=true;
        if(fault==4)transport_error=true;
        if(fault==5)transport_error_phase=2;
        if(fault==6)transport_error_phase=3;
        assert(!usb_storage_read(1,sector)&&!usb_storage_present());
        assert(reported_opcode==0x28&&numbers==2);
        assert(reported_numbers[0]==(fault==4?1:fault==5?2:3));
        assert(reported_numbers[1]==(fault<4));
        before=calls;
        assert(!usb_storage_write(1,sector)&&calls==before);
    }
    return 0;
}
"""
def main():
    with tempfile.TemporaryDirectory(dir=ROOT/'build',prefix='usb-bot-protocol-') as tmp:
        c=Path(tmp)/'test.c';exe=Path(tmp)/'test'
        c.write_text(SOURCE)
        subprocess.run(['gcc','-std=c11','-O1','-g','-Wall','-Wextra','-Werror',
                        '-fsanitize=address,undefined','-I'+str(ROOT),'-I'+str(ROOT/'include'),str(c),'-o',str(exe)],check=True)
        subprocess.run([str(exe)],check=True)
    print('PASS: production BOT wrappers; zero probe writes; LBA0/overflow/end bounds; mandatory cache flush; bad tag/signature/residue/phase/transport fail closed')
if __name__=='__main__':main()
