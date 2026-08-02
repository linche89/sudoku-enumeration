param(
    [Parameter(Mandatory = $true)]
    [switch]$AuthorizeFullC6,

    [int]$Threads = 24,
    [string]$Caps = "2000,14000000,1350000000,250000000,100000",
    [UInt64]$ChunkParents = 100000,
    [double]$CheckpointMinutes = 40.0,
    [ValidateRange(1.0, 24.0)]
    [double]$WindowHours = 8.0,
    [ValidateRange(1.0, 24.0)]
    [double]$HardMaxHours = 10.0,
    [int]$RssLimitGB = 85,
    [int]$MinimumAvailableGB = 8,
    [int]$StartAvailableGB = 80,
    [string]$Layer3 =
        "data/checkpoints/layer_dp_c6_layer3_20260731.snap",
    [string]$Layer3Backup =
        "D:\sudoku_FJ_checkpoint_backups\layer_dp_c6_layer3_20260731.snap",
    [string]$CheckpointBase =
        "data/checkpoints/layer_dp_c6_s1_prod_20260802",
    [string]$ExternalBackupDir =
        "D:\sudoku_FJ_checkpoint_backups\layer_dp_c6_s1_prod_20260802",
    [string]$LogRoot =
        "data/logs/layer_dp_c6_s1_prod_20260802",
    [switch]$ContinueExisting,
    [switch]$PrepareOnly
)

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
Set-Location $root

if (!$AuthorizeFullC6) {
    throw "-AuthorizeFullC6 is required for the production S1 controller"
}
if ($Threads -lt 2) { throw "Threads must be at least 2" }
if ($CheckpointMinutes -le 0 -or $ChunkParents -eq 0) {
    throw "checkpoint period and chunk size must be positive"
}
if ($HardMaxHours -le $WindowHours) {
    throw "HardMaxHours must exceed WindowHours so a checkpoint can close"
}
if ($RssLimitGB -lt 1 -or $MinimumAvailableGB -lt 1 -or
    $StartAvailableGB -lt 1) {
    throw "RAM guards must be positive"
}

function Get-Sha256 {
    param([Parameter(Mandatory = $true)][string]$Path)
    $stream = [IO.File]::OpenRead($Path)
    $sha = [Security.Cryptography.SHA256]::Create()
    try {
        return ([BitConverter]::ToString(
            $sha.ComputeHash($stream))).Replace("-", "")
    }
    finally {
        $sha.Dispose()
        $stream.Dispose()
    }
}

function Assert-PathUnder {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Parent,
        [Parameter(Mandatory = $true)][string]$Label
    )
    $full = [IO.Path]::GetFullPath($Path)
    $parentFull = [IO.Path]::GetFullPath($Parent).TrimEnd('\', '/') +
        [IO.Path]::DirectorySeparatorChar
    if (!$full.StartsWith($parentFull,
            [StringComparison]::OrdinalIgnoreCase)) {
        throw "$Label must stay under $parentFull"
    }
    return $full
}

function Get-AvailableRamGiB {
    $os = Get-CimInstance Win32_OperatingSystem
    return [Math]::Round(
        ([Int64]$os.FreePhysicalMemory * 1KB) / 1GB, 3)
}

function Wait-StartMemory {
    $deadline = (Get-Date).AddMinutes(10)
    while ($true) {
        $available = Get-AvailableRamGiB
        if ($available -ge $StartAvailableGB) {
            Write-Host "available RAM $available GiB [READY]"
            return
        }
        if ((Get-Date) -ge $deadline) {
            throw "available RAM $available GiB did not recover to " +
                  "$StartAvailableGB GiB within ten minutes"
        }
        Write-Host "waiting for RAM ($available/$StartAvailableGB GiB)"
        Start-Sleep -Seconds 10
    }
}

function Get-LatestDurableImage {
    $candidates = @(
        $checkpointBasePath + ".L4.snap",
        $checkpointBasePath + ".a",
        $checkpointBasePath + ".b"
    ) | Where-Object { Test-Path -LiteralPath $_ }
    if (!$candidates.Count) { return $null }
    return $candidates |
        Get-Item -Force |
        Sort-Object LastWriteTimeUtc -Descending |
        Select-Object -First 1
}

