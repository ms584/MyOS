# Read hardware facts only; run on the intended physical Phase 24 target.
# No firmware changes, physical writes, serial numbers, MACs or disk selection.
[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
function Read-Facts([scriptblock]$Query) {
    try { & $Query } catch { [pscustomobject]@{ unknown = $_.Exception.Message } }
}
$report = [ordered]@{
    schema = 1
    captured_at = (Get-Date).ToString('o')
    scope = 'Inventory only; this does not prove MyOS compatibility or GOP modes'
    computer = Read-Facts { Get-CimInstance Win32_ComputerSystem |
        Select-Object Manufacturer,Model,TotalPhysicalMemory }
    board = Read-Facts { Get-CimInstance Win32_BaseBoard |
        Select-Object Manufacturer,Product,Version }
    cpu = @(Read-Facts { Get-CimInstance Win32_Processor |
        Select-Object Name,AddressWidth })
    nx = Read-Facts { Get-CimInstance Win32_OperatingSystem |
        Select-Object DataExecutionPrevention_Available }
    bios = Read-Facts { Get-CimInstance Win32_BIOS |
        Select-Object SMBIOSBIOSVersion,ReleaseDate }
    firmware = Read-Facts { Get-ComputerInfo -Property BiosFirmwareType |
        Select-Object BiosFirmwareType }
    secure_boot = Read-Facts {
        Get-ItemProperty -LiteralPath 'HKLM:\SYSTEM\CurrentControlSet\Control\SecureBoot\State' |
        Select-Object UEFISecureBootEnabled
    }
    disks = @(Read-Facts { Get-Disk |
        Select-Object FriendlyName,BusType,Size,PartitionStyle })
    devices = @(Read-Facts { Get-CimInstance Win32_PnPEntity |
        Where-Object { $_.ConfigManagerErrorCode -eq 0 -and
            ($_.PNPClass -in @('Display','Net','MEDIA','Keyboard','Mouse','USB',
                              'SCSIAdapter','HIDClass') -or
             $_.Name -match 'I2C|GPIO|PS/2') -and
            $_.Name -notmatch 'WAN Miniport|Virtual|Hyper-V|Debug Network' } |
        Select-Object Name,PNPClass,HardwareID })
}
$report | ConvertTo-Json -Depth 8
