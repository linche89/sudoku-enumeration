param(
    [Parameter(Mandatory=$true)] [string]$FullGateEvidence,
    [Parameter(Mandatory=$true)] [string]$SharedGateEvidence,
    [Parameter(Mandatory=$true)] [string]$DirectGateEvidence,
    [Parameter(Mandatory=$true)] [string]$SourceBackup,
    [Parameter(Mandatory=$true)] [string]$NamespacePath,
    [Parameter(Mandatory=$true)] [string]$NamespaceBackup,
    [Parameter(Mandatory=$true)] [ValidatePattern('^[0-9A-Fa-f]{64}$')] [string]$ManifestSha256,
    [Parameter(Mandatory=$true)] [string]$OutputPath,
    [Parameter(Mandatory=$true)] [string]$OutputBackup,
    [Parameter(Mandatory=$true)] [ValidateRange(1,480)] [int]$MaxMinutes,
    [Parameter(Mandatory=$true)] [ValidateRange(1,55)] [int]$LimitGiB,
    [ValidateRange(1,300)] [int]$AuditMaxSeconds = 300,
    [ValidateRange(1,24)] [int]$Threads = 4,
    [ValidateRange(1,1000000)] [long]$Chunk = 25000,
    [ValidateSet(5,6)] [int]$C = 6,
    [string]$C5Source = '',
    [string]$C5SourceSha256 = ''
)

# CLOSED-ONLY controller around the unchanged released producer. The native
# checkpointreadonly switch protects the original source, not chunk writes.
# This controller first requires complete headers, then retains deny-write /
# deny-delete handles on every expected input through export and readback.
# Thus a missing chunk cannot appear between preflight and producer resume.
# Any validation failure aborts; there is no compute-missing fallback.
# MaxMinutes bounds the producer child; AuditMaxSeconds bounds the streaming
# readback. Source/backup hashing and physical copying are additional overhead.
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root

