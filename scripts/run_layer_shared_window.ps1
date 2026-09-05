param(
    [Parameter(Mandatory=$true)] [string]$FullGateEvidence,
    [Parameter(Mandatory=$true)] [string]$SharedGateEvidence,
    [Parameter(Mandatory=$true)] [string]$SourceBackup,
    [ValidateRange(1, 480)] [int]$MaxMinutes = 10,
    [ValidateRange(1, 479)] [int]$WorkMinutes = 8,
    [ValidateRange(1, 24)] [int]$Threads = 24,
    [ValidateRange(1, 903398621)] [long]$Limit = 500000,
    [ValidateRange(1, 1000000)] [long]$Chunk = 25000,
    [ValidateRange(42, 55)] [int]$LimitGiB = 55,
    [string]$NamespacePath = "data/checkpoints/c6_shared_f4_20260905",
    [string]$BackupRoot = "D:/sudoku_FJ_checkpoint_backups/c6_shared_f4_20260905"
)

# A bounded production WINDOW, not the complete N(6) computation. New chunk
# files contain only closed graph values and exact aliases. Original inputs
# remain read-only. No path is deleted, moved, or overwritten by this script.
# MaxMinutes bounds the computing child, not pre/post backup hashing/copying.
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root
if (![IO.Path]::IsPathRooted($BackupRoot)) { throw "BackupRoot must be an explicit absolute path" }
if ($WorkMinutes -ge $MaxMinutes) { throw "WorkMinutes must be below MaxMinutes" }
if ($Limit -lt $Chunk) { throw "Limit must permit at least one complete chunk" }

