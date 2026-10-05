"""Real guest TCP server: host only drives deterministic peers, not transport."""
from pathlib import Path
import socket
import subprocess
import sys
import tempfile
import time
import re
from test_system import ROOT,VM
from test_shell import command,type_text
from test_elf_dynamic import assert_live_resources
sys.path.insert(0,str(ROOT/'tools'))
from make_asset_disk import create
out=Path(tempfile.mkdtemp(prefix='tcp-server-',dir=ROOT/'build'))
assets=[]
for name,defines in [('server',[]),('servercancel',['-DCANCEL_ONLY'])]:
 obj=out/(name+'.o');lib=out/(name+'.so')
 subprocess.run(['gcc','-m64','-std=c11','-Os','-fPIC','-ffreestanding','-fno-builtin',
  '-mno-red-zone','-fno-stack-protector','-fno-asynchronous-unwind-tables','-fno-unwind-tables',
  '-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-unused-variable','-Iuser/sdk',*defines,
  '-c','user/elf-server.c','-o',str(obj)],check=True)
 subprocess.run(['ld','-shared','--hash-style=both','-z','now','-z','relro','-z','noexecstack',
  '-e','_start','-o',str(lib),str(obj)],check=True)
 assets.append(name+'='+str(lib))
disk=out/'assets.img';create(disk,assets)
with socket.socket() as reservation:
 reservation.bind(('127.0.0.1',0));port=reservation.getsockname()[1]
uefi='--uefi' in sys.argv
options=dict(ram=64,boot_image=ROOT/'build'/('myos-uefi.img' if uefi else 'myos-current.img'),
 asset_disk=disk,nic=f'user,model=rtl8139,hostfwd=tcp:127.0.0.1:{port}-:46080')
if uefi:options.update(boot_bus='ide',firmware=(Path('/usr/share/OVMF/OVMF_CODE_4M.fd'),Path('/usr/share/OVMF/OVMF_VARS_4M.fd')))
vm=VM('tcp-server'+('-uefi' if uefi else ''),**options)
def wait_since(offset,text,timeout=20):
 end=time.monotonic()+timeout
 while time.monotonic()<end:
  result=vm.log.read_text()[offset:]
  assert 'PANIC' not in result,result
  if text in result:return result
  if result.endswith('myos> '):raise AssertionError(result)
  time.sleep(.05)
 raise AssertionError(vm.log.read_text()[offset:])
try:
 vm.wait('myos> ')
 network=vm.command('human-monitor-command',{'command-line':'info network'})
 match=re.search(r'(\S+): index=\d+,type=user,',network);assert match,network
 vm.command('object-add',{'qom-type':'filter-dump','id':'tcp-server-capture','netdev':match.group(1),'file':str(out/'traffic.pcap')})
 result=command(vm,'net up');assert 'Network: DHCP configured' in result,result
 for launch in range(3):
  offset=len(vm.log.read_text());type_text(vm,'elf live server\n')
  wait_since(offset,'native server ready')
  peers=[]
  try:
   for c in range(2):
    peer=socket.create_connection(('127.0.0.1',port),timeout=10);peers.append(peer)
    peer.sendall(bytes((i*17+c)&255 for i in range(4096)))
   for c,peer in enumerate(peers):
    payload=bytearray()
    while len(payload)<4096:
     data=peer.recv(4096-len(payload))
     if not data:
      result=wait_since(offset,'myos> ',10)
      raise AssertionError(('early EOF',len(payload),result))
     payload.extend(data)
    assert payload==bytes((i*17+c)&255 for i in range(4096))
    peer.sendall(b'\xa5');peer.shutdown(socket.SHUT_WR)
   result=wait_since(offset,'myos> ',30)
   assert 'native TCP server handshake/queue/EMFILE/echo/close-reuse PASS' in result,result
   assert 'ELF exited with status 88' in result,result
   assert_live_resources(result)
  finally:
   for peer in peers:peer.close()
  print('PASS native TCP server launch',launch,flush=True)
 offset=len(vm.log.read_text());type_text(vm,'elf live servercancel\n')
 wait_since(offset,'native server cancel ready')
 peer=socket.create_connection(('127.0.0.1',port),timeout=10)
 try:
  peer.sendall(b'pending resource');time.sleep(.1);vm.key('esc')
  result=wait_since(offset,'myos> ')
  assert 'ELF cancelled; all threads stopped' in result,result;assert_live_resources(result)
 finally:peer.close()
finally:vm.close()
print('PASS real guest TCP server scoped BIOS/UEFI profile; no Cobalt web claim.',out,flush=True)
