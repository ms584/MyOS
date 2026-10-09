"""ASan/UBSan bounded WebM parser corpus; no host codec or playback."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]
base=root/'build/media-evaluation'
out=Path(tempfile.mkdtemp(prefix='parser-safety-',dir=base))
source=out/'corpus.c'
source.write_text(r'''
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "media-webm.h"
struct input {const unsigned char *p;size_t left;};
static int read_input(void *ctx,void *out,size_t n){
 struct input *s=ctx;if(n>s->left)return -1;
 memcpy(out,s->p,n);s->p+=n;s->left-=n;return 0;
}
static int packet(void *ctx,int type,const unsigned char *data,size_t size,int64_t timestamp,int key){
 (void)ctx;(void)type;(void)data;(void)size;(void)timestamp;(void)key;return 0;
}
static void parse(const unsigned char *data,size_t n){
 struct input input={data,n};struct myos_webm_source source={read_input,&input,n};
 (void)myos_webm_demux(&source,packet,0);
}
int main(int argc,char **argv){
 if(argc!=2)return 1;
 FILE *f=fopen(argv[1],"rb");if(!f)return 2;
 fseek(f,0,SEEK_END);long length=ftell(f);rewind(f);
 if(length<1||length>1048576)return 3;
 unsigned char *data=malloc((size_t)length),*copy=malloc((size_t)length);
 if(!data||!copy)return 4;if(fread(data,1,(size_t)length,f)!=(size_t)length)return 5;fclose(f);
 parse(data,(size_t)length);
 for(size_t n=0;n<1024&&n<(size_t)length;++n)parse(data,n);
 unsigned random=1;
 for(unsigned i=0;i<2000;++i){
  memcpy(copy,data,(size_t)length);
  random=random*1664525u+1013904223u;
  size_t position=random%(size_t)length;
  copy[position]^=(unsigned char)(1u<<(i%8));parse(copy,(size_t)length);
 }
 for(unsigned i=0;i<2000;++i){
  unsigned char bytes[128];
  for(unsigned j=0;j<sizeof(bytes);++j){random=random*1664525u+1013904223u;bytes[j]=(unsigned char)(random>>24);}
  parse(bytes,i%sizeof(bytes));
 }
 free(copy);free(data);puts("PASS bounded parser ASan/UBSan corpus; no host decoder");return 0;
}
''')
binary=out/'corpus'
subprocess.run(['gcc','-std=c11','-O1','-g','-fsanitize=address,undefined',
 '-fno-omit-frame-pointer','-Iuser','user/media-webm.c',str(source),'-o',str(binary)],cwd=root,check=True)
subprocess.run([str(binary),str(base/'opus-trimming-video-test.webm')],check=True)
print(out)
