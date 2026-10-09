/* PRIVATE: include after user-copy validators in kernel/syscall.c, and route
 * SYSTEM_AUDIO here. Scheduler process-stop MUST release p->pid on every
 * exit/fault/kill; native lifetime proof required before production use. */
static int64_t pcm_call(struct process *p,struct frame *f)
{
    if(f->rdx)return -22;
    struct pcm_request request;
    if(!copy_from_user(p,(char *)&request,f->rsi,sizeof(request)))return -14;
    if(request.reserved||request.action>PCM_CONFIGURE)return -22;
    bool output=request.action==PCM_QUERY||request.action==PCM_ACQUIRE||request.action==PCM_STATUS;
    if(output){
        if(request.buffer||request.length)return -22;
        if(!writable_range(p,request.result,sizeof(struct pcm_status)))return -14;
    }else if(request.result)return -22;
    if(request.action==PCM_CONFIGURE){
        if(request.buffer||(request.length!=1024&&request.length!=8192))return -22;
    }else if(request.action!=PCM_QUEUE&&(request.buffer||request.length))return -22;
    struct audio_stream_status status={0};int result=0;
    switch(request.action){
    case PCM_QUERY:
        if(audio_ready())status=(struct audio_stream_status){0,0,0,65536,11025,1,8};
        break;
    case PCM_ACQUIRE:
        result=audio_stream_acquire(p->pid);
        if(!result)result=audio_stream_status(p->pid,&status);
        break;
    case PCM_STATUS:result=audio_stream_status(p->pid,&status);break;
    case PCM_QUEUE:{
        if(!request.length||request.length>4096)return -22;
        /* Whole request copied before queue mutation; never DMA from user VA. */
        unsigned char pcm[4096];
        if(!copy_from_user(p,(char *)pcm,request.buffer,request.length))return -14;
        return audio_stream_queue(p->pid,pcm,request.length);
    }
    case PCM_FLUSH:return audio_stream_flush(p->pid);
    case PCM_START:return audio_stream_start(p->pid);
    case PCM_CONFIGURE:return audio_stream_configure(p->pid,(unsigned)request.length);
    case PCM_RELEASE:
        result=audio_stream_status(p->pid,&status);
        if(!result)audio_stream_release(p->pid);return result;
    }
    if(!result&&output)copy_to_user(p,request.result,&status,sizeof(status));
    return result;
}
