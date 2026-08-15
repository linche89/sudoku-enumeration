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
    [string]$ProgressSidecar =
        "data/logs/layer_dp_c6_s1_prod_20260802/progress.csv",
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
    $snapshot = $checkpointBasePath + ".L4.snap"
    if (Test-Path -LiteralPath $snapshot) {
        return Get-Item -Force -LiteralPath $snapshot
    }
    $candidates = @(
        ($checkpointBasePath + ".a"),
        ($checkpointBasePath + ".b")
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

    $isSnapshot = $Source.Name.EndsWith(
        ".L4.snap", [StringComparison]::OrdinalIgnoreCase)
    $stamp = Get-Date -Format "yyyyMMdd-HHmmss"
    $destName = if ($isSnapshot) {
        $Source.Name
    }
    else {
        "session-{0:D3}-{1}-{2}" -f `
            $SessionNumber, $stamp, $Source.Name
    }
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
        "mode=$(if ($isSnapshot) {'C6_PRODUCTION_L4_SNAPSHOT'} else {'C6_PRODUCTION_S1_PARTIAL'})",
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

function Add-LayerProgress {
    param(
        [Parameter(Mandatory = $true)][string]$Checkpoint,
        [Parameter(Mandatory = $true)][string]$Parent,
        [Parameter(Mandatory = $true)][string]$Event,
        [Parameter(Mandatory = $true)][int]$SessionNumber,
        [Parameter(Mandatory = $true)][string]$GitHead,
        [Parameter(Mandatory = $true)][string]$ObservedAt,
        [Parameter(Mandatory = $true)][double]$ElapsedSeconds,
        [Parameter(Mandatory = $true)][Int64]$RssBytes,
        [Parameter(Mandatory = $true)][double]$AvailableGiB,
        [double]$CheckpointWriteSeconds = 0,
        [Nullable[UInt64]]$ExpectedGeneration = $null,
        [Nullable[UInt64]]$ExpectedCursor = $null,
        [Nullable[UInt64]]$ExpectedChunks = $null,
        [string]$KnownSha256 = ""
    )
    $invariant = [Globalization.CultureInfo]::InvariantCulture
    $progressArgs = @(
        $progressHelper,
        $Checkpoint,
        "--progress", $progressPath,
        "--parent", $Parent,
        "--event", $Event,
        "--session", "$SessionNumber",
        "--git-head", $GitHead,
        "--observed-at", $ObservedAt,
        "--elapsed-seconds", $ElapsedSeconds.ToString("F3", $invariant),
        "--rss-bytes", "$RssBytes",
        "--rss-gib", ($RssBytes / 1GB).ToString("F6", $invariant),
        "--available-gib", $AvailableGiB.ToString("F3", $invariant)
    )
    if ($CheckpointWriteSeconds -gt 0) {
        $progressArgs += @(
            "--checkpoint-write-seconds",
            $CheckpointWriteSeconds.ToString("F3", $invariant)
        )
    }
    if ($null -ne $ExpectedGeneration) {
        $progressArgs += @("--expect-generation", "$ExpectedGeneration")
    }
    if ($null -ne $ExpectedCursor) {
        $progressArgs += @("--expect-cursor", "$ExpectedCursor")
    }
    if ($null -ne $ExpectedChunks) {
        $progressArgs += @("--expect-nchunks", "$ExpectedChunks")
    }
    if ($KnownSha256) {
        $progressArgs += @("--known-sha256", $KnownSha256)
    }
    $output = @(& python @progressArgs 2>&1)
    if ($LASTEXITCODE -ne 0) {
        throw "progress sidecar append failed: $($output -join ' ')"
    }
    Write-Host "progress sidecar: $($output -join ' ')"
}

function Sync-CheckpointProgress {
    param(
        [Parameter(Mandatory = $true)][string]$StdoutPath,
        [Parameter(Mandatory = $true)][int]$ProcessedCount,
        [Parameter(Mandatory = $true)][int]$SessionNumber,
        [Parameter(Mandatory = $true)][string]$GitHead,
        [Parameter(Mandatory = $true)][datetime]$Started,
        [Parameter(Mandatory = $true)][Int64]$RssBytes,
        [Parameter(Mandatory = $true)][double]$AvailableGiB,
        [Parameter(Mandatory = $true)][string]$Parent
    )
    $markers = @(Get-CheckpointMarkers $StdoutPath)
    if ($markers.Count -lt $ProcessedCount) {
        throw "checkpoint marker log moved backwards"
    }
    for ($i = $ProcessedCount; $i -lt $markers.Count; $i++) {
        $marker = $markers[$i]
        $checkpoint = $marker.Groups[5].Value.Trim()
        $allowed = @(
            ($checkpointBasePath + ".a"),
            ($checkpointBasePath + ".b")
        )
        if ($checkpoint -notin $allowed) {
            throw "checkpoint marker names unexpected path $checkpoint"
        }
        Add-LayerProgress `
            -Checkpoint $checkpoint -Parent $Parent -Event "checkpoint" `
            -SessionNumber $SessionNumber -GitHead $GitHead `
            -ObservedAt ((Get-Date).ToString('o')) `
            -ElapsedSeconds (((Get-Date) - $Started).TotalSeconds) `
            -RssBytes $RssBytes -AvailableGiB $AvailableGiB `
            -CheckpointWriteSeconds ([double]$marker.Groups[6].Value) `
            -ExpectedGeneration ([UInt64]$marker.Groups[1].Value) `
            -ExpectedCursor ([UInt64]$marker.Groups[2].Value) `
            -ExpectedChunks ([UInt64]$marker.Groups[3].Value)
    }
    return $markers.Count
}

