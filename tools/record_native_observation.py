"""Mux native QEMU screen captures and captured SB16 PCM; no media fetch/codec decoding.
WAV backend records active samples only. Poll byte offsets map chunks onto wall time.
The replay is approximate (poll interval + WAV buffering), NOT a sync measurement.
Original PNG/WAV/timeline remain authoritative. Encodes screenshots as MJPEG/PCM AVI.
"""
import argparse,io,json,math,struct,wave,hashlib
from pathlib import Path
from PIL import Image
p=argparse.ArgumentParser(description=__doc__);p.add_argument('bundle',type=Path);p.add_argument('--fps',type=int,default=1);a=p.parse_args()
d=json.loads((a.bundle/'evidence.json').read_text())
with wave.open(str(a.bundle/'audio.wav'),'rb') as w:
    assert (w.getframerate(),w.getnchannels(),w.getsampwidth())==(11025,1,1)
    pcm=w.readframes(w.getnframes())
pack=lambda fmt,*v:struct.pack('<'+fmt,*v)
def chunk(tag,data):return tag+pack('I',len(data))+data+(b'\0' if len(data)%2 else b'')
def lst(tag,data):return chunk(b'LIST',tag+data)
for row in d['launches']:
    fs=row['frames'];ts=row['recording_timeline'];duration=row['elapsed'];count=math.ceil(duration*a.fps)
    sound=bytearray([128])*math.ceil(count*11025/a.fps);placed=0;overlaps=0;intervals=[]
    for prev,cur in zip(ts,ts[1:]):
        lo=max(0,prev['wav_bytes']-44);hi=min(len(pcm),max(lo,cur['wav_bytes']-44));data=pcm[lo:hi]
        if not data:continue
        end=round(cur['elapsed']*11025);start=max(0,end-len(data))
        data=data[-(end-start):];end=min(end,len(sound));data=data[:end-start]
        sound[start:end]=data;placed+=len(data)
        intervals.append({'wall_start':start/11025,'wall_end':end/11025,'source_offset':lo,'bytes':len(data)})
    avih=pack('14I',round(1e6/a.fps),0,0,0x10,count,0,2,0,800,600,0,0,0,0)
    def sh(kind,handler,scale,rate,length,sample):
        return pack('4s4sIHHIIIIIIIIhhhh',kind,handler,0,0,0,0,scale,rate,0,length,0,0xffffffff,sample,0,0,800 if kind==b'vids' else 0,600 if kind==b'vids' else 0)
    vh=sh(b'vids',b'MJPG',1,a.fps,count,0);vf=pack('IiiHH4sIiiII',40,800,600,1,24,b'MJPG',800*600*3,0,0,0,0)
    ah=sh(b'auds',b'\0'*4,1,11025,len(sound),1);af=pack('HHIIHH',1,1,11025,11025,1,8)
    header=lst(b'hdrl',chunk(b'avih',avih)+lst(b'strl',chunk(b'strh',vh)+chunk(b'strf',vf))+lst(b'strl',chunk(b'strh',ah)+chunk(b'strf',af)))
    dest=a.bundle/f"launch-{row['launch']}-screen-audio.avi";index=[]
    with dest.open('wb') as out:
        out.write(b'RIFF'+b'\0'*4+b'AVI '+header);movi=out.tell();out.write(b'LIST'+b'\0'*4+b'movi');base=out.tell()-4
        cursor=0;last_path=None;jpeg=None
        for n in range(count):
            time=n/a.fps
            while cursor+1<len(fs) and fs[cursor+1]['elapsed']<=time:cursor+=1
            path=fs[cursor]['path']
            if path!=last_path:
                buf=io.BytesIO();Image.open(a.bundle/path).convert('RGB').save(buf,format='JPEG',quality=85);jpeg=buf.getvalue();last_path=path
            for tag,data in [(b'00dc',jpeg),(b'01wb',bytes(sound[round(n*11025/a.fps):round((n+1)*11025/a.fps)]))]:
                index.append(pack('4sIII',tag,0x10,out.tell()-base,len(data)));out.write(chunk(tag,data))
        end=out.tell();out.seek(movi+4);out.write(pack('I',end-movi-8));out.seek(end);out.write(chunk(b'idx1',b''.join(index)));end=out.tell();out.seek(4);out.write(pack('I',end-8))
    (a.bundle/f"launch-{row['launch']}-recording-map.json").write_text(json.dumps({'phase_passed':False,'file':dest.name,'fps':a.fps,'duration':count/a.fps,'sha256':hashlib.sha256(dest.read_bytes()).hexdigest(),'placed_pcm_bytes':placed,'native_pcm_wall_intervals':intervals,'caveat':'Approximate screen/audio review only: byte polling plus WAV buffering. No resampling, pitch changes or online media decoding; original WAV/timestamps preserved. Native hardware-clock logs are sync evidence, not this mux.'},indent=2)+'\n')
    print(dest, dest.stat().st_size)
