param(
    [Parameter(Mandatory = $true)]
    [switch]$AuthorizeFullC6,

    [Parameter(Mandatory = $true)]
    [ValidatePattern('^[0-9A-Fa-f]{64}$')]
    [string]$Layer5Sha256,

    [string]$ExpectedN =
        "38296278920738107863746324732012492486187417600000",
    [int]$Threads = 24,
    [string]$Caps = "2000,14000000,1350000000,250000000,100000",
    [UInt64]$ChunkParents = 100000,
    [double]$CheckpointMinutes = 40.0,
    [ValidateRange(1.0, 24.0)]
    [double]$MaxHoursPerAttempt = 10.0,
    [int]$RssLimitGB = 32,
    [int]$MinimumAvailableGB = 8,
    [int]$StartAvailableGB = 32,
    [string]$Layer5 =
        "data/checkpoints/layer_dp_c6_s2_prod_20260815.L5.snap",
    [string]$Layer5Backup =
        "D:\sudoku_FJ_checkpoint_backups\layer_dp_c6_s2_prod_20260815\layer_dp_c6_s2_prod_20260815.L5.snap",
    [string]$PrimaryBase =
        "data/checkpoints/layer_dp_c6_s3_prod_20260815_primary",
    [string]$ReplayBase =
        "data/checkpoints/layer_dp_c6_s3_prod_20260815_replay",
    [string]$ExternalBackupDir =
        "D:\sudoku_FJ_checkpoint_backups\layer_dp_c6_s3_prod_20260815",
    [string]$LogRoot =
        "data/logs/layer_dp_c6_s3_prod_20260815",
    [switch]$ContinueExisting,
    [switch]$PrepareOnly
)

# Final production controller.  It contracts one immutable, hash-pinned L5
# snapshot twice in independent checkpoint namespaces, requires byte-identical
# CSVs, runs the semantic certificate verifier, and cross-checks the exact
# square sum in Python and PowerShell/.NET BigInteger.

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root

if (!$AuthorizeFullC6) { throw "-AuthorizeFullC6 is required" }
if ($Threads -lt 2) { throw "Threads must be at least 2" }
if ($CheckpointMinutes -le 0 -or $ChunkParents -eq 0) {
    throw "checkpoint period and chunk size must be positive"
}
if ($MaxHoursPerAttempt -le 0 -or $RssLimitGB -lt 1 -or
    $MinimumAvailableGB -lt 1 -or $StartAvailableGB -lt 1) {
    throw "time and RAM guards must be positive"
}
if ($ExpectedN -notmatch '^[0-9]+$') { throw "ExpectedN must be decimal" }
$Layer5Sha256 = $Layer5Sha256.ToUpperInvariant()

function Get-Sha256 {
    param([Parameter(Mandatory = $true)][string]$Path)
    $stream = [IO.File]::OpenRead($Path)
    $sha = [Security.Cryptography.SHA256]::Create()
    try {
        return ([BitConverter]::ToString(
            $sha.ComputeHash($stream))).Replace("-", "")
    }
    finally { $sha.Dispose(); $stream.Dispose() }
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
    param([string]$Label)
    $deadline = (Get-Date).AddMinutes(10)
    while ($true) {
        $available = Get-AvailableRamGiB
        if ($available -ge $StartAvailableGB) {
            Write-Host "$Label`: available RAM $available GiB [READY]"
            return
        }
        if ((Get-Date) -ge $deadline) {
            throw "$Label`: available RAM did not reach $StartAvailableGB GiB"
        }
        Start-Sleep -Seconds 10
    }
}

function Invoke-LoggedProcess {
    param([string]$FilePath, [string[]]$Arguments, [string]$StdoutPath,
          [string]$StderrPath, [string]$SuccessPattern)
    $child = Start-Process -FilePath $FilePath -ArgumentList $Arguments `
        -RedirectStandardOutput $StdoutPath `
        -RedirectStandardError $StderrPath -PassThru -WindowStyle Hidden
    $child.WaitForExit(); $child.Refresh()
    if ($null -ne $child.ExitCode) { return [int]$child.ExitCode }
    if ((Get-Content -Raw -LiteralPath $StdoutPath) -match $SuccessPattern) {
        return 0
    }
    return 97
}

