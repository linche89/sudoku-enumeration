param(
    [Parameter(Mandatory = $true)]
    [switch]$AuthorizeFullC6,

    [Parameter(Mandatory = $true)]
    [ValidatePattern('^[0-9A-Fa-f]{64}$')]
    [string]$Layer4Sha256,

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
    [int]$StartAvailableGB = 85,
    [string]$Layer4 =
        "data/checkpoints/layer_dp_c6_s1_prod_20260802.L4.snap",
    [string]$Layer4Backup =
        "D:\sudoku_FJ_checkpoint_backups\layer_dp_c6_s1_prod_20260802\layer_dp_c6_s1_prod_20260802.L4.snap",
    [string]$CheckpointBase =
        "data/checkpoints/layer_dp_c6_s2_prod_20260815",
    [string]$ExternalBackupDir =
        "D:\sudoku_FJ_checkpoint_backups\layer_dp_c6_s2_prod_20260815",
    [string]$LogRoot =
        "data/logs/layer_dp_c6_s2_prod_20260815",
    [string]$ProgressSidecar =
        "data/logs/layer_dp_c6_s2_prod_20260815/progress.csv",
    [switch]$ContinueExisting,
    [switch]$PrepareOnly
)

# Bounded production controller for the single C=6 4->5 transition.  It is
# intentionally unusable until S1 has closed and MANIFEST.md supplies the
# exact L4 snapshot SHA-256 via -Layer4Sha256.

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root

if (!$AuthorizeFullC6) { throw "-AuthorizeFullC6 is required" }
if ($Threads -lt 2) { throw "Threads must be at least 2" }
if ($CheckpointMinutes -le 0 -or $ChunkParents -eq 0) {
    throw "checkpoint period and chunk size must be positive"
}
if ($HardMaxHours -le $WindowHours) {
    throw "HardMaxHours must exceed WindowHours"
}
if ($RssLimitGB -lt 1 -or $MinimumAvailableGB -lt 1 -or
    $StartAvailableGB -lt 1) {
    throw "RAM guards must be positive"
}
$Layer4Sha256 = $Layer4Sha256.ToUpperInvariant()

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
    param([string]$Path, [string]$Parent, [string]$Label)
    $full = [IO.Path]::GetFullPath($Path)
    $parentFull = [IO.Path]::GetFullPath($Parent).TrimEnd('\', '/') + '\'
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
            throw "available RAM did not reach $StartAvailableGB GiB"
        }
        Write-Host "waiting for RAM ($available/$StartAvailableGB GiB)"
        Start-Sleep -Seconds 10
    }
}

function Invoke-LoggedProcess {
    param([string]$FilePath, [string[]]$Arguments, [string]$StdoutPath,
          [string]$StderrPath, [string]$SuccessPattern)
    $child = Start-Process -FilePath $FilePath -ArgumentList $Arguments `
        -RedirectStandardOutput $StdoutPath `
        -RedirectStandardError $StderrPath -PassThru -WindowStyle Hidden
    $child.WaitForExit()
    $child.Refresh()
    if ($null -ne $child.ExitCode) { return [int]$child.ExitCode }
    if ((Get-Content -Raw -LiteralPath $StdoutPath) -match $SuccessPattern) {
        return 0
    }
    return 97
}

function Get-LatestDurableImage {
    $snapshot = $checkpointBasePath + ".L5.snap"
    if (Test-Path -LiteralPath $snapshot) {
        return Get-Item -Force -LiteralPath $snapshot
    }
    $candidates = @(
        ($checkpointBasePath + ".a"),
        ($checkpointBasePath + ".b")
    ) | Where-Object { Test-Path -LiteralPath $_ }
    if (!$candidates.Count) { return $null }
    return $candidates | Get-Item -Force |
        Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 1
}

