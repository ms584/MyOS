"""Read-only KVM API probe; no VM or system configuration changes."""
import os,fcntl,json
result={'device':'/dev/kvm','euid':os.geteuid(),'groups':os.getgroups()}
try:
    fd=os.open('/dev/kvm',os.O_RDWR|os.O_CLOEXEC)
    try:result['api_version']=fcntl.ioctl(fd,0xae00,0)
    finally:os.close(fd)
except OSError as error:result.update(error=str(error),errno=error.errno)
print(json.dumps(result,indent=2))
