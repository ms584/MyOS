/* PRIVATE batch prototype: pinned x64 little-endian musl message ABI.
 * One open-object pin and one stream operation for the entire vector, rather
 * than reacquiring a reused fd or blocking separately for each iovec.
 * No datagrams/ancillary transfer/MSG_WAITALL claim in this slice.
 */
struct message_abi {
    void *name;
    uint32_t name_length,pad_name;
    struct iovec_abi *vectors;
    int32_t vector_count,pad_vectors;
    void *control;
    uint32_t control_length,pad_control;
    int32_t flags,pad_flags;
};
_Static_assert(sizeof(struct message_abi)==56,"pinned musl msghdr size");
_Static_assert(offsetof(struct message_abi,vectors)==16,"pinned musl msg_iov");
_Static_assert(offsetof(struct message_abi,control_length)==40,"pinned musl controllen");

static int64_t stream_message(int fd,struct message_abi *message,int flags,int writing,
                             const struct descriptor_ref *pinned)
{
    if(!message)return set_error(EFAULT);
    struct message_abi header=*message;
    if(header.vector_count<0||header.vector_count>1024)return set_error(EINVAL);
    if(header.vector_count&&!header.vectors)return set_error(EFAULT);
    if(writing&&header.control_length)return set_error(ENOTSUP);
    /* Connected stream destination handling is explicitly unsupported rather
     * than silently routing a datagram to the existing peer. */
    if(writing&&header.name)return set_error(106); /* EISCONN */
    struct descriptor_ref reference={0};
    if(pinned){
        if(!descriptor_current(pinned))return set_error(EBADF);
        reference=*pinned;
        __atomic_add_fetch(&reference.object->references,1,__ATOMIC_RELAXED);
    }else if(!descriptor_acquire(fd,&reference))return set_error(EBADF);
    struct descriptor_object *object=reference.object;
    if(object->type!=DESCRIPTOR_TCP&&object->type!=DESCRIPTOR_SOCKETPAIR){
        object_release(object);return set_error(ENOTSOCK);
    }
    size_t total=0;
    for(int i=0;i<header.vector_count;++i){
        if(header.vectors[i].length>(size_t)INT64_MAX-total){
            object_release(object);return set_error(EINVAL);
        }
        if(header.vectors[i].length&&!header.vectors[i].base){
            object_release(object);return set_error(EFAULT);
        }
        total+=header.vectors[i].length;
    }
    unsigned char *buffer=total?malloc(total):0;
    if(total&&!buffer){object_release(object);return set_error(ENOMEM);}
    struct sockaddr_in_abi peer={0};int peer_length=0;
    if(!writing&&header.name){
        if(object->type==DESCRIPTOR_TCP){
            struct tcp_status_abi status;
            spin_lock(&object->lock);
            int64_t rc=call3(SYS_TCP_STATUS,object->value.tcp.handle,(uintptr_t)&status,0);
            spin_unlock(&object->lock);
            if(rc<0){free(buffer);object_release(object);return fail(rc);}
            peer.family=2;
            peer.port[0]=(unsigned char)(status.peer.port>>8);
            peer.port[1]=(unsigned char)status.peer.port;
            for(unsigned i=0;i<4;++i)peer.ip[i]=status.peer.ip[i];
            peer_length=sizeof(peer);
        }else{peer.family=1;peer_length=2;} /* unnamed AF_UNIX socketpair */
    }
    size_t offset=0;
    if(writing)for(int i=0;i<header.vector_count;++i){
        const unsigned char *source=header.vectors[i].base;
        for(size_t j=0;j<header.vectors[i].length;++j)buffer[offset++]=source[j];
    }
    /* socketpair_io_ref also supports TCP and releases our single pin on
     * every result, including close/reuse, EINTR and EOF. */
    int64_t result=socketpair_io_ref(&reference,buffer,total,flags,writing);
    if(result>=0&&!writing){
        offset=0;
        for(int i=0;i<header.vector_count&&offset<(size_t)result;++i){
            unsigned char *destination=header.vectors[i].base;
            size_t count=header.vectors[i].length;
            if(count>(size_t)result-offset)count=(size_t)result-offset;
            for(size_t j=0;j<count;++j)destination[j]=buffer[offset++];
        }
        if(header.name){
            size_t count=header.name_length;
            if(count>(size_t)peer_length)count=(size_t)peer_length;
            for(size_t i=0;i<count;++i)((unsigned char *)header.name)[i]=((unsigned char *)&peer)[i];
            message->name_length=(uint32_t)peer_length;
        }else message->name_length=0;
        message->control_length=0;message->flags=0;
    }
    free(buffer);return result;
}
int64_t sendmsg(int fd,const struct message_abi *message,int flags)
{return stream_message(fd,(struct message_abi *)message,flags,1,0);}
int64_t recvmsg(int fd,struct message_abi *message,int flags)
{return stream_message(fd,message,flags,0,0);}