function Backup-DurableImage {
    param(
        [Parameter(Mandatory = $true)][IO.FileInfo]$Source,
        [Parameter(Mandatory = $true)][int]$SessionNumber,
        [Parameter(Mandatory = $true)][string]$Reason
    )
    $sourceHash = Get-Sha256 $Source.FullName
    $receipts = @(Get-ChildItem -LiteralPath $externalBackupPath `
        -Filter "*.sha256.txt" -File -ErrorAction SilentlyContinue)
    foreach ($receipt in $receipts) {
        $text = Get-Content -Raw -LiteralPath $receipt.FullName
        if ($text -match "(?m)^sha256=$sourceHash\r?$") {
            Write-Host "external backup already covers $($Source.Name): " +
                $receipt.FullName
            return [pscustomobject]@{
                Source = $Source.FullName
                Hash = $sourceHash
                Backup = (($text -split "`r?`n" | Where-Object {
                    $_ -like "backup_path=*"
                } | Select-Object -First 1) -replace '^backup_path=', '')
                Receipt = $receipt.FullName
            }
        }
    }

    $stamp = Get-Date -Format "yyyyMMdd-HHmmss"
    $destName = "session-{0:D3}-{1}-{2}" -f `
        $SessionNumber, $stamp, $Source.Name
    $dest = Join-Path $externalBackupPath $destName
    $temp = $dest + ".copying"
    if ((Test-Path -LiteralPath $dest) -or
        (Test-Path -LiteralPath $temp)) {
        throw "refusing to overwrite external backup target $dest"
    }

    Write-Host "copying durable image to external backup: $dest"
    Copy-Item -LiteralPath $Source.FullName -Destination $temp
    $destHash = Get-Sha256 $temp
    if ($destHash -ne $sourceHash) {
        throw "external backup SHA-256 mismatch for $temp"
    }
    Move-Item -LiteralPath $temp -Destination $dest
    $receiptPath = $dest + ".sha256.txt"
    @(
        "mode=C6_PRODUCTION_S1_PARTIAL",
        "reason=$Reason",
        "created=$((Get-Date).ToString('o'))",
        "source_path=$($Source.FullName)",
        "backup_path=$dest",
        "bytes=$($Source.Length)",
        "sha256=$sourceHash",
        "git_head=$gitHead"
    ) | Set-Content -LiteralPath $receiptPath -Encoding ascii
    return [pscustomobject]@{
        Source = $Source.FullName
        Hash = $sourceHash
        Backup = $dest
        Receipt = $receiptPath
    }
}

function Get-CheckpointMarkers {
    param([string]$Path)
    if (!(Test-Path -LiteralPath $Path)) { return @() }
    try {
        $content = Get-Content -Raw -LiteralPath $Path
        return @([regex]::Matches(
            $content,
            'checkpoint gen=(\d+) trans=3->4 chunk=(\d+)/(\d+) ' +
            'entries=(\d+) -> (.+?) \(([0-9.]+)s\)'))
    }
    catch {
        return @()
    }
}

function Invoke-LoggedProcess {
    param(
        [Parameter(Mandatory = $true)][string]$FilePath,
        [string[]]$Arguments = @(),
        [Parameter(Mandatory = $true)][string]$StdoutPath,
        [Parameter(Mandatory = $true)][string]$StderrPath
    )
    $child = Start-Process -FilePath $FilePath -ArgumentList $Arguments `
        -RedirectStandardOutput $StdoutPath `
        -RedirectStandardError $StderrPath -PassThru -WindowStyle Hidden
    $child.WaitForExit()
    $child.Refresh()
    if ($null -eq $child.ExitCode) { return 97 }
    return [int]$child.ExitCode
}

$checkpointRoot = Join-Path $root "data/checkpoints"
$logParent = Join-Path $root "data/logs"
$checkpointBasePath = Assert-PathUnder $CheckpointBase $checkpointRoot `
    "CheckpointBase"
$logRootPath = Assert-PathUnder $LogRoot $logParent "LogRoot"
$externalRoot = "D:\sudoku_FJ_checkpoint_backups"
$externalBackupPath = Assert-PathUnder $ExternalBackupDir $externalRoot `
    "ExternalBackupDir"

if ([IO.Path]::GetPathRoot($checkpointBasePath) -eq
    [IO.Path]::GetPathRoot($externalBackupPath)) {
    throw "external backup must be on a different volume"
}
if ([IO.Path]::GetFileName($checkpointBasePath) -notlike
    "layer_dp_c6_s1_prod_*") {
    throw "CheckpointBase must use a layer_dp_c6_s1_prod_* basename"
}

$dirty = @(& git status --porcelain)
if ($LASTEXITCODE -ne 0) { throw "git status failed" }
if ($dirty.Count) {
    throw "production S1 requires a clean tracked worktree"
}
$gitHead = (& git rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0) { throw "cannot resolve Git HEAD" }

if (Get-Process -Name "layer_dp_gate" -ErrorAction SilentlyContinue) {
    throw "another layer_dp_gate process is already running"
}

$layer3Path = (Resolve-Path -LiteralPath $Layer3).Path
$layer3BackupPath = (Resolve-Path -LiteralPath $Layer3Backup).Path
$expectedLayer3Hash =
    "1D882DB7B0B18490E981539F7C690FA90227ECEC243817F5B538BEC8DAC865B7"
$layer3Hash = Get-Sha256 $layer3Path
$layer3BackupHash = Get-Sha256 $layer3BackupPath
if ($layer3Hash -ne $expectedLayer3Hash -or
    $layer3BackupHash -ne $expectedLayer3Hash) {
    throw "active/external L3 snapshot hash does not match MANIFEST.md"
}

New-Item -ItemType Directory -Force -Path $logRootPath | Out-Null
New-Item -ItemType Directory -Force -Path $externalBackupPath | Out-Null

$sessionNumber = 1
while (Test-Path -LiteralPath (Join-Path $logRootPath (
        "session-{0:D3}" -f $sessionNumber))) {
    $sessionNumber++
}
$sessionDir = Join-Path $logRootPath ("session-{0:D3}" -f $sessionNumber)
New-Item -ItemType Directory -Path $sessionDir | Out-Null

$gateLog = Join-Path $sessionDir "verify_all.stdout.log"
$gateErrorLog = Join-Path $sessionDir "verify_all.stderr.log"
Write-Host "running the mandatory complete gate"
$gateArgs = @(
    "-NoProfile", "-ExecutionPolicy", "Bypass", "-File",
    (Join-Path $PSScriptRoot "verify_all.ps1")
)
$gateCode = Invoke-LoggedProcess "powershell.exe" $gateArgs $gateLog `
    $gateErrorLog
if ($gateCode -ne 0) {
    Get-Content -LiteralPath $gateLog -Tail 100
    Get-Content -LiteralPath $gateErrorLog -Tail 100
    throw "complete repository gate failed"
}

$buildLog = Join-Path $sessionDir "build.log"
$buildErrorLog = Join-Path $sessionDir "build.stderr.log"
$buildArgs = @(
    "-NoProfile", "-ExecutionPolicy", "Bypass", "-File",
    (Join-Path $PSScriptRoot "build_layer_dp.ps1")
)
$buildCode = Invoke-LoggedProcess "powershell.exe" $buildArgs $buildLog `
    $buildErrorLog
if ($buildCode -ne 0) {
    Get-Content -LiteralPath $buildLog -Tail 100
    Get-Content -LiteralPath $buildErrorLog -Tail 100
    throw "layer-DP build failed"
}
$exe = (Resolve-Path -LiteralPath "build/layer_dp_gate.exe").Path

$existing = @(
    $checkpointBasePath + ".a",
    $checkpointBasePath + ".b",
    $checkpointBasePath + ".L3.snap",
    $checkpointBasePath + ".L4.snap",
    $checkpointBasePath + ".tmp"
) | Where-Object { Test-Path -LiteralPath $_ }
if ($ContinueExisting) {
    if (!(Test-Path -LiteralPath ($checkpointBasePath + ".a")) -and
        !(Test-Path -LiteralPath ($checkpointBasePath + ".b")) -and
        !(Test-Path -LiteralPath ($checkpointBasePath + ".L4.snap"))) {
        throw "-ContinueExisting requested without an S1 checkpoint"
    }
}
elseif ($existing.Count) {
    throw "fresh production base already has files; use -ContinueExisting"
}
if (Test-Path -LiteralPath ($checkpointBasePath + ".L4.snap")) {
    throw "S1 is already closed; do not resume it as a partial window"
}

if ($ContinueExisting) {
    $beforeImage = Get-LatestDurableImage
    if ($null -eq $beforeImage) {
        throw "resume base has no durable image"
    }
    $null = Backup-DurableImage $beforeImage $sessionNumber `
        "pre-resume coverage"
}

Wait-StartMemory
$preflightLog = Join-Path $sessionDir "resource-preflight.log"
$preflightArgs = @(
    "6", "--threads", "$Threads", "--caps", $Caps,
    "--checkpoint", $checkpointBasePath, "$CheckpointMinutes",
    "--resource-preflight", "3"
)
$preflightErrorLog = Join-Path $sessionDir "resource-preflight.stderr.log"
$preflightCode = Invoke-LoggedProcess $exe $preflightArgs $preflightLog `
    $preflightErrorLog
if ($preflightCode -ne 0) {
    Get-Content -LiteralPath $preflightLog
    Get-Content -LiteralPath $preflightErrorLog
    throw "C=6 S1 resource preflight failed"
}

if ($PrepareOnly) {
    Write-Host "PRODUCTION S1 PREPARATION PASSED (no checkpoint written)"
    Write-Host "session directory: $sessionDir"
    exit 0
}

$stdout = Join-Path $sessionDir "stdout.log"
$stderr = Join-Path $sessionDir "stderr.log"
$rssLog = Join-Path $sessionDir "rss.csv"
"timestamp,pid,rss_bytes,rss_gib,available_gib" |
    Set-Content -LiteralPath $rssLog -Encoding ascii

$common = @(
    "6", "--threads", "$Threads", "--caps", $Caps,
    "--ckpt-chunk", "$ChunkParents",
    "--checkpoint", $checkpointBasePath, "$CheckpointMinutes",
    "--stop-after", "4", "--ack-full-c6"
)
$engineArgs = if ($ContinueExisting) {
    $common + @("--resume", $checkpointBasePath)
}
else {
    $common + @("--load-layer", "3", $layer3Path)
}

Write-Host "starting production S1 window"
Write-Host "target: $WindowHours h; hard bound: $HardMaxHours h; " +
    "RSS bound: $RssLimitGB GiB"
Write-Host "engine: $exe $($engineArgs -join ' ')"
$proc = Start-Process -FilePath $exe -ArgumentList $engineArgs `
    -RedirectStandardOutput $stdout -RedirectStandardError $stderr `
    -PassThru -WindowStyle Hidden
$started = Get-Date
[Int64]$peakRss = 0
$pauseRequested = $false
$markersAtRequest = 0
$stopReason = ""
$lastReport = $started.AddMinutes(-10)

try {
    while (!$proc.HasExited) {
        Start-Sleep -Seconds 10
        $proc.Refresh()
        if ($proc.HasExited) { break }
        $live = Get-Process -Id $proc.Id -ErrorAction SilentlyContinue
        if ($null -eq $live) { break }
        [Int64]$rss = $live.WorkingSet64
        if ($rss -gt $peakRss) { $peakRss = $rss }
        $rssGiB = [Math]::Round($rss / 1GB, 3)
        $availableGiB = Get-AvailableRamGiB
        "$((Get-Date).ToString('o')),$($proc.Id),$rss,$rssGiB,$availableGiB" |
            Add-Content -LiteralPath $rssLog
        $elapsedHours = ((Get-Date) - $started).TotalHours

        if (((Get-Date) - $lastReport).TotalMinutes -ge 5) {
            $markers = Get-CheckpointMarkers $stdout
            Write-Host ("S1 elapsed={0:N2} h rss={1:N3} GiB " +
                "available={2:N3} GiB checkpoints={3}" -f `
                $elapsedHours, $rssGiB, $availableGiB, $markers.Count)
            $lastReport = Get-Date
        }

        if ($rssGiB -gt $RssLimitGB) {
            $stopReason = "rss_guard"
            Stop-Process -Id $proc.Id -Force
            break
        }
        if ($availableGiB -lt $MinimumAvailableGB) {
            $stopReason = "available_ram_guard"
            Stop-Process -Id $proc.Id -Force
            break
        }
        if ($elapsedHours -ge $HardMaxHours) {
            $stopReason = "hard_time_guard"
            Stop-Process -Id $proc.Id -Force
            break
        }
        if (!$pauseRequested -and $elapsedHours -ge $WindowHours) {
            $pauseRequested = $true
            $markersAtRequest = (Get-CheckpointMarkers $stdout).Count
            Write-Host "target window reached; waiting for the next durable " +
                "checkpoint (currently $markersAtRequest)"
        }
        if ($pauseRequested) {
            $markerCount = (Get-CheckpointMarkers $stdout).Count
            if ($markerCount -gt $markersAtRequest) {
                $stopReason = "target_window_checkpoint"
                Write-Host "post-target durable checkpoint observed; " +
                    "stopping S1 process"
                Stop-Process -Id $proc.Id -Force
                break
            }
        }
    }
}
finally {
    Wait-Process -Id $proc.Id -Timeout 60 -ErrorAction SilentlyContinue
    $proc.Refresh()
}

$ended = Get-Date
$elapsed = ($ended - $started).TotalHours
$naturalExit = !$stopReason
$engineFailure = ""
if ($naturalExit) {
    $proc.WaitForExit()
    $proc.Refresh()
    $exitCode = $proc.ExitCode
    if ($null -eq $exitCode) {
        $terminal = Get-Content -Raw -LiteralPath $stdout
        $exitCode = if ($terminal -match "STOPPED AFTER LAYER 4") { 0 } else { 97 }
    }
    if ($exitCode -ne 0) {
        $engineFailure = "production S1 failed with exit code $exitCode"
    }
    else {
        $stopReason = "s1_closed"
    }
}

if ((Test-Path -LiteralPath $stderr) -and
    (Get-Item -LiteralPath $stderr).Length -ne 0) {
    if ($engineFailure) { $engineFailure += "; " }
    $engineFailure += "production S1 wrote stderr"
}

$latest = Get-LatestDurableImage
if ($null -eq $latest) {
    throw "production S1 stopped without a durable checkpoint"
}
$backup = Backup-DurableImage $latest $sessionNumber $stopReason
$markers = Get-CheckpointMarkers $stdout
$lastMarker = if ($markers.Count) { $markers[-1] } else { $null }
$markerText = if ($lastMarker) { $lastMarker.Value } else { "none" }

$afterLayer3Hash = Get-Sha256 $layer3Path
$afterLayer3BackupHash = Get-Sha256 $layer3BackupPath
if ($afterLayer3Hash -ne $expectedLayer3Hash -or
    $afterLayer3BackupHash -ne $expectedLayer3Hash) {
    throw "authoritative L3 or its external backup changed during S1"
}
if (Get-Process -Name "layer_dp_gate" -ErrorAction SilentlyContinue) {
    throw "a layer_dp_gate process remains after the S1 window"
}

$summary = @(
    "mode=C6_PRODUCTION_S1_WINDOW",
    "git_head=$gitHead",
    "session=$sessionNumber",
    "continued=$ContinueExisting",
    "started=$($started.ToString('o'))",
    "ended=$($ended.ToString('o'))",
    "elapsed_hours=$([Math]::Round($elapsed, 4))",
    "target_hours=$WindowHours",
    "hard_max_hours=$HardMaxHours",
    "threads=$Threads",
    "caps=$Caps",
    "chunk_parents=$ChunkParents",
    "checkpoint_minutes=$CheckpointMinutes",
    "rss_limit_gib=$RssLimitGB",
    "peak_rss_gib=$([Math]::Round($peakRss / 1GB, 3))",
    "stop_reason=$stopReason",
    "last_marker=$markerText",
    "durable_source=$($backup.Source)",
    "durable_backup=$($backup.Backup)",
    "durable_sha256=$($backup.Hash)",
    "receipt=$($backup.Receipt)",
    "layer3_sha256=$afterLayer3Hash",
    "layer3_backup_sha256=$afterLayer3BackupHash"
)
$summaryPath = Join-Path $sessionDir "summary.txt"
$summary | Set-Content -LiteralPath $summaryPath -Encoding ascii

Write-Host "PRODUCTION S1 WINDOW CLOSED SAFELY"
Write-Host "reason: $stopReason"
Write-Host "elapsed: $([Math]::Round($elapsed, 3)) h"
Write-Host "peak RSS: $([Math]::Round($peakRss / 1GB, 3)) GiB"
Write-Host "last checkpoint: $markerText"
Write-Host "external backup: $($backup.Backup)"
Write-Host "SHA-256: $($backup.Hash)"
Write-Host "summary: $summaryPath"

if ($engineFailure) {
    Get-Content -LiteralPath $stdout -Tail 100 -ErrorAction SilentlyContinue
    Get-Content -LiteralPath $stderr -ErrorAction SilentlyContinue
    throw $engineFailure
}
if ($stopReason -eq "rss_guard" -or
    $stopReason -eq "available_ram_guard" -or
    $stopReason -eq "hard_time_guard") {
    throw "S1 stopped by safety guard: $stopReason"
}