function Get-CheckpointMarkers {
    param([string]$Path)
    if (!(Test-Path -LiteralPath $Path)) { return @() }
    try {
        return @([regex]::Matches(
            (Get-Content -Raw -LiteralPath $Path),
            'checkpoint gen=(\d+) trans=5->6 chunk=(\d+)/(\d+) ' +
            'entries=(\d+) -> (.+?) \(([0-9.]+)s\)'))
    }
    catch { return @() }
}

function Get-LatestImage {
    param([string]$Base)
    $candidates = @(
        ($Base + ".L6.snap"), ($Base + ".a"), ($Base + ".b")
    ) | Where-Object { Test-Path -LiteralPath $_ }
    if (!$candidates.Count) { return $null }
    return $candidates | Get-Item -Force |
        Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 1
}

function Backup-Artifact {
    param([IO.FileInfo]$Source, [string]$Label, [string]$Reason)
    $hash = Get-Sha256 $Source.FullName
    $safeLabel = $Label -replace '[^A-Za-z0-9_.-]', '_'
    $isFinal = $Source.Name.EndsWith(
        ".L6.snap", [StringComparison]::OrdinalIgnoreCase)
    $destName = if ($isFinal) {
        "$safeLabel.L6.snap"
    } else {
        "$safeLabel-$((Get-Date).ToString('yyyyMMdd-HHmmss'))-$($Source.Name)"
    }
    $dest = Join-Path $externalPath $destName
    $receipt = $dest + ".sha256.txt"
    if (Test-Path -LiteralPath $receipt) {
        $text = Get-Content -Raw -LiteralPath $receipt
        if ($text -match "(?m)^sha256=$hash\r?$" -and
            (Test-Path -LiteralPath $dest) -and
            (Get-Sha256 $dest) -eq $hash) {
            return [pscustomobject]@{
                Source=$Source.FullName; Backup=$dest; Hash=$hash; Receipt=$receipt
            }
        }
        throw "existing receipt does not match $Source"
    }
    if (Test-Path -LiteralPath $dest) {
        throw "refusing to overwrite external artifact $dest"
    }
    $temp = $dest + ".copying"
    if (Test-Path -LiteralPath $temp) {
        throw "unfinished external copy exists: $temp"
    }
    Copy-Item -LiteralPath $Source.FullName -Destination $temp
    if ((Get-Sha256 $temp) -ne $hash) {
        throw "external artifact hash mismatch"
    }
    Move-Item -LiteralPath $temp -Destination $dest
    @(
        "mode=$(if ($isFinal) {'C6_PRODUCTION_FINAL_WIDE_SNAPSHOT'} else {'C6_PRODUCTION_S3_PARTIAL'})",
        "label=$Label", "reason=$Reason",
        "created=$((Get-Date).ToString('o'))",
        "source_path=$($Source.FullName)", "backup_path=$dest",
        "bytes=$($Source.Length)", "sha256=$hash", "git_head=$gitHead"
    ) | Set-Content -LiteralPath $receipt -Encoding ascii
    return [pscustomobject]@{
        Source=$Source.FullName; Backup=$dest; Hash=$hash; Receipt=$receipt
    }
}

