#include "tcp-server-call.h"
/* AF_INET stream server adapter. Owned generation stays pinned across wait. */
int bind(int fd,const void *raw,uint32_t length)
{
    if(!raw)return set_error(EFAULT);
    if(length<sizeof(struct sockaddr_in_abi))return set_error(EINVAL);
    const struct sockaddr_in_abi *address=raw;
    if(address->family!=2)return set_error(97);
    struct tcp_address_abi local={{0},(uint16_t)(((unsigned)address->port[0]<<8)|address->port[1]),0};
    for(unsigned i=0;i<4;++i)local.ip[i]=address->ip[i];
    struct descriptor_ref reference={0};
    if(!descriptor_acquire(fd,&reference))return set_error(EBADF);
    if(reference.object->type!=DESCRIPTOR_TCP){object_release(reference.object);return set_error(ENOTSOCK);}
    spin_lock(&reference.object->lock);
    int64_t result=call3(SYS_TCP_BIND,reference.object->value.tcp.handle,(uintptr_t)&local,0);
    spin_unlock(&reference.object->lock);object_release(reference.object);
    return result<0?fail(result):0;
}
int listen(int fd,int backlog)
{
    if(backlog<0)return set_error(EINVAL);
    struct descriptor_ref reference={0};
    if(!descriptor_acquire(fd,&reference))return set_error(EBADF);
    if(reference.object->type!=DESCRIPTOR_TCP){object_release(reference.object);return set_error(ENOTSOCK);}
    spin_lock(&reference.object->lock);
    int64_t result=call3(SYS_TCP_LISTEN,reference.object->value.tcp.handle,(unsigned)backlog,0);
    spin_unlock(&reference.object->lock);object_release(reference.object);
    return result<0?fail(result):0;
}
int accept(int fd,void *address,uint32_t *length)
{
    if(address&&!length)return set_error(EFAULT);
    /* Snapshot input before dequeuing; a bad output pointer faults this user
     * process, never produces unchecked kernel writes. */
    uint32_t capacity=address?*length:0;
    struct descriptor_ref reference={0};
    if(!descriptor_acquire(fd,&reference))return set_error(EBADF);
    struct descriptor_object *listener=reference.object;
    if(listener->type!=DESCRIPTOR_TCP){object_release(listener);return set_error(ENOTSOCK);}
    struct descriptor_state *state=descriptor_state();
    struct descriptor_object *object=descriptor_object(DESCRIPTOR_TCP);
    if(!object){object_release(listener);return set_error(ENOMEM);}
    struct tcp_accept_request request={.object_sequence=(uintptr_t)&object->sequence,
        .registry_sequence=(uintptr_t)&state->sequence};
    uint64_t deadline=socket_deadline(socket_timeout(listener,0,0));int result=-1;
    for(;;){
        if(!descriptor_current(&reference)){set_error(EBADF);break;}
        /* Same state->object lock ordering as close. Reserve table capacity
         * before kernel dequeue, so EMFILE cannot consume a queued peer. */
        spin_lock(&state->lock);unsigned slot=0;
        while(slot<FD_SLOTS&&state->slots[slot].object)++slot;
        if(slot==FD_SLOTS){spin_unlock(&state->lock);set_error(24);break;}
        spin_lock(&listener->lock);
        uint64_t sequence=__atomic_load_n(&listener->sequence,__ATOMIC_ACQUIRE);
        int64_t accepted=call3(SYS_TCP_ACCEPT,listener->value.tcp.handle,(uintptr_t)&request,sizeof(request));
        unsigned status=__atomic_load_n(&listener->status_flags,__ATOMIC_ACQUIRE);
        if(accepted>=0){
            object->value.tcp.handle=(uint32_t)accepted;object->value.tcp.opens=1;
            object->receive_timeout[0]=listener->receive_timeout[0];
            object->send_timeout[0]=listener->send_timeout[0];
            /* Plain Linux-compatible accept does not inherit NONBLOCK/CLOEXEC. */
            result=descriptor_install_locked(state,object,3,0);
        }
        spin_unlock(&listener->lock);spin_unlock(&state->lock);
        if(accepted>=0)break;
        if(accepted!=-EAGAIN){fail(accepted);break;}
        if((status&O_NONBLOCK)||(deadline&&ticks()>=deadline)){set_error(EAGAIN);break;}
        int waited=wait64(&listener->sequence,sequence,deadline);
        if(waited==-ETIMEDOUT){set_error(EAGAIN);break;}
        if(waited&&waited!=-EAGAIN){set_error(-waited);break;}
    }
    object_release(listener);
    if(result<0){free(object);return -1;}
    if(address){
        struct sockaddr_in_abi peer={2,{(unsigned char)(request.peer_port>>8),(unsigned char)request.peer_port},{0},{0}};
        for(unsigned i=0;i<4;++i)peer.ip[i]=request.peer_ip[i];
        if(capacity>sizeof(peer))capacity=sizeof(peer);
        for(unsigned i=0;i<capacity;++i)((unsigned char *)address)[i]=((unsigned char *)&peer)[i];
        *length=sizeof(peer);
    }
    return result;
}
