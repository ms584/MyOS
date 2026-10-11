$ErrorActionPreference = 'Stop'
$taskPrincipal = [Security.Principal.WindowsPrincipal]::new([Security.Principal.WindowsIdentity]::GetCurrent())
if (-not $taskPrincipal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) { throw 'Open PowerShell as Administrator, then run this script.' }
$taskExpectedPath = 'C:\Users\Xytron\AppData\Local\wsl\{87aa3900-660f-45f7-81fd-4b21dfb5ca96}\ext4.vhdx'
$taskDisk = Get-Item -LiteralPath $taskExpectedPath
if ($taskDisk.FullName -ne $taskExpectedPath -or $taskDisk.Name -ne 'ext4.vhdx') { throw 'Unexpected VHDX path; aborting.' }
$taskBeforeBytes = $taskDisk.Length
$taskBeforeFree = ([System.IO.DriveInfo]::new('C:\')).TotalFreeSpace
$taskPreflight = 'cd /home/xytron404/myos && python3 tools/run_logged.py -- python3 docs/wip/phase335/checkpoint20/storage/compact_preflight.py'
& wsl.exe -d Ubuntu -u root -- /bin/bash -lc $taskPreflight
if ($LASTEXITCODE -ne 0) { throw 'Preflight failed: no shutdown or compaction performed.' }
$taskStamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$taskLog = Join-Path $PSScriptRoot ('compact-' + $taskStamp + '.log')
$taskCommands = Join-Path $PSScriptRoot ('diskpart-' + $taskStamp + '.txt')
Start-Transcript -LiteralPath $taskLog | Out-Null
try {
    & wsl.exe --shutdown
    if ($LASTEXITCODE -ne 0) { throw 'WSL shutdown failed.' }
    $taskRunning = (& wsl.exe --list --running --quiet | Out-String).Replace([char]0,'')
    if ($taskRunning -match 'Ubuntu') { throw 'Ubuntu reopened; aborting compact.' }
    $taskDiskPartLines = @(
        ('select vdisk file="' + $taskExpectedPath + '"')
        'compact vdisk'
        'exit'
    )
    $taskDiskPartLines | Set-Content -LiteralPath $taskCommands -Encoding ascii
    & diskpart.exe /s $taskCommands
    if ($LASTEXITCODE -ne 0) { throw 'DiskPart failed; inspect transcript.' }
    $taskAfterBytes = (Get-Item -LiteralPath $taskExpectedPath).Length
    $taskAfterFree = ([System.IO.DriveInfo]::new('C:\')).TotalFreeSpace
    [pscustomobject]@{VhdxBeforeBytes=$taskBeforeBytes;VhdxAfterBytes=$taskAfterBytes;VhdxReductionBytes=($taskBeforeBytes-$taskAfterBytes);CFreeBeforeBytes=$taskBeforeFree;CFreeAfterBytes=$taskAfterFree} | ConvertTo-Json
    & wsl.exe -d Ubuntu -u root -- /bin/bash -lc ($taskPreflight + ' verify')
    if ($LASTEXITCODE -ne 0) { throw 'Post-compact normal hash verification failed.' }
    Write-Output 'Review actual byte measurements and DiskPart result above. Ubuntu reopened; normal hashes verified.'
} finally {
    if (Test-Path -LiteralPath $taskCommands) { Remove-Item -LiteralPath $taskCommands }
    Stop-Transcript | Out-Null
}