function Add-ProgressRecord {
    param([string]$Progress, [string]$Checkpoint, [string]$Event,
          [string]$Attempt, [datetime]$Started, [Int64]$RssBytes,
          [double]$AvailableGiB,
          [Text.RegularExpressions.Match]$Marker = $null,
          [string]$KnownSha256 = "")
    $invariant = [Globalization.CultureInfo]::InvariantCulture
    $progressArgs = @(
        $progressHelper, $Checkpoint, "--progress", $Progress,
        "--parent", $layer5Path, "--event", $Event,
        "--session", "$session",
        "--git-head", $gitHead, "--observed-at", (Get-Date).ToString('o'),
        "--elapsed-seconds",
            (((Get-Date)-$Started).TotalSeconds).ToString("F3", $invariant),
        "--rss-bytes", "$RssBytes", "--rss-gib",
            ($RssBytes/1GB).ToString("F6", $invariant),
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
    $out = @(& python @progressArgs 2>&1)
    if ($LASTEXITCODE -ne 0) {
        throw "progress sidecar failed: $($out -join ' ')"
    }
}

function Invoke-FinalAttempt {
    param([string]$Attempt, [string]$Base, [string]$Dump,
          [string]$Directory, [string]$Progress)
    Wait-StartMemory $Attempt
    New-Item -ItemType Directory -Force -Path $Directory | Out-Null
    $stdout = Join-Path $Directory "stdout.log"
    $stderr = Join-Path $Directory "stderr.log"
    $rssLog = Join-Path $Directory "rss.csv"
    $attemptDump = Join-Path $Directory "final.csv"
    if (Test-Path -LiteralPath $attemptDump) {
        throw "$Attempt attempt output path already exists: $attemptDump"
    }
    "timestamp,pid,rss_bytes,rss_gib,available_gib" |
        Set-Content -LiteralPath $rssLog -Encoding ascii
    $haveGeneration = (Test-Path -LiteralPath ($Base + ".a")) -or
                      (Test-Path -LiteralPath ($Base + ".b"))
    $engineArgs = @(
        "6", "--threads", "$Threads", "--caps", $Caps,
        "--ckpt-chunk", "$ChunkParents", "--checkpoint", $Base,
        "$CheckpointMinutes", "--dump", $attemptDump, "--ack-full-c6"
    )
    if ($haveGeneration) {
        $before = Get-LatestImage $Base
        $beforeBackup = Backup-Artifact $before "$Attempt-partial" `
            "pre-resume coverage"
        $engineArgs += @("--resume", $Base)
    } else {
        $beforeBackup = $null
        $engineArgs += @("--load-layer", "5", $layer5Path)
    }
    $commandPath = Join-Path $Directory "command.txt"
    @(
        "git_head=$gitHead",
        "attempt=$Attempt",
        "created=$((Get-Date).ToString('o'))",
        "command=$exe $($engineArgs -join ' ')"
    ) | Set-Content -LiteralPath $commandPath -Encoding ascii
    $started = Get-Date
    if ($beforeBackup) {
        Add-ProgressRecord $Progress $beforeBackup.Source "resume_baseline" `
            $Attempt $started 0 (Get-AvailableRamGiB) $null $beforeBackup.Hash
    }
    $proc = Start-Process -FilePath $exe -ArgumentList $engineArgs `
        -RedirectStandardOutput $stdout -RedirectStandardError $stderr `
        -PassThru -WindowStyle Hidden
    [Int64]$peak = 0
    [Int64]$lastRss = 0
    $available = Get-AvailableRamGiB
    $processed = 0
    $guardReason = ""
    $progressFailure = ""
    try {
        while (!$proc.HasExited) {
            Start-Sleep -Seconds 10
            $proc.Refresh()
            if ($proc.HasExited) { break }
            $live = Get-Process -Id $proc.Id -ErrorAction SilentlyContinue
            if ($null -eq $live) { break }
            [Int64]$lastRss = $live.WorkingSet64
            if ($lastRss -gt $peak) { $peak = $lastRss }
            $available = Get-AvailableRamGiB
            $rssGiB = [Math]::Round($lastRss/1GB,3)
            "$((Get-Date).ToString('o')),$($proc.Id),$lastRss,$rssGiB,$available" |
                Add-Content -LiteralPath $rssLog
            $markers = @(Get-CheckpointMarkers $stdout)
            try {
                for ($i=$processed; $i -lt $markers.Count; $i++) {
                    $checkpoint = $markers[$i].Groups[5].Value.Trim()
                    if ($checkpoint -notin @(
                            ($Base + ".a"),
                            ($Base + ".b")
                        )) {
                        throw "unexpected checkpoint path $checkpoint"
                    }
                    Add-ProgressRecord $Progress $checkpoint "checkpoint" `
                        $Attempt $started $lastRss $available $markers[$i]
                }
                $processed = $markers.Count
            }
            catch {
                $progressFailure = $_.Exception.Message
                $guardReason = "progress_sidecar_failure"
                Stop-Process -Id $proc.Id -Force; break
            }
            if ($rssGiB -gt $RssLimitGB) {
                $guardReason="rss_guard"; Stop-Process -Id $proc.Id -Force; break
            }
            if ($available -lt $MinimumAvailableGB) {
                $guardReason="available_ram_guard"
                Stop-Process -Id $proc.Id -Force; break
            }
            if (((Get-Date)-$started).TotalHours -ge $MaxHoursPerAttempt) {
                $guardReason="hard_time_guard"; Stop-Process -Id $proc.Id -Force; break
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
            for ($i=$processed; $i -lt $markers.Count; $i++) {
                $checkpoint = $markers[$i].Groups[5].Value.Trim()
                Add-ProgressRecord $Progress $checkpoint "checkpoint" `
                    $Attempt $started $lastRss $available $markers[$i]
            }
        }
        catch { $progressFailure = $_.Exception.Message }
    }
    if ($guardReason) {
        $latest = Get-LatestImage $Base
        if ($latest) {
            $null = Backup-Artifact $latest "$Attempt-partial" $guardReason
        }
        throw "$Attempt stopped safely: $guardReason $progressFailure"
    }
    $proc.WaitForExit(); $proc.Refresh(); $exitCode = $proc.ExitCode
    $terminal = Get-Content -Raw -LiteralPath $stdout
    if ($null -eq $exitCode) {
        $exitCode = if ($terminal -match "complete classes = 63199") {0} else {97}
    }
    if ($exitCode -ne 0 -or (Get-Item -LiteralPath $stderr).Length -ne 0 -or
        $terminal -notmatch "C=6 G1 bridge F=6986348258918400: OK" -or
        $terminal -notmatch "C=6 G2 bridge F=7053808087203840: OK" -or
        $terminal -notmatch "complete classes = 63199" -or
        !(Test-Path -LiteralPath $attemptDump) -or
        !(Test-Path -LiteralPath ($Base + ".L6.snap"))) {
        throw "$Attempt final contraction did not pass its terminal gates"
    }
    if (Test-Path -LiteralPath $Dump) {
        throw "$Attempt stable CSV path unexpectedly exists: $Dump"
    }
    Move-Item -LiteralPath $attemptDump -Destination $Dump
    $finalImage = Get-Item -LiteralPath ($Base + ".L6.snap")
    $backup = Backup-Artifact $finalImage $Attempt "closed final wide snapshot"
    return [pscustomobject]@{
        Attempt=$Attempt; PeakGiB=[Math]::Round($peak/1GB,3)
        Hours=[Math]::Round(((Get-Date)-$started).TotalHours,4)
        Dump=$Dump; Snapshot=$finalImage.FullName
        SnapshotSha256=$backup.Hash; SnapshotBackup=$backup.Backup
        Command="$exe $($engineArgs -join ' ')"; CommandRecord=$commandPath
    }
}

$checkpointRoot = Join-Path $root "data/checkpoints"
$logParent = Join-Path $root "data/logs"
$primaryBasePath = Assert-PathUnder $PrimaryBase $checkpointRoot "PrimaryBase"
$replayBasePath = Assert-PathUnder $ReplayBase $checkpointRoot "ReplayBase"
$logRootPath = Assert-PathUnder $LogRoot $logParent "LogRoot"
$externalRoot = "D:\sudoku_FJ_checkpoint_backups"
$externalPath = Assert-PathUnder $ExternalBackupDir $externalRoot `
    "ExternalBackupDir"
foreach ($base in @($primaryBasePath,$replayBasePath)) {
    if ([IO.Path]::GetFileName($base) -notlike "layer_dp_c6_s3_prod_*") {
        throw "S3 checkpoint basenames must use layer_dp_c6_s3_prod_*"
    }
}
if ([IO.Path]::GetPathRoot($primaryBasePath) -eq
    [IO.Path]::GetPathRoot($externalPath)) {
    throw "external backup must be on a different volume"
}

$dirtyTracked = @(& git status --porcelain --untracked-files=no)
if ($LASTEXITCODE -ne 0 -or $dirtyTracked.Count) {
    throw "production S3 requires a clean tracked worktree"
}
$gitHead = (& git rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0) { throw "cannot resolve Git HEAD" }
if (Get-Process -Name "layer_dp_gate" -ErrorAction SilentlyContinue) {
    throw "another layer_dp_gate process is already running"
}
$layer5Path = (Resolve-Path -LiteralPath $Layer5).Path
$layer5BackupPath = (Resolve-Path -LiteralPath $Layer5Backup).Path
if ((Get-Sha256 $layer5Path) -ne $Layer5Sha256 -or
    (Get-Sha256 $layer5BackupPath) -ne $Layer5Sha256) {
    throw "local/external L5 hashes do not match -Layer5Sha256"
}
New-Item -ItemType Directory -Force -Path $logRootPath,$externalPath | Out-Null
$progressHelper = (Resolve-Path -LiteralPath "scripts/layer_dp_progress.py").Path
$certificate = (Resolve-Path -LiteralPath `
    "experiments/proto/s4_certificate_verify.py").Path
$pythonSum = (Resolve-Path -LiteralPath `
    "experiments/proto/s4_exact_sum.py").Path
$powerShellSum = (Resolve-Path -LiteralPath `
    "experiments/proto/s4_exact_sum.ps1").Path

$session = 1
while (Test-Path -LiteralPath (Join-Path $logRootPath (
        "session-{0:D3}" -f $session))) { $session++ }
$sessionDir = Join-Path $logRootPath ("session-{0:D3}" -f $session)
New-Item -ItemType Directory -Path $sessionDir | Out-Null
$gateOut=Join-Path $sessionDir "verify_all.stdout.log"
$gateErr=Join-Path $sessionDir "verify_all.stderr.log"
$gateArgs=@("-NoProfile","-ExecutionPolicy","Bypass","-File",
            (Join-Path $PSScriptRoot "verify_all.ps1"))
if ((Invoke-LoggedProcess "powershell.exe" $gateArgs $gateOut $gateErr `
        "ALL REPOSITORY CHECKS PASSED") -ne 0) {
    throw "complete repository gate failed"
}
$buildOut=Join-Path $sessionDir "build.log"
$buildErr=Join-Path $sessionDir "build.stderr.log"
$buildArgs=@("-NoProfile","-ExecutionPolicy","Bypass","-File",
             (Join-Path $PSScriptRoot "build_layer_dp.ps1"))
if ((Invoke-LoggedProcess "powershell.exe" $buildArgs $buildOut $buildErr `
        "built layer-DP gate engine") -ne 0) { throw "layer-DP build failed" }
$exe=(Resolve-Path -LiteralPath "build/layer_dp_gate.exe").Path
$preflightOut=Join-Path $sessionDir "resource-preflight.log"
$preflightErr=Join-Path $sessionDir "resource-preflight.stderr.log"
$preflightArgs=@("6","--threads","$Threads","--caps",$Caps,
    "--checkpoint",$primaryBasePath,"$CheckpointMinutes",
    "--resource-preflight","5")
if ((Invoke-LoggedProcess $exe $preflightArgs $preflightOut $preflightErr `
        "RESOURCE PREFLIGHT PASS") -ne 0) { throw "S3 preflight failed" }
if ($PrepareOnly) {
    Write-Host "PRODUCTION S3 PREPARATION PASSED (no checkpoint or CSV written)"
    exit 0
}

$primaryDump = Join-Path $logRootPath "final-primary.csv"
$replayDump = Join-Path $logRootPath "final-replay.csv"
$primaryProgress = Join-Path $logRootPath "progress-primary.csv"
$replayProgress = Join-Path $logRootPath "progress-replay.csv"
$allArtifacts = @(
    $primaryDump,
    $replayDump,
    ($primaryBasePath + ".a"),
    ($primaryBasePath + ".b"),
    ($primaryBasePath + ".L6.snap"),
    ($replayBasePath + ".a"),
    ($replayBasePath + ".b"),
    ($replayBasePath + ".L6.snap")
)
if (!$ContinueExisting -and ($allArtifacts | Where-Object {
        Test-Path -LiteralPath $_ }).Count) {
    throw "S3 artifacts already exist; use -ContinueExisting"
}

$primaryResult = $null
if (!(Test-Path -LiteralPath $primaryDump)) {
    $primaryResult = Invoke-FinalAttempt "primary" $primaryBasePath `
        $primaryDump (Join-Path $sessionDir "primary") $primaryProgress
} elseif (!(Test-Path -LiteralPath ($primaryBasePath+".L6.snap"))) {
    throw "primary CSV exists without its final wide snapshot"
}
$replayResult = $null
if (!(Test-Path -LiteralPath $replayDump)) {
    $replayResult = Invoke-FinalAttempt "replay" $replayBasePath `
        $replayDump (Join-Path $sessionDir "replay") $replayProgress
} elseif (!(Test-Path -LiteralPath ($replayBasePath+".L6.snap"))) {
    throw "replay CSV exists without its final wide snapshot"
}

$primaryHash = Get-Sha256 $primaryDump
$replayHash = Get-Sha256 $replayDump
if ($primaryHash -ne $replayHash) { throw "S3 replay CSV SHA-256 mismatch" }

$certificateLog = Join-Path $sessionDir "certificate.log"
& python $certificate $primaryDump "--c" "6" "--classes" "63199" `
    "--expect-n" $ExpectedN "--quiet" *> $certificateLog
if ($LASTEXITCODE -ne 0) { throw "semantic certificate verification failed" }
$certificateText = Get-Content -Raw -LiteralPath $certificateLog
if ($certificateText -notmatch "CERTIFICATE PASS") {
    throw "certificate verifier omitted its PASS marker"
}

$pythonSumLog = Join-Path $sessionDir "sum-python.log"
& python $pythonSum $primaryDump "--classes" "63199" `
    "--expect-f" "6986348258918400,7053808087203840" `
    "--expect-n" $ExpectedN *> $pythonSumLog
if ($LASTEXITCODE -ne 0) { throw "Python exact sum failed" }
$powerShellSumLog = Join-Path $sessionDir "sum-powershell.log"
& powershell -NoProfile -ExecutionPolicy Bypass -File $powerShellSum `
    $primaryDump -Classes 63199 -ExpectN $ExpectedN *> $powerShellSumLog
if ($LASTEXITCODE -ne 0) { throw "PowerShell BigInteger exact sum failed" }
$pythonText = Get-Content -Raw -LiteralPath $pythonSumLog
$powerShellText = Get-Content -Raw -LiteralPath $powerShellSumLog
if ($pythonText -notmatch "N\(6\) = ([0-9]+)") {
    throw "cannot parse Python exact sum"
}
$pythonN = $Matches[1]
if ($powerShellText -notmatch "N\(6\) = ([0-9]+)") {
    throw "cannot parse PowerShell exact sum"
}
$powerShellN = $Matches[1]
if ($pythonN -ne $powerShellN -or $pythonN -ne $ExpectedN) {
    throw "independent exact sums disagree"
}

$externalCsv = Join-Path $externalPath "layer_dp_c6_final.csv"
$externalCsvReceipt = $externalCsv + ".sha256.txt"
if (!(Test-Path -LiteralPath $externalCsv)) {
    $tempCsv = $externalCsv + ".copying"
    if (Test-Path -LiteralPath $tempCsv) {
        throw "unfinished external CSV copy exists"
    }
    Copy-Item -LiteralPath $primaryDump -Destination $tempCsv
    if ((Get-Sha256 $tempCsv) -ne $primaryHash) {
        throw "external CSV SHA-256 mismatch"
    }
    Move-Item -LiteralPath $tempCsv -Destination $externalCsv
    @(
        "mode=C6_FINAL_CERTIFICATE_CSV", "created=$((Get-Date).ToString('o'))",
        "source_path=$primaryDump", "backup_path=$externalCsv",
        "bytes=$((Get-Item -LiteralPath $primaryDump).Length)",
        "sha256=$primaryHash", "git_head=$gitHead",
        "python_sum=$pythonN", "powershell_sum=$powerShellN"
    ) | Set-Content -LiteralPath $externalCsvReceipt -Encoding ascii
} else {
    if ((Get-Sha256 $externalCsv) -ne $primaryHash) {
        throw "existing external final CSV has a different hash"
    }
    if (!(Test-Path -LiteralPath $externalCsvReceipt) -or
        (Get-Content -Raw -LiteralPath $externalCsvReceipt) -notmatch
            "(?m)^sha256=$primaryHash\r?$") {
        throw "existing external final CSV lacks a matching receipt"
    }
}

if ((Get-Sha256 $layer5Path) -ne $Layer5Sha256 -or
    (Get-Sha256 $layer5BackupPath) -ne $Layer5Sha256) {
    throw "local or external immutable L5 changed during S3"
}
if (Get-Process -Name "layer_dp_gate" -ErrorAction SilentlyContinue) {
    throw "a layer_dp_gate process remains after S3"
}

$semanticHash = if ($certificateText -match
    "semantic_sha256 = ([0-9A-F]{64})") { $Matches[1] } else { "UNPARSED" }
$commandFiles = @(Get-ChildItem -LiteralPath $logRootPath -Recurse `
    -Filter "command.txt" -File | Sort-Object FullName)
$commandText = @($commandFiles | ForEach-Object {
    Get-Content -Raw -LiteralPath $_.FullName
}) -join "`n"
if ($commandFiles.Count -lt 2 -or
    $commandText -notmatch "(?m)^attempt=primary\r?$" -or
    $commandText -notmatch "(?m)^attempt=replay\r?$") {
    throw "final certificate lacks both exact engine command records"
}
$commandRecords = @($commandFiles | ForEach-Object { $_.FullName }) -join ';'
$summary = @(
    "mode=C6_PRODUCTION_S3_FINALIZED", "git_head=$gitHead",
    "layer5_sha256=$Layer5Sha256", "primary_csv=$primaryDump",
    "replay_csv=$replayDump", "csv_sha256=$primaryHash",
    "semantic_sha256=$semanticHash", "classes=63199",
    "G1_F=6986348258918400", "G2_F=7053808087203840",
    "python_sum=$pythonN", "powershell_sum=$powerShellN",
    "expected_n=$ExpectedN", "external_csv=$externalCsv",
    "external_csv_receipt=$externalCsvReceipt",
    "command_records=$commandRecords",
    "primary_command=$($primaryResult.Command)",
    "replay_command=$($replayResult.Command)"
)
$summaryPath = Join-Path $sessionDir "summary.txt"
$summary | Set-Content -LiteralPath $summaryPath -Encoding ascii
Write-Host "PRODUCTION S3 CERTIFICATE CLOSED"
Write-Host "CSV SHA-256: $primaryHash"
Write-Host "N(6): $pythonN"
Write-Host "summary: $summaryPath"
