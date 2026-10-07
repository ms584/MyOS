/* Included after checked user-copy helpers. SYS_FS_OPERATION=56 is separate
 * from existing shared-map57 and volume58, preserving their current ABI. */
static int64_t fs_operation_call(struct process *p,struct frame *f)
{
    struct fs_operation_request r;
    /* INT80 three-argument ABI does not use R10. Never interpret the caller's
     * unrelated preserved register as an undocumented fourth argument. */
    if(f->rsi!=sizeof(r)||f->rdx)return -22;
    if(!copy_from_user(p,(char *)&r,f->rdi,sizeof(r)))return -14;
    if(r.operation>FS_CALL_RECORD_RELEASE)return -22;
    if(r.operation==FS_CALL_RECORD||r.operation==FS_CALL_RECORD_RELEASE){
        if(r.path||r.second_path||r.second_directory||r.value)return -22;
        if(r.operation==FS_CALL_RECORD_RELEASE){
            if(r.flags||r.buffer||r.length)return -22;
            return fs_record_release(p->files,r.directory);
        }
        if(r.flags!=RECORD_GET&&r.flags!=RECORD_SET&&r.flags!=RECORD_SET_WAIT)return -22;
        struct record_lock_abi lock;
        if(r.length!=sizeof(lock))return -22;
        /* GETLK is both input and output. Validate ALL output pages before
         * consulting/modifying state; no user VA is directly dereferenced. */
        if(r.flags==RECORD_GET&&!writable_range(p,r.buffer,sizeof(lock)))return -14;
        if(!copy_from_user(p,(char *)&lock,r.buffer,sizeof(lock)))return -14;
        int result=fs_record_lock(p->files,r.directory,p->pid,r.flags,&lock);
        if(!result&&r.flags==RECORD_GET)copy_to_user(p,r.buffer,&lock,sizeof(lock));
        return result;
    }
    char first[FS_PATH],second[FS_PATH];struct fs_entry info;
    int result=user_path(p,first,r.path);if(result)return result;
    if(r.operation==FS_CALL_OPEN_AT){
        if(r.second_directory||r.second_path||r.buffer||r.length||r.value||r.flags>127)return -22;
        return fs_open_at(p->files,r.directory,first,r.flags);
    }
    if(r.operation==FS_CALL_UNLINK_AT){
        if(r.second_directory||r.second_path||r.buffer||r.length||r.value||r.flags>1)return -22;
        return fs_unlink_at(p->files,r.directory,first,r.flags);
    }
    if(r.operation==FS_CALL_RENAME_AT){
        if(r.flags||r.buffer||r.length||r.value)return -22;
        result=user_path(p,second,r.second_path);if(result)return result;
        return fs_rename_at(p->files,r.directory,first,r.second_directory,second);
    }
    if(r.flags||r.second_directory||r.second_path||r.length!=sizeof(info)||
       (r.operation==FS_CALL_STAT_AT?r.value!=0:r.value>UINT32_MAX))return -22;
    if(!writable_range(p,r.buffer,sizeof(info)))return -14;
    result=r.operation==FS_CALL_STAT_AT?fs_stat_at(p->files,r.directory,first,&info):
        fs_list_at(p->files,r.directory,first,(unsigned)r.value,&info);
    if(result>=0&&(r.operation==FS_CALL_STAT_AT||result==1))copy_to_user(p,r.buffer,&info,sizeof(info));
    return result;
}