function Backup-DurableImage {
    param([IO.FileInfo]$Source, [int]$SessionNumber, [string]$Reason)
    $sourceHash = Get-Sha256 $Source.FullName
    $receipts = @(Get-ChildItem -LiteralPath $externalBackupPath `
        -Filter "*.sha256.txt" -File -ErrorAction SilentlyContinue)
    foreach ($receipt in $receipts) {
        $text = Get-Content -Raw -LiteralPath $receipt.FullName
        if ($text -match "(?m)^sha256=$sourceHash\r?$") {
            $backupPath = (($text -split "`r?`n" | Where-Object {
                $_ -like "backup_path=*"
            } | Select-Object -First 1) -replace '^backup_path=', '')
            if (!(Test-Path -LiteralPath $backupPath) -or
                (Get-Sha256 $backupPath) -ne $sourceHash) {
                throw "receipt $($receipt.FullName) does not verify its backup"
            }
            return [pscustomobject]@{
                Source=$Source.FullName; Hash=$sourceHash
                Backup=$backupPath; Receipt=$receipt.FullName
            }
        }
    }
    $isSnapshot = $Source.Name.EndsWith(
        ".L5.snap", [StringComparison]::OrdinalIgnoreCase)
    $destName = if ($isSnapshot) {
        $Source.Name
    } else {
        "session-{0:D3}-{1}-{2}" -f $SessionNumber,
            (Get-Date -Format "yyyyMMdd-HHmmss"), $Source.Name
    }
    $dest = Join-Path $externalBackupPath $destName
    $temp = $dest + ".copying"
    if ((Test-Path -LiteralPath $dest) -or (Test-Path -LiteralPath $temp)) {
        throw "refusing to overwrite external backup target $dest"
    }
    Copy-Item -LiteralPath $Source.FullName -Destination $temp
    if ((Get-Sha256 $temp) -ne $sourceHash) {
        throw "external backup SHA-256 mismatch for $temp"
    }
    Move-Item -LiteralPath $temp -Destination $dest
    $receiptPath = $dest + ".sha256.txt"
    @(
        "mode=$(if ($isSnapshot) {'C6_PRODUCTION_L5_SNAPSHOT'} else {'C6_PRODUCTION_S2_PARTIAL'})",
        "reason=$Reason",
        "created=$((Get-Date).ToString('o'))",
        "source_path=$($Source.FullName)",
        "backup_path=$dest",
        "bytes=$($Source.Length)",
        "sha256=$sourceHash",
        "git_head=$gitHead"
    ) | Set-Content -LiteralPath $receiptPath -Encoding ascii
    return [pscustomobject]@{
        Source=$Source.FullName; Hash=$sourceHash
        Backup=$dest; Receipt=$receiptPath
    }
}

function Get-CheckpointMarkers {
    param([string]$Path)
    if (!(Test-Path -LiteralPath $Path)) { return @() }
    try {
        return @([regex]::Matches(
            (Get-Content -Raw -LiteralPath $Path),
            'checkpoint gen=(\d+) trans=4->5 chunk=(\d+)/(\d+) ' +
            'entries=(\d+) -> (.+?) \(([0-9.]+)s\)'))
    }
    catch { return @() }
}

function Add-Progress {
    param([Text.RegularExpressions.Match]$Marker, [int]$SessionNumber,
          [datetime]$Started, [Int64]$RssBytes, [double]$AvailableGiB,
          [string]$Event = "checkpoint", [string]$KnownSha256 = "",
          [string]$Checkpoint = "")
    $invariant = [Globalization.CultureInfo]::InvariantCulture
    if ($Marker) {
        $Checkpoint = $Marker.Groups[5].Value.Trim()
        if ($Checkpoint -notin @(
                ($checkpointBasePath + ".a"),
                ($checkpointBasePath + ".b")
            )) {
            throw "checkpoint marker names unexpected path $Checkpoint"
        }
    }
    $progressArgs = @(
        $progressHelper, $Checkpoint,
        "--progress", $progressPath, "--parent", $layer4Path,
        "--event", $Event, "--session", "$SessionNumber",
        "--git-head", $gitHead, "--observed-at", (Get-Date).ToString('o'),
        "--elapsed-seconds",
            (((Get-Date) - $Started).TotalSeconds).ToString("F3", $invariant),
        "--rss-bytes", "$RssBytes", "--rss-gib",
            ($RssBytes / 1GB).ToString("F6", $invariant),
        "--available-gib", $AvailableGiB.ToString("F3", $invariant)
    )
    if ($Marker) {
        $progressArgs += @(
            "--checkpoint-write-seconds", $Marker.Groups[6].Value,
            "--expect-generation", $Marker.Groups[1].Value,
            "--expect-cursor", $Marker.Groups[2].Value,
            "--expect-nchunks", $Marker.Groups[3].Value
        )
    }
    if ($KnownSha256) { $progressArgs += @("--known-sha256", $KnownSha256) }
    $output = @(& python @progressArgs 2>&1)
    if ($LASTEXITCODE -ne 0) {
        throw "progress sidecar append failed: $($output -join ' ')"
    }
    Write-Host "progress sidecar: $($output -join ' ')"
}

$checkpointRoot = Join-Path $root "data/checkpoints"
$logParent = Join-Path $root "data/logs"
$checkpointBasePath = Assert-PathUnder $CheckpointBase $checkpointRoot `
    "CheckpointBase"
$logRootPath = Assert-PathUnder $LogRoot $logParent "LogRoot"
$progressPath = Assert-PathUnder $ProgressSidecar $logParent "ProgressSidecar"
$externalRoot = "D:\sudoku_FJ_checkpoint_backups"
$externalBackupPath = Assert-PathUnder $ExternalBackupDir $externalRoot `
    "ExternalBackupDir"
if ([IO.Path]::GetPathRoot($checkpointBasePath) -eq
    [IO.Path]::GetPathRoot($externalBackupPath)) {
    throw "external backup must be on a different volume"
}
if ([IO.Path]::GetFileName($checkpointBasePath) -notlike
    "layer_dp_c6_s2_prod_*") {
    throw "CheckpointBase must use a layer_dp_c6_s2_prod_* basename"
}

$dirtyTracked = @(& git status --porcelain --untracked-files=no)
if ($LASTEXITCODE -ne 0) { throw "git status failed" }
if ($dirtyTracked.Count) { throw "production S2 requires a clean tracked worktree" }
$gitHead = (& git rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0) { throw "cannot resolve Git HEAD" }
if (Get-Process -Name "layer_dp_gate" -ErrorAction SilentlyContinue) {
    throw "another layer_dp_gate process is already running"
}

$layer4Path = (Resolve-Path -LiteralPath $Layer4).Path
$layer4BackupPath = (Resolve-Path -LiteralPath $Layer4Backup).Path
if ((Get-Sha256 $layer4Path) -ne $Layer4Sha256 -or
    (Get-Sha256 $layer4BackupPath) -ne $Layer4Sha256) {
    throw "local/external L4 hashes do not match -Layer4Sha256"
}
$progressHelper = (Resolve-Path -LiteralPath `
    "scripts/layer_dp_progress.py").Path
New-Item -ItemType Directory -Force -Path $logRootPath | Out-Null
New-Item -ItemType Directory -Force -Path $externalBackupPath | Out-Null

$sessionNumber = 1
while (Test-Path -LiteralPath (Join-Path $logRootPath (
        "session-{0:D3}" -f $sessionNumber))) { $sessionNumber++ }
$sessionDir = Join-Path $logRootPath ("session-{0:D3}" -f $sessionNumber)
New-Item -ItemType Directory -Path $sessionDir | Out-Null

$gateOut = Join-Path $sessionDir "verify_all.stdout.log"
$gateErr = Join-Path $sessionDir "verify_all.stderr.log"
$gateArgs = @("-NoProfile", "-ExecutionPolicy", "Bypass", "-File",
              (Join-Path $PSScriptRoot "verify_all.ps1"))
if ((Invoke-LoggedProcess "powershell.exe" $gateArgs $gateOut $gateErr `
        "ALL REPOSITORY CHECKS PASSED") -ne 0) {
    throw "complete repository gate failed"
}
$buildOut = Join-Path $sessionDir "build.log"
$buildErr = Join-Path $sessionDir "build.stderr.log"
$buildArgs = @("-NoProfile", "-ExecutionPolicy", "Bypass", "-File",
               (Join-Path $PSScriptRoot "build_layer_dp.ps1"))
if ((Invoke-LoggedProcess "powershell.exe" $buildArgs $buildOut $buildErr `
        "built layer-DP gate engine") -ne 0) {
    throw "layer-DP build failed"
}
$exe = (Resolve-Path -LiteralPath "build/layer_dp_gate.exe").Path

$existing = @(
    ($checkpointBasePath + ".a"), ($checkpointBasePath + ".b"),
    ($checkpointBasePath + ".L4.snap"), ($checkpointBasePath + ".L5.snap"),
    ($checkpointBasePath + ".tmp")
) | Where-Object { Test-Path -LiteralPath $_ }
if ($ContinueExisting) {
    if (!(Test-Path -LiteralPath ($checkpointBasePath + ".a")) -and
        !(Test-Path -LiteralPath ($checkpointBasePath + ".b")) -and
        !(Test-Path -LiteralPath ($checkpointBasePath + ".L5.snap"))) {
        throw "-ContinueExisting requested without an S2 checkpoint"
    }
}
elseif ($existing.Count) {
    throw "fresh S2 base already has files; use -ContinueExisting"
}
if (Test-Path -LiteralPath ($checkpointBasePath + ".L5.snap")) {
    throw "S2 is already closed; proceed to the separately authorized S3"
}

$beforeBackup = $null
if ($ContinueExisting) {
    $beforeImage = Get-LatestDurableImage
    if ($null -eq $beforeImage) { throw "resume base has no durable image" }
    $beforeBackup = Backup-DurableImage $beforeImage $sessionNumber `
        "pre-resume coverage"
}

Wait-StartMemory
$preflightOut = Join-Path $sessionDir "resource-preflight.log"
$preflightErr = Join-Path $sessionDir "resource-preflight.stderr.log"
$preflightArgs = @("6", "--threads", "$Threads", "--caps", $Caps,
    "--checkpoint", $checkpointBasePath, "$CheckpointMinutes",
    "--resource-preflight", "4")
if ((Invoke-LoggedProcess $exe $preflightArgs $preflightOut $preflightErr `
        "RESOURCE PREFLIGHT PASS") -ne 0) {
    throw "C=6 S2 resource preflight failed"
}
if ($PrepareOnly) {
    Write-Host "PRODUCTION S2 PREPARATION PASSED (no checkpoint written)"
    exit 0
}

