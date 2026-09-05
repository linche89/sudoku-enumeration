param(
    [Parameter(Mandatory=$true)] [string]$FullGateEvidence,
    [Parameter(Mandatory=$true)] [string]$ReverseGateEvidence,
    [Parameter(Mandatory=$true)] [string]$Layer4,
    [Parameter(Mandatory=$true)] [string]$Layer4Backup,
    [Parameter(Mandatory=$true)] [ValidatePattern('^[0-9A-Fa-f]{64}$')] [string]$Layer4Sha256,
    [ValidateRange(1,480)] [int]$MaxMinutes=10,
    [ValidateRange(1,479)] [int]$WorkMinutes=8,
    [ValidateRange(1,24)] [int]$Threads=24,
    [ValidateRange(1,96452976)] [long]$Limit=10000,
    [ValidateRange(1,1000000)] [long]$Chunk=10000,
    [ValidateRange(45,55)] [int]$LimitGiB=55,
    [string]$NamespacePath="data/checkpoints/c6_reverse_f5_20260905",
    [string]$BackupRoot="D:/sudoku_FJ_checkpoint_backups/c6_reverse_f5_20260905",
    [string]$SupportBackup="D:/sudoku_FJ_checkpoint_backups/c6_layer5_support_20260905/ck.L5.rehearsal.snap",
    [string]$ExportComplete=""
)