int64_t sendto(int fd,const void *buffer,size_t length,int flags,
               const void *destination,uint32_t destination_length)
{
    struct iovec_abi vector={(void *)buffer,length};
    struct message_abi message={.name=(void *)destination,
        .name_length=destination_length,.vectors=&vector,.vector_count=1};
    return stream_message(fd,&message,flags,1,0);
}

/* Pinned musl x64 mmsghdr: 56-byte msghdr + uint32 result + alignment. */
struct multi_message_abi {struct message_abi message;uint32_t length,padding;};
_Static_assert(sizeof(struct multi_message_abi)==64,"pinned mmsghdr stride");
int recvmmsg(int fd,struct multi_message_abi *messages,unsigned count,int flags,
             struct timespec *timeout)
{
    const int wait_for_one=0x10000;
    if(!count||count>1024)return set_error(EINVAL);
    if(!messages)return set_error(EFAULT);
    if(flags&~(MSG_DONTWAIT|MSG_PEEK|wait_for_one))return set_error(ENOTSUP);
    uint64_t duration=0,start=0,deadline=0;
    if(timeout){
        int error=timespec_nanos(timeout,&duration);if(error)return set_error(error);
        error=clock_nanos(1,&start);if(error)return set_error(error);
        deadline=duration_deadline(duration,ticks());
    }
    struct descriptor_ref reference={0};
    if(!descriptor_acquire(fd,&reference))return set_error(EBADF);
    struct descriptor_object *object=reference.object;
    if(object->type!=DESCRIPTOR_TCP&&object->type!=DESCRIPTOR_SOCKETPAIR){
        object_release(object);return set_error(ENOTSOCK);
    }
    unsigned side=object->type==DESCRIPTOR_SOCKETPAIR?(reference.access>>2)&1:0;
    uint64_t receive_deadline=socket_deadline(socket_timeout(object,side,0));
    unsigned completed=0;int expired=0;
    for(;completed<count;){
        int current_flags=flags&~wait_for_one;
        if((flags&wait_for_one)&&completed)current_flags|=MSG_DONTWAIT;
        uint64_t sequence=__atomic_load_n(&object->sequence,__ATOMIC_ACQUIRE);
        /* With a caller timeout, never enter a receive whose independent
         * SO_RCVTIMEO could exceed the shared vector deadline. */
        int64_t result=stream_message(fd,&messages[completed].message,
            current_flags|(timeout?MSG_DONTWAIT:0),0,&reference);
        if(result>=0){
            messages[completed++].length=(uint32_t)result;
            receive_deadline=socket_deadline(socket_timeout(object,side,0));
            continue;
        }
        int error=*__errno_location();
        unsigned status=object->type==DESCRIPTOR_TCP?
            __atomic_load_n(&object->status_flags,__ATOMIC_ACQUIRE):
            __atomic_load_n(&object->value.socketpair.status_flags[(reference.access>>2)&1],__ATOMIC_ACQUIRE);
        if(error!=EAGAIN||!timeout||(current_flags&MSG_DONTWAIT)||(status&O_NONBLOCK))break;
        if(ticks()>=deadline){expired=1;break;}
        if(receive_deadline&&ticks()>=receive_deadline)break;
        uint64_t wait_deadline=receive_deadline&&receive_deadline<deadline?receive_deadline:deadline;
        int waited=wait64(&object->sequence,sequence,wait_deadline);
        if(waited==-ETIMEDOUT){expired=wait_deadline==deadline;break;}
        if(waited&&waited!=-EAGAIN){set_error(-waited);break;}
    }
    object_release(object);
    if(timeout){
        uint64_t now=0;int error=clock_nanos(1,&now);
        uint64_t elapsed=error||now<start?duration:now-start;
        uint64_t left=elapsed>=duration?0:duration-elapsed;
        *timeout=(struct timespec){(int64_t)(left/1000000000ull),(int64_t)(left%1000000000ull)};
    }
    if(completed)return (int)completed;
    if(expired)return 0;
    return -1;
}
