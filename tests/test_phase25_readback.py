"""Run the production read-only script in Windows PowerShell5.1 against files.
Never invokes Get-Disk or a valid physical DiskNumber. Guard tests extract only
Assert-SelectedDisk from the production AST, then supply synthetic metadata.
"""
from pathlib import Path
import subprocess,sys,json,hashlib,shutil
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'build/phase25-readback-tests'
SCRIPT=ROOT/'tools/verify_phase25_usb_readback.ps1'
IMAGE=ROOT/'build/phase24-usb-bot-clock-2026-10-24/images/myos-usb-data.img'
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def win(p):return subprocess.check_output(['wslpath','-w',str(p)],text=True).strip()
def powershell(args):
    return subprocess.run(['powershell.exe','-NoProfile','-NonInteractive','-ExecutionPolicy','Bypass',*args],
        stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=90)
def main():
    OUT.mkdir(exist_ok=True)
    initial=digest(IMAGE)
    assert initial=='d66310b71d0ecb737941bf5a6dfb22448cf728d0211456de923a06d0effa85aa'
    paths=[]
    fixture=OUT/'generated image fixture.img'
    if fixture.exists():raise AssertionError('Fixture exists; inspect preserved output before rerun')
    shutil.copyfile(IMAGE,fixture)
    reports=[]
    def verify(label,expect,failed=None):
        before=digest(fixture)
        p=powershell(['-File',win(SCRIPT),'-FixturePath',win(fixture)])
        (OUT/(label+'.stdout')).write_bytes(p.stdout);(OUT/(label+'.stderr')).write_bytes(p.stderr)
        assert p.returncode==(0 if expect else 1),(label,p.stdout,p.stderr)
        d=json.loads(p.stdout.decode('utf-8-sig'))
        assert not d['physical'] and d['scope'].startswith('file-fixture-only')
        assert d['no_physical_write'] and d['result']==('PASS' if expect else 'FAIL')
        assert [v['name'] for v in d['regions'] if not v['match']]==([] if expect else [failed])
        assert [(v['offset'],v['bytes']) for v in d['regions']]==[(0,35651584),(35651584,512),(52428800,14680064)]
        with fixture.open('rb') as f:
            for v in d['regions']:
                f.seek(v['offset']);assert hashlib.sha256(f.read(v['bytes'])).hexdigest()==v['actual_sha256']
        assert digest(fixture)==before,'Production verifier modified its input'
        reports.append(dict(name=label,exit_code=p.returncode,expected_result=d['result'],input_unchanged=True))
    verify('pristine',True)
    with fixture.open('r+b') as f:f.seek(35651584+8192);f.write(b'legitimate saved generation14')
    assert digest(fixture)!=initial
    verify('mutable-save',True)
    def flip(offset):
        with fixture.open('r+b') as f:f.seek(offset);b=f.read(1);f.seek(offset);f.write(bytes([b[0]^1]))
    flip(10000);verify('corrupt-boot',False,'boot-prefix');flip(10000)
    flip(35651584);verify('corrupt-marker',False,'data-marker');flip(35651584)
    flip(52428800+512);verify('corrupt-tail',False,'immutable-tail');flip(52428800+512)
    short=OUT/'generated short.img';short.write_bytes(b'not a complete image')
    for label,args in [('short',['-FixturePath',win(short)]),('missing-selection',[]),('negative-number',['-DiskNumber','-1']),('device-in-fixture',['-FixturePath',r'\\.\PhysicalDrive1'])]:
        p=powershell(['-File',win(SCRIPT),*args]);assert p.returncode,(label,p.stdout,p.stderr)
        if label=='device-in-fixture':assert b'not a device path' in p.stderr
        reports.append(dict(name=label,rejected=True))
    unit=OUT/'metadata-guards.ps1'
    unit.write_text(r"""
param([string]$Script)
$ErrorActionPreference='Stop'
$tokens=$null;$errors=$null
$ast=[System.Management.Automation.Language.Parser]::ParseFile($Script,[ref]$tokens,[ref]$errors)
if($errors.Count){throw $errors[0]}
$deviceLiteral=@($ast.FindAll({param($n) $n -is [System.Management.Automation.Language.StringConstantExpressionAst] -and $n.Value -eq '\\.\PhysicalDrive'},$true))
if($deviceLiteral.Count -ne 1){throw 'Invalid Windows raw device path literal'}
foreach($name in @('Assert-SelectedDisk','Get-RegionDigest')){
 $node=$ast.Find({param($n) $n -is [System.Management.Automation.Language.FunctionDefinitionAst] -and $n.Name -eq $name},$true)
 if(-not $node){throw 'Missing production function'}
 . ([scriptblock]::Create($node.Extent.Text))
}
$good=[pscustomobject]@{Number=1;FriendlyName='Samsung Flash Drive DUO';BusType='USB';Size=[uint64]32078036992;LogicalSectorSize=512;PhysicalSectorSize=512;PartitionStyle='MBR';IsBoot=$false;IsSystem=$false}
Assert-SelectedDisk $good 1
$changes=@(@('Number',0),@('FriendlyName','Other USB'),@('BusType','NVMe'),@('Size',256060514304),@('LogicalSectorSize',4096),@('PhysicalSectorSize',4096),@('PartitionStyle','GPT'),@('IsBoot',$true),@('IsSystem',$true))
foreach($change in $changes){
 $bad=$good.PSObject.Copy();$bad.($change[0])=$change[1];$rejected=$false
 try{Assert-SelectedDisk $bad 1}catch{$rejected=$true}
 if(-not $rejected){throw ('Accepted bad metadata '+$change[0])}
}
$stream=[IO.MemoryStream]::new([byte[]](1,2,3),$false);$rejected=$false
try{Get-RegionDigest $stream 0 512}catch{$rejected=$true}finally{$stream.Dispose()}
if(-not $rejected){throw 'Accepted premature EOF'}
'PASS: production disk identity/number/bus/capacity/sector/MBR/boot/system guards and EOF; no device opened'
""")
    p=powershell(['-File',win(unit),'-Script',win(SCRIPT)])
    (OUT/'metadata-guards.stdout').write_bytes(p.stdout);(OUT/'metadata-guards.stderr').write_bytes(p.stderr)
    assert p.returncode==0,(p.stdout,p.stderr)
    assert digest(IMAGE)==initial
    (OUT/'results.json').write_text(json.dumps(dict(script_sha256=digest(SCRIPT),reference_image_sha256=initial,tests=reports,metadata_guards='PASS',physical_readback='NOT RUN',no_physical_device_opened=True),indent=2)+'\n')
    print('PASS: Windows PowerShell production verifier; pristine and modified-save fixtures; boot/marker/tail corruption rejected; independent hashes, inputs unchanged; bad identity/number/bus/size/sector/MBR/boot/system rejected; missing/negative selection, device fixture, short/EOF rejected. File fixtures only, no physical disk opened.')
if __name__=='__main__':main()