# Bounded reverse-F5 computation from a VERIFIED CLOSED F4 export. This is
# not the final outer contraction. Input SHA checks and all backups are
# outside MaxMinutes, which bounds only the computing child.
$ErrorActionPreference="Stop"
$root=Split-Path -Parent $PSScriptRoot
Set-Location $root
if ($WorkMinutes -ge $MaxMinutes -or $Limit -lt $Chunk) {throw "Inconsistent whole-chunk/time bounds"}
if (![IO.Path]::IsPathRooted($BackupRoot)) {throw "BackupRoot must be absolute"}
$Layer4Sha256=$Layer4Sha256.ToUpperInvariant()
$l4=(Resolve-Path -LiteralPath $Layer4).Path
$l4copy=(Resolve-Path -LiteralPath $Layer4Backup).Path
$support=(Resolve-Path -LiteralPath "data/logs/layer-dp-c6-e2e-rehearsal-20260802-1pct/s2-4to5/ck.L5.snap").Path
$supportCopy=(Resolve-Path -LiteralPath $SupportBackup).Path
$supportSha="A5FDDB22F8C79FD4DDC0015795558FBF1647F0DB556248F5C9551F2DB24891DF"
$exe=(Resolve-Path -LiteralPath "build/layer_reverse_f5.exe").Path
$namespace=if ([IO.Path]::IsPathRooted($NamespacePath)) {
    [IO.Path]::GetFullPath($NamespacePath)
} else {[IO.Path]::GetFullPath((Join-Path $root $NamespacePath))}
$backupPath=[IO.Path]::GetFullPath($BackupRoot)
if ($ExportComplete) {
    $ExportComplete=if ([IO.Path]::IsPathRooted($ExportComplete)) {
        [IO.Path]::GetFullPath($ExportComplete)
    } else {[IO.Path]::GetFullPath((Join-Path $root $ExportComplete))}
    if ([IO.Path]::GetPathRoot($ExportComplete) -eq [IO.Path]::GetPathRoot($backupPath)) {
        throw "Completed L5 export and its physical backup must be on different volumes"
    }
}
if ($namespace.TrimEnd('\','/') -eq $root.TrimEnd('\','/') -or
    [IO.Path]::GetPathRoot($namespace) -eq [IO.Path]::GetPathRoot($backupPath)) {
    throw "Dedicated namespace and separate backup volume are required"
}

function Assert-Gate([string]$Path,[string]$Marker) {
    $item=Get-Item -LiteralPath $Path
    if ($item.LastWriteTimeUtc -lt [DateTime]::UtcNow.AddHours(-24) -or
        (Get-Content -LiteralPath $item.FullName -Raw) -notmatch [regex]::Escape($Marker)) {
        throw "Missing or stale exact gate: $Path"
    }
    return $item
}
$null=Assert-Gate $FullGateEvidence "ALL REPOSITORY CHECKS PASSED"
$reverseGate=Assert-Gate $ReverseGateEvidence "PRODUCTION REVERSE F5 CHECKS PASSED"
$built=(Get-Item -LiteralPath $exe).LastWriteTimeUtc
if ($built -gt $reverseGate.LastWriteTimeUtc) {throw "Reverse executable rebuilt after its successful gate"}
foreach ($path in @("experiments/proto/layer_reverse_f5.cpp","experiments/proto/layer_reverse_f5_core.h",
    "experiments/proto/layer_reverse_f5_chunks.h","experiments/proto/layer_shared_catalog.h",
    "experiments/proto/layer_shared_chunks.h","experiments/proto/layer_native_gather_bench.cpp",
    "experiments/proto/layer_dp_gate.cpp")) {
    if ((Get-Item -LiteralPath $path).LastWriteTimeUtc -gt $built) {throw "Source changed after build: $path"}
}
function Assert-InputCopies([string]$Source,[string]$Copy,[string]$Sha) {
    if ([IO.Path]::GetPathRoot($Source) -eq [IO.Path]::GetPathRoot($Copy)) {throw "Input needs a separate-volume physical copy"}
    if ((Get-Item -LiteralPath $Source).Length -ne (Get-Item -LiteralPath $Copy).Length -or
        (Get-FileHash -LiteralPath $Source -Algorithm SHA256).Hash -ne $Sha -or
        (Get-FileHash -LiteralPath $Copy -Algorithm SHA256).Hash -ne $Sha) {throw "Input/source-backup SHA mismatch"}
}
Assert-InputCopies $l4 $l4copy $Layer4Sha256
Assert-InputCopies $support $supportCopy $supportSha

$stamp=(Get-Date -Format "yyyyMMdd-HHmmss")+"-"+[guid]::NewGuid().ToString("N")
$logDir=Join-Path $root "data/logs/reverse-f5-window-$stamp"
New-Item -ItemType Directory -Path $logDir | Out-Null
New-Item -ItemType Directory -Force -Path $backupPath | Out-Null
function Backup-ClosedChunks([string]$Phase) {
    if (!(Test-Path -LiteralPath $namespace)) {return}
    $files=@(Get-ChildItem -LiteralPath $namespace -File | Where-Object {
        $_.Name -eq "manifest.bin" -or $_.Name -match '^f5-chunk-[0-9a-f]{16}\.bin$'
    } | Sort-Object Name)
    if (!$files.Count) {return}
    $destination=Join-Path $backupPath "$stamp-$Phase"
    New-Item -ItemType Directory -Path $destination | Out-Null
    $rows=foreach ($item in $files) {
        $target=Join-Path $destination $item.Name
        if (Test-Path -LiteralPath $target) {throw "Refusing backup overwrite"}
        $before=(Get-FileHash -LiteralPath $item.FullName -Algorithm SHA256).Hash
        Copy-Item -LiteralPath $item.FullName -Destination $target
        $copied=(Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash
        if ($copied -ne $before -or (Get-FileHash -LiteralPath $item.FullName -Algorithm SHA256).Hash -ne $before) {
            throw "Closed F5 backup hash mismatch"
        }
        [pscustomobject]@{Name=$item.Name;Bytes=$item.Length;SHA256=$copied}
    }
    $rows | Export-Csv -LiteralPath (Join-Path $destination "receipt.csv") -NoTypeInformation -Encoding UTF8
    Write-Host "PHYSICAL_BACKUP phase=$Phase files=$($files.Count) directory=$destination"
}
function Backup-CompletedExport {
    if (!$ExportComplete -or !(Test-Path -LiteralPath $ExportComplete)) {return}
    $source=(Resolve-Path -LiteralPath $ExportComplete).Path
    $target=Join-Path $backupPath "$stamp-closed.L5.snap"
    if ($source -eq $target -or [IO.Path]::GetPathRoot($source) -eq [IO.Path]::GetPathRoot($target)) {
        throw "Completed export requires an independent separate-volume copy"
    }
    $terminal=Get-Content -LiteralPath (Join-Path $logDir "stdout.log") -Raw
    $exports=[regex]::Matches($terminal,'(?m)^EXPORTED_CLOSED_NATIVE_L5 .+ sha256=([A-F0-9]{64}) native_readonly_roundtrip=yes')
    if ($terminal -notmatch '(?m)^SUMMARY status=CLOSED_F5_CATALOGUE ' -or $exports.Count -ne 1) {
        throw "An existing file is not sufficient evidence of a closed L5 export"
    }
    $sha=(Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash
    if ($sha -ne $exports[0].Groups[1].Value) {throw "Closed L5 export SHA differs from native readback"}
    if (Test-Path -LiteralPath $target) {throw "Refusing completed-export backup overwrite"}
    Copy-Item -LiteralPath $source -Destination $target
    if ((Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash -ne $sha) {throw "Closed L5 export backup hash mismatch"}
    if ((Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash -ne $sha) {throw "Closed L5 export changed during backup"}
    Write-Host "CLOSED_L5_PHYSICAL_BACKUP path=$target sha256=$sha"
}
Backup-ClosedChunks "before"
$argsForExe=@("6","l4=$l4","l4sha256=$Layer4Sha256","l5support=$support","output=$namespace",
    "chunk=$Chunk","limit=$Limit","threads=$Threads","workseconds=$($WorkMinutes*60)",
    "maxseconds=$($MaxMinutes*60-5)","maxrssgib=$LimitGiB","checkpointreadonly")
if ($ExportComplete) {$argsForExe += "export=$ExportComplete"}
$quotedArgs=@($argsForExe | ForEach-Object {if ($_ -match '\s') {'"'+$_.Replace('"','\"')+'"'} else {$_}})
Write-Host "REVERSE_WINDOW logs=$logDir namespace=$namespace"
$code=1
try {
    & "$PSScriptRoot/watch_rss.ps1" -Exe $exe -Arguments $quotedArgs -LimitGB $LimitGiB `
        -MaxMinutes $MaxMinutes -IntervalSeconds 2 -LogPath (Join-Path $logDir "rss.csv") `
        -StdoutPath (Join-Path $logDir "stdout.log") -StderrPath (Join-Path $logDir "stderr.log")
    $code=$LASTEXITCODE
} finally {
    Backup-ClosedChunks "after"
    if ($code -eq 0) {Backup-CompletedExport}
}
Write-Host "REVERSE_WINDOW_END exit=$code logs=$logDir N6=NOT_COMPUTED"
if ($code -ne 0) {exit $code}