$started = Get-Date
if ($ContinueExisting) {
    Add-Progress -Marker $null -SessionNumber $sessionNumber `
        -Started $started -RssBytes 0 -AvailableGiB (Get-AvailableRamGiB) `
        -Event "resume_baseline" -KnownSha256 $beforeBackup.Hash `
        -Checkpoint $beforeBackup.Source
}

$stdout = Join-Path $sessionDir "stdout.log"
$stderr = Join-Path $sessionDir "stderr.log"
$rssLog = Join-Path $sessionDir "rss.csv"
"timestamp,pid,rss_bytes,rss_gib,available_gib" |
    Set-Content -LiteralPath $rssLog -Encoding ascii
$common = @("6", "--threads", "$Threads", "--caps", $Caps,
    "--ckpt-chunk", "$ChunkParents", "--checkpoint", $checkpointBasePath,
    "$CheckpointMinutes", "--stop-after", "5", "--ack-full-c6")
$engineArgs = if ($ContinueExisting) {
    $common + @("--resume", $checkpointBasePath)
} else {
    $common + @("--load-layer", "4", $layer4Path)
}
@(
    "git_head=$gitHead",
    "created=$((Get-Date).ToString('o'))",
    "command=$exe $($engineArgs -join ' ')"
) | Set-Content -LiteralPath (Join-Path $sessionDir "command.txt") `
    -Encoding ascii
$proc = Start-Process -FilePath $exe -ArgumentList $engineArgs `
    -RedirectStandardOutput $stdout -RedirectStandardError $stderr `
    -PassThru -WindowStyle Hidden
[Int64]$peakRss = 0
[Int64]$lastRss = 0
$lastAvailable = Get-AvailableRamGiB
$processedMarkers = 0
$pauseRequested = $false
$markersAtRequest = 0
$stopReason = ""
$progressFailure = ""
try {
    while (!$proc.HasExited) {
        Start-Sleep -Seconds 10
        $proc.Refresh()
        if ($proc.HasExited) { break }
        $live = Get-Process -Id $proc.Id -ErrorAction SilentlyContinue
        if ($null -eq $live) { break }
        [Int64]$lastRss = $live.WorkingSet64
        if ($lastRss -gt $peakRss) { $peakRss = $lastRss }
        $lastAvailable = Get-AvailableRamGiB
        $rssGiB = [Math]::Round($lastRss / 1GB, 3)
        "$((Get-Date).ToString('o')),$($proc.Id),$lastRss,$rssGiB,$lastAvailable" |
            Add-Content -LiteralPath $rssLog
        $elapsedHours = ((Get-Date) - $started).TotalHours
        $markers = @(Get-CheckpointMarkers $stdout)
        try {
            for ($i = $processedMarkers; $i -lt $markers.Count; $i++) {
                Add-Progress -Marker $markers[$i] -SessionNumber $sessionNumber `
                    -Started $started -RssBytes $lastRss `
                    -AvailableGiB $lastAvailable
            }
            $processedMarkers = $markers.Count
        }
        catch {
            $progressFailure = $_.Exception.Message
            $stopReason = "progress_sidecar_failure"
            Stop-Process -Id $proc.Id -Force
            break
        }
        if ($rssGiB -gt $RssLimitGB) {
            $stopReason = "rss_guard"; Stop-Process -Id $proc.Id -Force; break
        }
        if ($lastAvailable -lt $MinimumAvailableGB) {
            $stopReason = "available_ram_guard"
            Stop-Process -Id $proc.Id -Force; break
        }
        if ($elapsedHours -ge $HardMaxHours) {
            $stopReason = "hard_time_guard"; Stop-Process -Id $proc.Id -Force; break
        }
        if (!$pauseRequested -and $elapsedHours -ge $WindowHours) {
            $pauseRequested = $true
            $markersAtRequest = $markers.Count
            Write-Host "target reached; waiting for the next durable checkpoint"
        }
        if ($pauseRequested -and $markers.Count -gt $markersAtRequest) {
            $stopReason = "target_window_checkpoint"
            Stop-Process -Id $proc.Id -Force
            break
        }
    }
}
finally {
    Wait-Process -Id $proc.Id -Timeout 60 -ErrorAction SilentlyContinue
    $proc.Refresh()
}

if (!$progressFailure) {
    try {
        $markers = @(Get-CheckpointMarkers $stdout)
        for ($i = $processedMarkers; $i -lt $markers.Count; $i++) {
            Add-Progress -Marker $markers[$i] -SessionNumber $sessionNumber `
                -Started $started -RssBytes $lastRss `
                -AvailableGiB $lastAvailable
        }
        $processedMarkers = $markers.Count
    }
    catch { $progressFailure = $_.Exception.Message }
}

$naturalExit = !$stopReason
$engineFailure = ""
if ($naturalExit) {
    $proc.WaitForExit(); $proc.Refresh(); $exitCode = $proc.ExitCode
    if ($null -eq $exitCode) {
        $exitCode = if ((Get-Content -Raw -LiteralPath $stdout) -match
            "STOPPED AFTER LAYER 5") { 0 } else { 97 }
    }
    if ($exitCode -ne 0) { $engineFailure = "S2 failed with exit code $exitCode" }
    else { $stopReason = "s2_closed" }
}
if ((Test-Path -LiteralPath $stderr) -and
    (Get-Item -LiteralPath $stderr).Length -ne 0) {
    $engineFailure = ($engineFailure + "; S2 wrote stderr").TrimStart(';', ' ')
}
$latest = Get-LatestDurableImage
if ($null -eq $latest) { throw "S2 stopped without a durable checkpoint" }
$backup = Backup-DurableImage $latest $sessionNumber $stopReason
if ((Get-Sha256 $layer4Path) -ne $Layer4Sha256 -or
    (Get-Sha256 $layer4BackupPath) -ne $Layer4Sha256) {
    throw "local or external immutable L4 changed during S2"
}
if (Get-Process -Name "layer_dp_gate" -ErrorAction SilentlyContinue) {
    throw "a layer_dp_gate process remains after S2"
}
$ended = Get-Date
$summary = @(
    "mode=C6_PRODUCTION_S2_WINDOW", "git_head=$gitHead",
    "session=$sessionNumber", "continued=$ContinueExisting",
    "started=$($started.ToString('o'))", "ended=$($ended.ToString('o'))",
    "elapsed_hours=$([Math]::Round(($ended-$started).TotalHours,4))",
    "target_hours=$WindowHours", "hard_max_hours=$HardMaxHours",
    "threads=$Threads", "caps=$Caps", "chunk_parents=$ChunkParents",
    "checkpoint_minutes=$CheckpointMinutes", "rss_limit_gib=$RssLimitGB",
    "peak_rss_gib=$([Math]::Round($peakRss/1GB,3))",
    "stop_reason=$stopReason", "durable_source=$($backup.Source)",
    "durable_backup=$($backup.Backup)", "durable_sha256=$($backup.Hash)",
    "receipt=$($backup.Receipt)", "progress_sidecar=$progressPath",
    "layer4_sha256=$Layer4Sha256",
    "engine_command=$exe $($engineArgs -join ' ')"
)
$summaryPath = Join-Path $sessionDir "summary.txt"
$summary | Set-Content -LiteralPath $summaryPath -Encoding ascii
Write-Host "PRODUCTION S2 WINDOW CLOSED SAFELY: $stopReason"
Write-Host "summary: $summaryPath"
if ($progressFailure) {
    throw "S2 progress sidecar failed after preserving the durable image: " +
          $progressFailure
}
if ($engineFailure) { throw $engineFailure }
if ($stopReason -in @("rss_guard", "available_ram_guard", "hard_time_guard")) {
    throw "S2 stopped by safety guard: $stopReason"
}