function Full-Path([string]$Path) {
    if (![IO.Path]::IsPathRooted($Path)) { $Path = Join-Path $root $Path }
    return [IO.Path]::GetFullPath($Path)
}
function Assert-NoReparse([string]$Path) {
    $at = $Path
    while ($at) {
        if (Test-Path -LiteralPath $at) {
            if ((Get-Item -LiteralPath $at -Force).Attributes -band [IO.FileAttributes]::ReparsePoint) {
                throw "Reparse-point paths are not accepted: $at"
            }
        }
        $parent = [IO.Path]::GetDirectoryName($at.TrimEnd('\','/'))
        if ($parent -eq $at) { break }
        $at = $parent
    }
}
function Recent-Gate([string]$Path, [string]$Marker) {
    $item = Get-Item -LiteralPath (Full-Path $Path)
    if ($item.LastWriteTimeUtc -lt [DateTime]::UtcNow.AddHours(-24) -or
        (Get-Content -LiteralPath $item.FullName -Raw) -notmatch [regex]::Escape($Marker)) {
        throw "Recent successful gate evidence is required: $Path"
    }
    return $item
}
function Under([string]$Child, [string]$Parent) {
    return $Child.Equals($Parent, [StringComparison]::OrdinalIgnoreCase) -or
        $Child.StartsWith($Parent.TrimEnd('\','/')+'\', [StringComparison]::OrdinalIgnoreCase)
}
function Hash([string]$Path) {
    $inputStream = [IO.File]::OpenRead($Path)
    $algorithm = [Security.Cryptography.SHA256]::Create()
    try { return [BitConverter]::ToString($algorithm.ComputeHash($inputStream)).Replace('-','') }
    finally { $algorithm.Dispose(); $inputStream.Dispose() }
}
function Quoted([string[]]$Items) {
    return @($Items | ForEach-Object {
        if ($_ -match '"|[\r\n]') { throw 'Quote/newline in process argument is forbidden' }
        if ($_ -match '\s') { '"' + $_ + '"' } else { $_ }
    })
}

$fullGate = Recent-Gate $FullGateEvidence 'ALL REPOSITORY CHECKS PASSED'
$sharedGate = Recent-Gate $SharedGateEvidence 'SHARED LAYER CHECKS PASSED'
$directGate = Recent-Gate $DirectGateEvidence 'DIRECT LAYER CHECKS PASSED'
$exe = Full-Path 'build/layer_shared_f4.exe'
$probe = Full-Path 'build/layer_support_probe.exe'
foreach ($pair in @(@($exe,$sharedGate),@($probe,$directGate))) {
    if ((Get-Item -LiteralPath $pair[0]).LastWriteTimeUtc -gt $pair[1].LastWriteTimeUtc) {
        throw 'Executable was rebuilt after the supplied gate'
    }
}
foreach ($path in @('experiments/proto/layer_shared_f4.cpp','experiments/proto/layer_shared_chunks.h',
    'experiments/proto/layer_shared_catalog.h','experiments/proto/layer_shared_f4_core.h',
    'experiments/proto/layer_shared_f4_bridge.cpp','experiments/proto/layer_shared_f4_bridge.h',
    'experiments/proto/layer_dp_gate.cpp','experiments/proto/layer_pairing_fiber_bench.cpp',
    'src/factorization_orbit.cpp','src/future_twin.hpp')) {
    if ((Get-Item -LiteralPath (Full-Path $path)).LastWriteTimeUtc -gt (Get-Item -LiteralPath $exe).LastWriteTimeUtc) {
        throw "Source changed after verified producer build: $path"
    }
}
if ((Get-Item -LiteralPath (Full-Path 'experiments/proto/layer_support_probe.cpp')).LastWriteTimeUtc -gt
    (Get-Item -LiteralPath $probe).LastWriteTimeUtc) { throw 'Support reader source is newer than its build' }

if ($C -eq 6) {
    if ($C5Source -or $C5SourceSha256 -or $LimitGiB -lt 42) { throw 'C6 uses only the pinned original input and 42..55 GiB bound' }
    $source = Full-Path 'data/checkpoints/layer_dp_c6_s1_prod_20260802.a'
    $sourcePin = 'ECF0837315B0FDF8AE21C394FDA6676490E43B1828A0825529A344EC17E4E844'
} else {
    if (!$C5Source -or $C5SourceSha256 -notmatch '^[0-9A-Fa-f]{64}$' -or $LimitGiB -gt 2) {
        throw 'C5 qualification needs an explicit small source/SHA and at most 2 GiB'
    }
    $source = Full-Path $C5Source
    $sourcePin = $C5SourceSha256.ToUpperInvariant()
}
$sourceCopy = Full-Path $SourceBackup
$namespace = Full-Path $NamespacePath
$namespaceCopy = Full-Path $NamespaceBackup
$output = Full-Path $OutputPath
$outputCopy = Full-Path $OutputBackup
$receiptPath = $output + '.receipt.json'
foreach ($path in @($source,$sourceCopy,$namespace,$namespaceCopy,$output,$outputCopy,$receiptPath)) {
    Assert-NoReparse $path
}
foreach ($pair in @(@($source,$sourceCopy),@($namespace,$namespaceCopy),@($output,$outputCopy))) {
    if ([IO.Path]::GetPathRoot($pair[0]) -eq [IO.Path]::GetPathRoot($pair[1])) {
        throw 'Each physical backup must be on a separate volume'
    }
}
foreach ($dir in @($namespace,$namespaceCopy)) {
    if (!(Test-Path -LiteralPath $dir -PathType Container) -or
        $dir.TrimEnd('\','/') -eq $root.TrimEnd('\','/') -or $dir -eq [IO.Path]::GetPathRoot($dir)) {
        throw 'Existing dedicated committed namespace directories are required'
    }
    foreach ($file in @($source,$sourceCopy,$output,$outputCopy,$receiptPath)) {
        if (Under $file $dir) { throw 'Inputs and native outputs must be outside both chunk namespaces' }
    }
}
foreach ($path in @($output,$outputCopy,$receiptPath)) {
    if (Test-Path -LiteralPath $path) { throw "Refusing to overwrite export/receipt: $path" }
    if (!(Test-Path -LiteralPath ([IO.Path]::GetDirectoryName($path)) -PathType Container)) {
        throw 'Explicit output parent directories must already exist'
    }
}
if ($output -eq $source -or $output -eq $sourceCopy -or $outputCopy -eq $source -or $outputCopy -eq $sourceCopy) {
    throw 'Export cannot replace either protected source'
}
if ($C -eq 6 -and ((Get-Item -LiteralPath $source).Length -ne 32522350448 -or
    (Get-Item -LiteralPath $sourceCopy).Length -ne 32522350448)) { throw 'C6 source length changed' }

# FileStream bufferSize=1 avoids 4-KiB buffers for about 36,000 chunks.
# Directory handles also deny renaming ancestor directories during validation.
Add-Type @'
using System;
using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;
public static class SharedExportReadLocks {
  [StructLayout(LayoutKind.Sequential)]
  struct Info {
    public uint Attributes, CreationLow, CreationHigh, AccessLow, AccessHigh,
        WriteLow, WriteHigh, VolumeSerial, SizeHigh, SizeLow, Links, IndexHigh, IndexLow;
  }
  [DllImport("kernel32.dll", CharSet=CharSet.Unicode, SetLastError=true)]
  static extern SafeFileHandle CreateFile(string path, uint access, uint share,
      IntPtr security, uint creation, uint flags, IntPtr template);
  [DllImport("kernel32.dll", SetLastError=true)]
  static extern bool GetFileInformationByHandle(SafeFileHandle h, out Info info);
  static SafeFileHandle Open(string path, uint share, uint flags) {
    var h=CreateFile(path, 0x80000000, share, IntPtr.Zero, 3,
        flags | 0x00200000, IntPtr.Zero); // OPEN_REPARSE_POINT: inspect the name itself
    if(h.IsInvalid) throw new System.ComponentModel.Win32Exception(Marshal.GetLastWin32Error());
    Info info;
    if(!GetFileInformationByHandle(h,out info)) {
      int error=Marshal.GetLastWin32Error(); h.Dispose();
      throw new System.ComponentModel.Win32Exception(error);
    }
    if((info.Attributes & 0x400)!=0) { h.Dispose(); throw new System.IO.IOException("Reparse handle refused"); }
    return h;
  }
  public static SafeFileHandle Directory(string path) {
    // Permit directory-content writes (the new export's atomic rename),
    // while denying DELETE access that would rename this directory itself.
    return Open(path,3,0x02000000);
  }
  public static System.IO.FileStream ReadFile(string path) {
    var h=Open(path,1,0);
    try { return new System.IO.FileStream(h,System.IO.FileAccess.Read,1,false); }
    catch { h.Dispose(); throw; }
  }
}
'@
$handles = [Collections.Generic.List[IDisposable]]::new()
$heldDirectories = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
function Hold-Parents([string]$Path) {
    $dir = [IO.Path]::GetDirectoryName($Path)
    $chain = [Collections.Generic.List[string]]::new()
    while ($dir -and $dir -ne [IO.Path]::GetPathRoot($dir)) {
        $chain.Add($dir)
        $dir = [IO.Path]::GetDirectoryName($dir)
    }
    # Pin and inspect each ancestor before resolving its child name.
    for ($j=$chain.Count-1;$j -ge 0;$j--) {
        if ($heldDirectories.Add($chain[$j])) { $handles.Add([SharedExportReadLocks]::Directory($chain[$j])) }
    }
}
function Hold-File([string]$Path) {
    Hold-Parents $Path
    Assert-NoReparse $Path
    $handles.Add([SharedExportReadLocks]::ReadFile($Path))
}
function Header-Check([string]$Dir, [string]$Exported = '') {
    $command = @((Full-Path 'scripts/layer_shared_export_preflight.py'),'--c',"$C",'--source',$source,
        '--source-sha256',$sourcePin,'--chunk',"$Chunk")
    if ($Dir) { $command += @('--namespace',$Dir) }
    if ($Exported) { $command += @('--exported',$Exported) }
    $raw = & python @command
    if ($LASTEXITCODE -ne 0) { throw 'Closed-export header preflight failed; producer not accepted' }
    return ($raw | ConvertFrom-Json)
}

$stamp = (Get-Date -Format 'yyyyMMdd-HHmmss')+'-'+[guid]::NewGuid().ToString('N')
$logDir = Join-Path $root "data/logs/shared-f4-export-$stamp"
New-Item -ItemType Directory -Path $logDir | Out-Null
$started = [Diagnostics.Stopwatch]::StartNew()
$exePin = Hash $exe
$probePin = Hash $probe
$records = [Collections.Generic.List[object]]::new()
try {
    Hold-File $exe
    Hold-File $probe
    Hold-File $source
    Hold-File $sourceCopy
    Hold-Parents $output
    Hold-Parents $outputCopy
    if ((Hash $source) -ne $sourcePin -or (Hash $sourceCopy) -ne $sourcePin) { throw 'Original source/local physical-copy SHA mismatch' }
    $manifest = Join-Path $namespace 'manifest.bin'
    $manifestCopy = Join-Path $namespaceCopy 'manifest.bin'
    Hold-File $manifest
    Hold-File $manifestCopy
    if ((Hash $manifest) -ne $ManifestSha256 -or (Hash $manifestCopy) -ne $ManifestSha256) {
        throw 'Pinned manifest/local physical-copy SHA mismatch'
    }
    $geometry = Header-Check $namespace
    # Every expected chunk is opened and retained BEFORE the definitive check.
    for ([long]$begin=0; $begin -lt $geometry.entries; $begin+=$Chunk) {
        $name = 'chunk-'+$begin.ToString('x16')+'.bin'
        Hold-File (Join-Path $namespace $name)
        Hold-File (Join-Path $namespaceCopy $name)
    }
    $geometry = Header-Check $namespace
    $backupGeometry = Header-Check $namespaceCopy
    if ($geometry.entries -ne $backupGeometry.entries -or $geometry.chunks -ne $backupGeometry.chunks) {
        throw 'Physical namespace backup is incomplete'
    }
    $exportBytes = [long]128 + [long]36 * [long]$geometry.entries
    foreach ($target in @($output,$outputCopy)) {
        $volume = [IO.DriveInfo]::new([IO.Path]::GetPathRoot($target))
        if ($volume.AvailableFreeSpace -lt $exportBytes + 1GB) { throw 'Insufficient export/backup disk headroom' }
    }
    foreach ($item in @(Get-ChildItem -LiteralPath $namespace -File | Where-Object {
        $_.Name -eq 'manifest.bin' -or $_.Name -match '^chunk-[0-9a-f]{16}\.bin$'
    } | Sort-Object Name)) {
        $copy = Join-Path $namespaceCopy $item.Name
        $hash = Hash $item.FullName
        if ((Get-Item -LiteralPath $copy).Length -ne $item.Length -or (Hash $copy) -ne $hash) {
            throw "Committed namespace physical backup differs: $($item.Name)"
        }
        $records.Add([pscustomobject]@{Name=$item.Name;Bytes=$item.Length;SHA256=$hash})
    }
    $records | Export-Csv -LiteralPath (Join-Path $logDir 'input-files.csv') -NoTypeInformation -Encoding UTF8
    Write-Host "CLOSED_EXPORT_PREFLIGHT entries=$($geometry.entries) live=$($geometry.live) chunks=$($geometry.chunks) held_handles=$($handles.Count) logs=$logDir"
    $arguments = @("$C","catalog=$source","output=$namespace","chunk=$Chunk","limit=$Chunk",
        "threads=$Threads",'workseconds=1',"maxseconds=$($MaxMinutes*60-5)","maxrssgib=$LimitGiB",
        'checkpointreadonly',"export=$output")
    if ($C -eq 6) { $arguments += 'repair-l4-support' }
    & "$PSScriptRoot/watch_rss.ps1" -Exe $exe -Arguments (Quoted $arguments) -LimitGB $LimitGiB `
        -MaxMinutes $MaxMinutes -IntervalSeconds 1 -LogPath (Join-Path $logDir 'producer-rss.csv') `
        -StdoutPath (Join-Path $logDir 'producer-stdout.log') -StderrPath (Join-Path $logDir 'producer-stderr.log')
    if ($LASTEXITCODE -ne 0) { throw 'Closed export producer failed; output is not accepted' }
    $text = Get-Content -LiteralPath (Join-Path $logDir 'producer-stdout.log') -Raw
    $summaries = @([regex]::Matches($text,'(?m)^SUMMARY .+$'))
    if ($summaries.Count -ne 1 -or $text -match '(?m)^CLOSED_CHUNK ' -or
        $text -notmatch 'EXPORTED_CLOSED_NATIVE_L4 ' -or
        $summaries[0].Value -notmatch 'status=CLOSED_F4_CATALOGUE\b' -or
        $summaries[0].Value -notmatch '\bnew_chunks=0\b' -or
        $summaries[0].Value -notmatch '\bnew_indices=0\b' -or
        $summaries[0].Value -notmatch ("\bclosed_prefix="+$geometry.entries+'\b') -or
        $summaries[0].Value -notmatch ("\btotal_entries="+$geometry.entries+'\b')) {
        throw 'Expected full alias closure and ZERO new computation; export is not accepted'
    }
    Hold-File $output
    $null = Header-Check '' $output
    $outputHash = Hash $output
    $auditArgs = @('--input',$output,'--expected-sha256',$outputHash,'--max-seconds',"$AuditMaxSeconds",
        '--max-rss-mib','2048','--checkpointreadonly')
    & "$PSScriptRoot/watch_rss.ps1" -Exe $probe -Arguments (Quoted $auditArgs) -LimitGB 2 `
        -MaxMinutes ([int][Math]::Ceiling($AuditMaxSeconds/60.0)) -IntervalSeconds 1 `
        -LogPath (Join-Path $logDir 'readback-rss.csv') -StdoutPath (Join-Path $logDir 'readback-stdout.log') `
        -StderrPath (Join-Path $logDir 'readback-stderr.log')
    if ($LASTEXITCODE -ne 0) { throw 'Independent native serialization readback failed' }
    $audit = Get-Content -LiteralPath (Join-Path $logDir 'readback-stdout.log') -Raw
    foreach ($required in @("C=$C layer=4 generation=0 entries=$($geometry.entries) holes=$($geometry.holes) real=$($geometry.live)",
        "stored_stabilizer_orbit_mass=$($geometry.mass)","sha256=$outputHash",
        'payload_hash_verified=yes header_hash_verified=yes readonly=yes','[OK]')) {
        if (!$audit.Contains($required)) { throw "Readback certificate lacks expected field: $required" }
    }
    # Copy(false) never replaces an existing file, including a creation race.
    [IO.File]::Copy($output,$outputCopy,$false)
    Hold-File $outputCopy
    if ((Hash $output) -ne $outputHash -or (Hash $outputCopy) -ne $outputHash) { throw 'New native physical backup SHA mismatch' }
    foreach ($row in $records) {
        if ((Hash (Join-Path $namespace $row.Name)) -ne $row.SHA256 -or
            (Hash (Join-Path $namespaceCopy $row.Name)) -ne $row.SHA256) { throw 'A committed input changed' }
    }
    if ((Hash $source) -ne $sourcePin -or (Hash $sourceCopy) -ne $sourcePin -or
        (Hash $exe) -ne $exePin -or (Hash $probe) -ne $probePin) { throw 'Protected source/executable changed' }
    $receipt = [ordered]@{schema='shared-f4-closed-export-v1';c=$C;layer=4;entries=$geometry.entries;
        live=$geometry.live;holes=$geometry.holes;orbit_mass=$geometry.mass;namespace=$namespace;
        namespace_backup=$namespaceCopy;manifest_sha256=$ManifestSha256.ToUpperInvariant();
        input_file_receipt=(Join-Path $logDir 'input-files.csv');source=$source;source_sha256=$sourcePin;
        source_backup=$sourceCopy;output=$output;backup=$outputCopy;sha256=$outputHash;
        bytes=(Get-Item -LiteralPath $output).Length;producer_sha256=$exePin;reader_sha256=$probePin;
        new_chunks=0;new_indices=0;alias_closure='producer_verified';
        serialization='independent_full_SHA_header_payload_count_mass_readback';
        independent_recalculation_of_all_C6_F4=$false;N6='NOT_COMPUTED';
        full_gate=$fullGate.FullName;shared_gate=$sharedGate.FullName;direct_gate=$directGate.FullName;
        wall_seconds=$started.Elapsed.TotalSeconds;logs=$logDir}
    $stream = [IO.FileStream]::new($receiptPath,[IO.FileMode]::CreateNew,[IO.FileAccess]::Write,[IO.FileShare]::None)
    try {
        $bytes = [Text.Encoding]::UTF8.GetBytes(($receipt | ConvertTo-Json -Depth 5))
        $stream.Write($bytes,0,$bytes.Length)
        $stream.Flush($true)
    } finally { $stream.Dispose() }
    Write-Host "CLOSED SHARED F4 EXPORT VERIFIED C=$C entries=$($geometry.entries) new_chunks=0 SHA256=$outputHash receipt=$receiptPath"
} finally {
    for ($i=$handles.Count-1;$i -ge 0;$i--) { $handles[$i].Dispose() }
}