function Invoke-LoggedProcess {
    param(
        [Parameter(Mandatory = $true)][string]$FilePath,
        [string[]]$Arguments = @(),
        [Parameter(Mandatory = $true)][string]$StdoutPath,
        [Parameter(Mandatory = $true)][string]$StderrPath,
        [string]$NullExitSuccessPattern = ""
    )
    $child = Start-Process -FilePath $FilePath -ArgumentList $Arguments `
        -RedirectStandardOutput $StdoutPath `
        -RedirectStandardError $StderrPath -PassThru -WindowStyle Hidden
    $child.WaitForExit()
    $child.Refresh()
    if ($null -eq $child.ExitCode) {
        if ($NullExitSuccessPattern -and
            (Get-Content -Raw -LiteralPath $StdoutPath) -match
                $NullExitSuccessPattern) {
            return 0
        }
        return 97
    }
    return [int]$child.ExitCode
}

$checkpointRoot = Join-Path $root "data/checkpoints"
$logParent = Join-Path $root "data/logs"
$checkpointBasePath = Assert-PathUnder $CheckpointBase $checkpointRoot `
    "CheckpointBase"
$logRootPath = Assert-PathUnder $LogRoot $logParent "LogRoot"
$progressPath = Assert-PathUnder $ProgressSidecar $logParent `
    "ProgressSidecar"
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

