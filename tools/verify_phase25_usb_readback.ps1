# Read-only Phase25 comparison for the selected Samsung DUO and pinned MyOS image.
# NEVER opens a device for write; no format/partition/mount/firmware operations.
# DiskNumber is mandatory and must be chosen by the owner from Get-Disk.
[CmdletBinding(DefaultParameterSetName='Disk')]
param(
    [Parameter(Mandatory=$true,ParameterSetName='Disk')]
    [ValidateRange(0,2147483647)][int]$DiskNumber,
    [Parameter(Mandatory=$true,ParameterSetName='Fixture')]
    [ValidateNotNullOrEmpty()][string]$FixturePath
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
function Assert-SelectedDisk($Disk,[int]$SelectedNumber) {
    if ($Disk.Number -ne $SelectedNumber -or
        $Disk.FriendlyName -cne 'Samsung Flash Drive DUO' -or
        [string]$Disk.BusType -ne 'USB' -or
        [uint64]$Disk.Size -ne 32078036992 -or
        [uint32]$Disk.LogicalSectorSize -ne 512 -or
        [uint32]$Disk.PhysicalSectorSize -ne 512 -or
        [string]$Disk.PartitionStyle -ne 'MBR' -or
        $Disk.IsBoot -ne $false -or $Disk.IsSystem -ne $false) {
        throw 'Selected disk identity does not match Samsung DUO USB/32078036992B/512B/MBR, or is a boot/system disk. Nothing written.'
    }
}
function Open-ReadOnlyStream([string]$Path) {
    # Use a read-only OS handle for Windows PowerShell5.1 device paths.
    if (-not ('Phase25ReadOnlyNative' -as [type])) {
        Add-Type -TypeDefinition @'
using System;
using System.IO;
using System.ComponentModel;
using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;
public static class Phase25ReadOnlyNative {
    [DllImport("kernel32.dll", EntryPoint="CreateFileW", CharSet=CharSet.Unicode, SetLastError=true)]
    private static extern SafeFileHandle CreateFile(string path, uint access,
        uint share, IntPtr security, uint disposition, uint flags, IntPtr template);
    public static FileStream Open(string path) {
        // GENERIC_READ only, FILE_SHARE_READ|WRITE, OPEN_EXISTING.
        SafeFileHandle handle = CreateFile(path, 0x80000000u, 3, IntPtr.Zero, 3, 0, IntPtr.Zero);
        if (handle.IsInvalid) {
            int error = Marshal.GetLastWin32Error(); handle.Dispose();
            throw new Win32Exception(error);
        }
        try { return new FileStream(handle, FileAccess.Read, 1, false); }
        catch { handle.Dispose(); throw; }
    }
}
'@
    }
    return [Phase25ReadOnlyNative]::Open($Path)
}
function Get-RegionDigest([System.IO.Stream]$Stream,[long]$Offset,[long]$Length) {
    [void]$Stream.Seek($Offset,[System.IO.SeekOrigin]::Begin)
    $hash = [System.Security.Cryptography.SHA256]::Create()
    $buffer = New-Object byte[] (1024*1024)
    try {
        $left = $Length
        while ($left -gt 0) {
            $want = [int][Math]::Min($buffer.Length,$left)
            $count = $Stream.Read($buffer,0,$want)
            if ($count -le 0) { throw 'Unexpected end of media during readback.' }
            [void]$hash.TransformBlock($buffer,0,$count,$buffer,0)
            $left -= $count
        }
        [void]$hash.TransformFinalBlock([byte[]]@(),0,0)
        return [System.BitConverter]::ToString($hash.Hash).Replace('-','').ToLowerInvariant()
    } finally { $hash.Dispose() }
}
# These digests are generated from the frozen67108864B image with the SHA below.
$regions = @(
    [pscustomobject]@{ name='boot-prefix'; offset=[long]0; bytes=[long]35651584; expected='65047f87ed20d372753176127e08977cd9d5dd0a57d618f46eb7c36e221457a7' },
    [pscustomobject]@{ name='data-marker'; offset=[long]35651584; bytes=[long]512; expected='8946a55bae244ba871d383ca57b5eaeb33b1d1a31bf0796e1082ba816dccc212' },
    [pscustomobject]@{ name='immutable-tail'; offset=[long]52428800; bytes=[long]14680064; expected='e86bae8c0598c4ff83c695f467daa4a1e8fa01d57f9140372993366204022a4d' }
)
$isFixture = $PSCmdlet.ParameterSetName -eq 'Fixture'
$disk = $null
if ($isFixture) {
    if ($FixturePath.StartsWith('\\.\') -or $FixturePath.StartsWith('\\?\')) {
        throw 'Fixture must be an ordinary image file, not a device path.'
    }
    $item = Get-Item -LiteralPath $FixturePath
    if ($item.PSIsContainer -or $item.Length -ne 67108864) { throw 'Fixture must be an ordinary67108864B image file.' }
    $path = $item.FullName
} else {
    $principal = New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
    if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
        throw 'Open Windows PowerShell as Administrator on the ThinkBook; read-only physical access requires it.'
    }
    $found = @(Get-Disk -Number $DiskNumber)
    if ($found.Count -ne 1) { throw 'Expected exactly one explicitly selected disk.' }
    $disk = $found[0]
    Assert-SelectedDisk $disk $DiskNumber
    $path = '\\.\PhysicalDrive' + $DiskNumber
}
# Both modes use the SAME tested read-only native open/stream path.
$stream = Open-ReadOnlyStream $path
$results = @()
try {
    foreach ($region in $regions) {
        $actual = Get-RegionDigest $stream $region.offset $region.bytes
        $results += [pscustomobject]@{
            name=$region.name; offset=$region.offset; bytes=$region.bytes
            expected_sha256=$region.expected; actual_sha256=$actual; match=($actual -ceq $region.expected)
        }
    }
} finally { $stream.Dispose() }
if (-not $isFixture) {
    # Recheck after reading, so removal/replacement cannot be silently accepted.
    $after = @(Get-Disk -Number $DiskNumber)
    if ($after.Count -ne 1) { throw 'Selected disk disappeared during readback.' }
    Assert-SelectedDisk $after[0] $DiskNumber
}
$allMatch = @($results | Where-Object { -not $_.match }).Count -eq 0
$identity = $null
if (-not $isFixture) {
    $identity = $disk | Select-Object Number,FriendlyName,BusType,Size,LogicalSectorSize,PhysicalSectorSize,PartitionStyle,IsBoot,IsSystem
}
[ordered]@{
    schema=1; captured_at=(Get-Date).ToString('o')
    scope=$(if ($isFixture) { 'file-fixture-only; NOT physical acceptance' } else { 'selected-physical-USB immutable regions; read-only' })
    physical=(-not $isFixture); result=$(if ($allMatch) { 'PASS' } else { 'FAIL' })
    reference_source_commit='6fc22f5ed84be859a8381de057b498c55b6b812c'
    reference_image_bytes=67108864
    reference_image_sha256='d66310b71d0ecb737941bf5a6dfb22448cf728d0211456de923a06d0effa85aa'
    device=$identity; regions=$results
    skipped_data=@{ offset=35652096; bytes=16776704; reason='Mutable MyOS snapshots; preserve saved files/generations' }
    ignored_capacity='Bytes beyond original64MiB image are not compared.'
    no_physical_write=$true
} | ConvertTo-Json -Depth 6
if (-not $allMatch) { exit 1 }