$source = (Resolve-Path -LiteralPath "data/checkpoints/layer_dp_c6_s1_prod_20260802.a").Path
$expectedSource = "ECF0837315B0FDF8AE21C394FDA6676490E43B1828A0825529A344EC17E4E844"
$exe = (Resolve-Path -LiteralPath "build/layer_shared_f4.exe").Path
$namespace = if ([IO.Path]::IsPathRooted($NamespacePath)) {
    [IO.Path]::GetFullPath($NamespacePath)
} else {
    [IO.Path]::GetFullPath((Join-Path $root $NamespacePath))
}
$backupRootPath = [IO.Path]::GetFullPath($BackupRoot)
if ($namespace.TrimEnd('\', '/') -eq $root.TrimEnd('\', '/')) { throw "Dedicated namespace required" }
if ($namespace -eq $source) { throw "Namespace cannot be a production input" }
if ([IO.Path]::GetPathRoot($namespace) -eq [IO.Path]::GetPathRoot($backupRootPath)) {
    throw "Checkpoint backup must be on the configured separate volume"
}

function Assert-RecentGate([string]$Path, [string]$Marker) {
    $item = Get-Item -LiteralPath $Path
    if ($item.LastWriteTimeUtc -lt [DateTime]::UtcNow.AddHours(-24)) { throw "Gate evidence is older than24hours: $Path" }
    if ((Get-Content -LiteralPath $item.FullName -Raw) -notmatch [regex]::Escape($Marker)) {
        throw "Missing successful gate marker: $Path"
    }
    return $item
}
$fullGate = Assert-RecentGate $FullGateEvidence "ALL REPOSITORY CHECKS PASSED"
$sharedGate = Assert-RecentGate $SharedGateEvidence "SHARED LAYER CHECKS PASSED"
if ((Get-Item -LiteralPath $exe).LastWriteTimeUtc -gt $sharedGate.LastWriteTimeUtc) {
    throw "Shared executable was rebuilt after its supplied successful gate"
}
foreach ($path in @("experiments/proto/layer_shared_f4.cpp", "experiments/proto/layer_shared_chunks.h",
                   "experiments/proto/layer_shared_catalog.h", "experiments/proto/layer_shared_f4_core.h",
                   "experiments/proto/layer_shared_f4_bridge.cpp", "experiments/proto/layer_shared_f4_bridge.h",
                   "experiments/proto/layer_shared_f4_incremental_bridge.cpp",
                   "experiments/proto/layer_cpu_f4_incremental_core.h", "experiments/proto/layer_gpu_f4_nodp_core.h",
                   "experiments/proto/layer_dp_gate.cpp", "experiments/proto/layer_pairing_fiber_bench.cpp",
                   "src/factorization_orbit.cpp", "src/future_twin.hpp")) {
    if ((Get-Item -LiteralPath $path).LastWriteTimeUtc -gt (Get-Item -LiteralPath $exe).LastWriteTimeUtc) {
        throw "Source changed after the verified executable build: $path"
    }
}
$backupSource = (Resolve-Path -LiteralPath $SourceBackup).Path
if ([IO.Path]::GetPathRoot($source) -eq [IO.Path]::GetPathRoot($backupSource)) {
    throw "The original source also requires a separate-volume physical backup"
}
if ((Get-Item -LiteralPath $backupSource).Length -ne 32522350448 -or
    (Get-FileHash -LiteralPath $backupSource -Algorithm SHA256).Hash -ne $expectedSource) {
    throw "Original source backup is not the pinned generation37 image"
}
if ((Get-Item -LiteralPath $source).Length -ne 32522350448) { throw "Original input size changed" }

$stamp = (Get-Date -Format "yyyyMMdd-HHmmss") + "-" + [guid]::NewGuid().ToString("N")
$logDir = Join-Path $root "data/logs/shared-f4-window-$stamp"
New-Item -ItemType Directory -Path $logDir | Out-Null
New-Item -ItemType Directory -Force -Path $backupRootPath | Out-Null

function Backup-ClosedNamespace([string]$Phase) {
    if (!(Test-Path -LiteralPath $namespace)) { return }
    $files = @(Get-ChildItem -LiteralPath $namespace -File | Where-Object {
        $_.Name -eq "manifest.bin" -or $_.Name -match '^chunk-[0-9a-f]{16}\.bin$'
    } | Sort-Object Name)
    if (!$files.Count) { return }
    $destination = Join-Path $backupRootPath "$stamp-$Phase"
    New-Item -ItemType Directory -Path $destination | Out-Null
    $receipt = foreach ($item in $files) {
        $target = Join-Path $destination $item.Name
        if (Test-Path -LiteralPath $target) { throw "Refusing backup overwrite" }
        $before = (Get-FileHash -LiteralPath $item.FullName -Algorithm SHA256).Hash
        Copy-Item -LiteralPath $item.FullName -Destination $target
        $copied = (Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash
        $after = (Get-FileHash -LiteralPath $item.FullName -Algorithm SHA256).Hash
        if ($before -ne $copied -or $before -ne $after) { throw "Checkpoint backup SHA mismatch: $($item.Name)" }
        [pscustomobject]@{ Name=$item.Name; Bytes=$item.Length; SHA256=$copied }
    }
    $receipt | Export-Csv -LiteralPath (Join-Path $destination "receipt.csv") -NoTypeInformation -Encoding UTF8
    Write-Host "PHYSICAL_BACKUP phase=$Phase files=$($files.Count) directory=$destination"
}

Backup-ClosedNamespace "before"
Write-Host "WINDOW logs=$logDir namespace=$namespace"
Write-Host "Original source backup SHA verified; original source remains checkpointreadonly."
$argsForExe = @("6", "catalog=$source", "output=$namespace", "chunk=$Chunk", "limit=$Limit",
    "threads=$Threads", "workseconds=$($WorkMinutes*60)", "maxseconds=$($MaxMinutes*60-5)",
    "maxrssgib=$LimitGiB", "checkpointreadonly", "repair-l4-support")
# Start-Process joins argument strings; quote whitespace-containing path args.
$quotedArgs = @($argsForExe | ForEach-Object {
    if ($_ -match '\s') { '"' + $_.Replace('"', '\"') + '"' } else { $_ }
})
$code = 1
try {
    & "$PSScriptRoot/watch_rss.ps1" -Exe $exe -Arguments $quotedArgs -LimitGB $LimitGiB `
        -MaxMinutes $MaxMinutes -IntervalSeconds 2 -LogPath (Join-Path $logDir "rss.csv") `
        -StdoutPath (Join-Path $logDir "stdout.log") -StderrPath (Join-Path $logDir "stderr.log")
    $code = $LASTEXITCODE
} finally {
    Backup-ClosedNamespace "after"
}
Write-Host "WINDOW_END exit=$code logs=$logDir"
Write-Host "Only committed chunks are recoverable results; N(6) remains uncomputed."
if ($code -ne 0) { exit $code }