$dirty = @(& git status --porcelain --untracked-files=no)
if ($LASTEXITCODE -ne 0) { throw "git status failed" }
if ($dirty.Count) {
    throw "production S1 requires a clean tracked worktree"
}
$gitHead = (& git rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0) { throw "cannot resolve Git HEAD" }
$progressHelper = (Resolve-Path -LiteralPath `
    "scripts/layer_dp_progress.py").Path

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
    $gateErrorLog "ALL REPOSITORY CHECKS PASSED"
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
    $buildErrorLog "built layer-DP gate engine and penultimate-layer counter"
if ($buildCode -ne 0) {
    Get-Content -LiteralPath $buildLog -Tail 100
    Get-Content -LiteralPath $buildErrorLog -Tail 100
    throw "layer-DP build failed"
}
$exe = (Resolve-Path -LiteralPath "build/layer_dp_gate.exe").Path

$existing = @(
    ($checkpointBasePath + ".a"),
    ($checkpointBasePath + ".b"),
    ($checkpointBasePath + ".L3.snap"),
    ($checkpointBasePath + ".L4.snap"),
    ($checkpointBasePath + ".tmp")
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

$beforeBackup = $null
if ($ContinueExisting) {
    $beforeImage = Get-LatestDurableImage
    if ($null -eq $beforeImage) {
        throw "resume base has no durable image"
    }
    $beforeBackup = Backup-DurableImage $beforeImage $sessionNumber `
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
    $preflightErrorLog "RESOURCE PREFLIGHT PASS"
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

if ($ContinueExisting) {
    Add-LayerProgress `
        -Checkpoint $beforeBackup.Source -Parent $layer3Path `
        -Event "resume_baseline" -SessionNumber $sessionNumber `
        -GitHead $gitHead -ObservedAt ((Get-Date).ToString('o')) `
        -ElapsedSeconds 0 -RssBytes 0 `
        -AvailableGiB (Get-AvailableRamGiB) `
        -KnownSha256 $beforeBackup.Hash
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
Write-Host ("target: {0} h; hard bound: {1} h; RSS bound: {2} GiB" -f `
    $WindowHours, $HardMaxHours, $RssLimitGB)
Write-Host "engine: $exe $($engineArgs -join ' ')"
$proc = Start-Process -FilePath $exe -ArgumentList $engineArgs `
    -RedirectStandardOutput $stdout -RedirectStandardError $stderr `
    -PassThru -WindowStyle Hidden
$started = Get-Date
[Int64]$peakRss = 0
[Int64]$lastRss = 0
$lastAvailableGiB = Get-AvailableRamGiB
$processedMarkerCount = 0
$progressFailure = ""
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
        $lastRss = $rss
        if ($rss -gt $peakRss) { $peakRss = $rss }
        $rssGiB = [Math]::Round($rss / 1GB, 3)
        $availableGiB = Get-AvailableRamGiB
        $lastAvailableGiB = $availableGiB
        "$((Get-Date).ToString('o')),$($proc.Id),$rss,$rssGiB,$availableGiB" |
            Add-Content -LiteralPath $rssLog
        $elapsedHours = ((Get-Date) - $started).TotalHours

        try {
            $processedMarkerCount = Sync-CheckpointProgress `
                -StdoutPath $stdout -ProcessedCount $processedMarkerCount `
                -SessionNumber $sessionNumber -GitHead $gitHead `
                -Started $started -RssBytes $rss `
                -AvailableGiB $availableGiB -Parent $layer3Path
        }
        catch {
            $progressFailure = $_.Exception.Message
            $stopReason = "progress_sidecar_failure"
            Stop-Process -Id $proc.Id -Force
            break
        }

        if (((Get-Date) - $lastReport).TotalMinutes -ge 5) {
            $markers = Get-CheckpointMarkers $stdout
            Write-Host (("S1 elapsed={0:N2} h rss={1:N3} GiB " +
                "available={2:N3} GiB checkpoints={3}") -f `
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

if (!$progressFailure) {
    try {
        $processedMarkerCount = Sync-CheckpointProgress `
            -StdoutPath $stdout -ProcessedCount $processedMarkerCount `
            -SessionNumber $sessionNumber -GitHead $gitHead `
            -Started $started -RssBytes $lastRss `
            -AvailableGiB $lastAvailableGiB -Parent $layer3Path
    }
    catch {
        $progressFailure = $_.Exception.Message
        if (!$stopReason) { $stopReason = "progress_sidecar_failure" }
    }
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
    "controller_pid=$PID",
    "engine_pid=$($proc.Id)",
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
    "progress_sidecar=$progressPath",
    "progress_rows=$(@(Import-Csv -LiteralPath $progressPath).Count)",
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

if ($progressFailure) {
    throw "S1 progress sidecar failed after preserving the newest durable " +
          "checkpoint: $progressFailure"
}
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
