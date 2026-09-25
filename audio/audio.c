#include "audio.h"
#include "fs.h"

static struct fs_handle handles[FS_HANDLES];
static bool same4(const uint8_t *p,const char *s){for(unsigned i=0;i<4;++i)if(p[i]!=(uint8_t)s[i])return false;return true;}
static uint32_t le32(const uint8_t *p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static uint16_t le16(const uint8_t *p){return (uint16_t)(p[0]|((uint16_t)p[1]<<8));}
bool audio_tone(unsigned hz,unsigned milliseconds){
    if(!audio_ready()||hz<50||hz>2000||!milliseconds||milliseconds>4000)return false;
    size_t count=(size_t)milliseconds*11025/1000;
    uint8_t *samples=kmalloc(count);if(!samples)return false;
    uint32_t phase=0;
    for(size_t i=0;i<count;++i){phase+=hz;if(phase>=11025)phase-=11025;samples[i]=phase<5512?208:48;}
    bool ok=audio_queue(samples,count);kfree(samples);return ok;
}
bool audio_play_wav(const char *path){
    if(!audio_ready())return false;
    struct fs_entry entry;if(fs_stat(path,&entry)<0||entry.type!=FS_FILE||entry.size<44||entry.size>FS_FILE_MAX)return false;
    uint8_t *bytes=kmalloc((size_t)entry.size);if(!bytes)return false;
    int fd=fs_open(handles,path,FS_READ);
    if(fd<0){kfree(bytes);return false;}
    int64_t got=fs_read(handles,fd,bytes,(size_t)entry.size);(void)fs_close(handles,fd);
    bool ok=false;
    if(got==(int64_t)entry.size && same4(bytes,"RIFF") && same4(bytes+8,"WAVE")){
        size_t pos=12;bool fmt=false;const uint8_t *pcm=0;size_t count=0;
        while(pos+8<=(size_t)got){
            uint32_t size=le32(bytes+pos+4);size_t begin=pos+8;
            if(size>(size_t)got-begin)break;
            if(same4(bytes+pos,"fmt ")&&size>=16){
                fmt=le16(bytes+begin)==1&&le16(bytes+begin+2)==1&&
                    le32(bytes+begin+4)==11025&&le16(bytes+begin+12)==1&&le16(bytes+begin+14)==8;
            }
            if(same4(bytes+pos,"data")){pcm=bytes+begin;count=size;}
            pos=begin+size+(size&1u);
        }
        if(fmt&&pcm&&count)ok=audio_queue(pcm,count);
    }
    kfree(bytes);return ok;
}
void audio_print_status(void){
    if(!audio_ready()){print("Audio: no SB16 device\n");return;}
    print("Audio: SB16 11025 Hz 8-bit mono; volume=");decimal(audio_volume());
    print(audio_muted()?" muted":" unmuted");
    print("; queued=");decimal(audio_queued());
    print("; IRQs=");decimal(audio_interrupts());
    print("; underruns=");decimal(audio_underruns());print("\n");
}
