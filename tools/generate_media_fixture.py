"""Generate deterministic standard WebM test input. Host ENCODING only.
No host decoding/rendering/playback is used or accepted as MyOS evidence.
VP8 encoder is the pinned upstream source, built separately without decoder.
Opus host encoder ABI is stable; record its version. Guest uses pinned Opus1.5.2.
"""
from pathlib import Path
import ctypes,hashlib,json,math,struct,subprocess
ROOT=Path(__file__).resolve().parents[1]
base=ROOT/'build/media-evaluation';out=base/'fixture-encoder'
out.mkdir(exist_ok=True)
code=r'''
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "vpx/vpx_encoder.h"
#include "vpx/vp8cx.h"
static void put32(unsigned v){fwrite(&v,4,1,stdout);}
int main(void){
 vpx_codec_ctx_t codec;vpx_codec_enc_cfg_t cfg;
 if(vpx_codec_enc_config_default(vpx_codec_vp8_cx(),&cfg,0))return 1;
 cfg.g_w=160;cfg.g_h=90;cfg.g_timebase.num=1;cfg.g_timebase.den=15;
 cfg.rc_target_bitrate=100;cfg.g_lag_in_frames=0;cfg.g_threads=1;
 cfg.kf_min_dist=15;cfg.kf_max_dist=15;
 if(vpx_codec_enc_init(&codec,vpx_codec_vp8_cx(),&cfg,0))return 2;
 vpx_image_t *im=vpx_img_alloc(0,VPX_IMG_FMT_I420,160,90,1);if(!im)return 3;
 for(int f=0;f<600;++f){
  for(int y=0;y<90;++y)for(int x=0;x<160;++x)
   im->planes[0][y*im->stride[0]+x]=((x+f*3)%160<24?220:40+(y/15)*20);
  for(int y=0;y<45;++y)for(int x=0;x<80;++x){
   im->planes[1][y*im->stride[1]+x]=90+(f/15)%5*18;
   im->planes[2][y*im->stride[2]+x]=170-(x/10)*8;
  }
  /* Binary frame identity cells, visibly change every frame. */
  for(int b=0;b<10;++b)for(int y=4;y<12;++y)for(int x=b*15+2;x<b*15+12;++x)
   im->planes[0][y*im->stride[0]+x]=(f&(1<<b))?235:16;
  if(vpx_codec_encode(&codec,im,f,1,0,VPX_DL_REALTIME))return 4;
  vpx_codec_iter_t it=0;const vpx_codec_cx_pkt_t *p;
  while((p=vpx_codec_get_cx_data(&codec,&it)))if(p->kind==VPX_CODEC_CX_FRAME_PKT){
   put32((unsigned)p->data.frame.pts);put32((unsigned)p->data.frame.flags);
   put32((unsigned)p->data.frame.sz);fwrite(p->data.frame.buf,1,p->data.frame.sz,stdout);
  }
 }
 vpx_img_free(im);vpx_codec_destroy(&codec);return 0;
}
'''
(out/'encode-vp8.c').write_text(code)
subprocess.run(['gcc','-O2','-I'+str(base/'libvpx-1.15.2'),str(out/'encode-vp8.c'),
 str(out/'libvpx.a'),'-lm','-lpthread','-o',str(out/'encode-vp8')],check=True)
raw=subprocess.check_output([str(out/'encode-vp8')]);video=[];offset=0
while offset<len(raw):
 pts,flags,n=struct.unpack_from('<III',raw,offset);offset+=12
 video.append((round(pts*1000/15),1,flags&1,raw[offset:offset+n]));offset+=n
assert len(video)==600
opus=ctypes.CDLL('libopus.so.0')
opus.opus_get_version_string.restype=ctypes.c_char_p
version=opus.opus_get_version_string().decode()
opus.opus_encoder_create.argtypes=[ctypes.c_int,ctypes.c_int,ctypes.c_int,ctypes.POINTER(ctypes.c_int)]
opus.opus_encoder_create.restype=ctypes.c_void_p
opus.opus_encode.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_int16),ctypes.c_int,ctypes.c_void_p,ctypes.c_int]
opus.opus_encoder_destroy.argtypes=[ctypes.c_void_p]
error=ctypes.c_int();encoder=opus.opus_encoder_create(48000,1,2049,ctypes.byref(error))
assert encoder and error.value==0
# Encoder lookahead query; pointer explicit for varargs ABI.
lookahead=ctypes.c_int();opus.opus_encoder_ctl(ctypes.c_void_p(encoder),ctypes.c_int(4027),ctypes.byref(lookahead))
audio=[];buffer=ctypes.create_string_buffer(4096);pcm=(ctypes.c_int16*960)()
for packet in range(2000):
 for i in range(960):
  sample=packet*960+i;hz=(440,660,880,550)[sample//480000]
  pcm[i]=round(10000*math.sin(2*math.pi*hz*sample/48000))
 n=opus.opus_encode(encoder,pcm,960,buffer,4096);assert n>0
 audio.append((packet*20,2,1,buffer.raw[:n]))
opus.opus_encoder_destroy(encoder)
def vint(n):
 for size in range(1,9):
  if n<(1<<(7*size))-1:return ((1<<(7*size))|n).to_bytes(size,'big')
 raise ValueError(n)
def el(tag,data):return bytes.fromhex(tag)+vint(len(data))+data
def uint(n):return n.to_bytes(max(1,(n.bit_length()+7)//8),'big')
def u(tag,n):return el(tag,uint(n))
def text(tag,s):return el(tag,s.encode())
header=el('1a45dfa3',u('4286',1)+u('42f7',1)+u('42f2',4)+u('42f3',8)+text('4282','webm')+u('4287',4)+u('4285',2))
info=el('1549a966',u('2ad7b1',1000000)+el('4489',struct.pack('>d',40000))+text('4d80','MyOS standard fixture encoder')+text('5741','MyOS Phase33.3'))
vt=el('ae',u('d7',1)+u('73c5',1)+u('83',1)+text('86','V_VP8')+u('23e383',66666667)+el('e0',u('b0',160)+u('ba',90)))
head=b'OpusHead'+bytes([1,1])+struct.pack('<HIhB',lookahead.value,48000,0,0)
at=el('ae',u('d7',2)+u('73c5',2)+u('83',2)+text('86','A_OPUS')+el('63a2',head)+u('56aa',lookahead.value*1000000000//48000)+u('56bb',80000000)+el('e1',el('b5',struct.pack('>d',48000))+u('9f',1)))
packets=sorted(video+audio,key=lambda x:(x[0],x[1]))
clusters=[]
for start in range(0,40000,1000):
 blocks=[]
 for ts,track,key,data in packets:
  if start<=ts<start+1000:
   blocks.append(el('a3',bytes([0x80|track])+struct.pack('>h',ts-start)+bytes([0x80 if key else 0])+data))
 clusters.append(el('1f43b675',u('e7',start)+b''.join(blocks)))
payload=header+el('18538067',info+el('1654ae6b',vt+at)+b''.join(clusters))
asset=base/'standard-160x90-40s.webm';asset.write_bytes(payload)
report={'asset':str(asset),'sha256':hashlib.sha256(payload).hexdigest(),'bytes':len(payload),
'width':160,'height':90,'fps':15,'duration_seconds':40,'video_frames':600,'opus_packets':2000,
'encoder_only_host_opus_version':version,'opus_preskip_samples':lookahead.value,
'host_decoding':False,'guest_acceptance':False,'generation':'deterministic raw moving cells and sine PCM, encoded VP8+Opus, standard WebM mux'}
(base/'standard-fixture.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
