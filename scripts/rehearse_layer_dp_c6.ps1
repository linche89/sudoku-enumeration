param(
    [int]$Threads = 24,
    [ValidateRange(2, 1000000000)]
    [UInt64]$Denom = 100,
    [string]$Caps = "2000,14000000,1350000000,250000000,100000",
    [UInt64]$ChunkParents = 100000,
    [double]$CheckpointMinutes = 10.0,
    [int]$RssLimitGB = 85,
    [int]$MaxStageMinutes = 240,
    [string]$Layer3 = "data/checkpoints/layer_dp_c6_layer3_20260731.snap",
    [string]$Layer3Backup = `
        "D:\sudoku_FJ_checkpoint_backups\layer_dp_c6_layer3_20260731.snap",
    [string]$RunDir = ""
)

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
Set-Location $root

if ($Threads -lt 2) { throw "Threads must be at least 2" }
if ($CheckpointMinutes -le 0) {
    throw "CheckpointMinutes must be positive for the interrupted rehearsal"
}
if ($RssLimitGB -lt 1 -or $MaxStageMinutes -lt 1) {
    throw "RSS and time guards must be positive"
}

function Get-Sha256 {
    param([string]$Path)
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

function Invoke-GuardedLayer {
    param(
        [string]$Label,
        [string[]]$Arguments,
        [string]$Directory,
        [string]$KillAfterCheckpointBase = ""
    )

    New-Item -ItemType Directory -Force -Path $Directory | Out-Null
    $stdout = Join-Path $Directory "stdout.log"
    $stderr = Join-Path $Directory "stderr.log"
    $rssLog = Join-Path $Directory "rss.csv"
    "timestamp,pid,rss_bytes,rss_gib" | Set-Content -LiteralPath $rssLog

    Write-Host "== $Label =="
    Write-Host "starting: $exe $($Arguments -join ' ')"
    $proc = Start-Process -FilePath $exe -ArgumentList $Arguments `
        -RedirectStandardOutput $stdout -RedirectStandardError $stderr `
        -PassThru -WindowStyle Hidden
    $started = Get-Date
    $lastReport = $started.AddMinutes(-1)
    [Int64]$peak = 0
    $forced = $false

    try {
        while (!$proc.HasExited) {
            Start-Sleep -Seconds 2
            $proc.Refresh()
            if ($proc.HasExited) { break }
            $live = Get-Process -Id $proc.Id -ErrorAction SilentlyContinue
            if ($null -eq $live) { break }
            [Int64]$rss = $live.WorkingSet64
            if ($rss -gt $peak) { $peak = $rss }
            $gib = [Math]::Round($rss / 1GB, 3)
            "$((Get-Date).ToString('o')),$($proc.Id),$rss,$gib" |
                Add-Content -LiteralPath $rssLog
            $elapsed = ((Get-Date) - $started).TotalMinutes
            if (((Get-Date) - $lastReport).TotalSeconds -ge 30) {
                Write-Host ("{0}: elapsed={1:N1} min rss={2:N3} GiB" -f `
                    $Label, $elapsed, $gib)
                $lastReport = Get-Date
            }
            if ($rss -gt ([Int64]$RssLimitGB * 1GB)) {
                Stop-Process -Id $proc.Id -Force
                throw "$Label exceeded the ${RssLimitGB}-GiB RSS guard"
            }
            if ($elapsed -ge $MaxStageMinutes) {
                Stop-Process -Id $proc.Id -Force
                throw "$Label exceeded the ${MaxStageMinutes}-minute guard"
            }
            if ($KillAfterCheckpointBase) {
                $haveGeneration =
                    (Test-Path -LiteralPath ($KillAfterCheckpointBase + ".a")) -or
                    (Test-Path -LiteralPath ($KillAfterCheckpointBase + ".b"))
                $haveMarker = $false
                if ($haveGeneration -and (Test-Path -LiteralPath $stdout)) {
                    try {
                        $haveMarker = (Get-Content -Raw -LiteralPath $stdout) `
                            -match "checkpoint gen="
                    }
                    catch { $haveMarker = $false }
                }
                if ($haveGeneration -and $haveMarker) {
                    Write-Host "${Label}: durable checkpoint observed; forcing interruption"
                    Stop-Process -Id $proc.Id -Force
                    $forced = $true
                    break
                }
            }
        }
    }
    finally {
        Wait-Process -Id $proc.Id -Timeout 30 -ErrorAction SilentlyContinue
        $proc.Refresh()
    }

    if ($forced) {
        if ((Test-Path -LiteralPath $stderr) -and
            (Get-Item -LiteralPath $stderr).Length -ne 0) {
            Get-Content -LiteralPath $stderr
            throw "$Label wrote stderr before the deliberate interruption"
        }
        return [pscustomobject]@{
            Forced = $true
            ExitCode = $proc.ExitCode
            PeakGiB = [Math]::Round($peak / 1GB, 3)
            Minutes = [Math]::Round(((Get-Date) - $started).TotalMinutes, 3)
            Stdout = $stdout
        }
    }

    $proc.WaitForExit()
    $proc.Refresh()
    $exitCode = $proc.ExitCode
    # Windows PowerShell can expose a null ExitCode after a redirected child
    # has already been reaped.  This is the same benign case handled by
    # watch_rss.ps1; non-empty stderr and required downstream artifacts still
    # fail closed.
    if ($null -eq $exitCode) { $exitCode = 0 }
    if ($exitCode -ne 0) {
        Get-Content -LiteralPath $stdout -ErrorAction SilentlyContinue
        Get-Content -LiteralPath $stderr -ErrorAction SilentlyContinue
        throw "$Label failed with exit code $exitCode"
    }
    if ((Get-Item -LiteralPath $stderr).Length -ne 0) {
        Get-Content -LiteralPath $stderr
        throw "$Label completed with non-empty stderr"
    }
    return [pscustomobject]@{
        Forced = $false
        ExitCode = $exitCode
        PeakGiB = [Math]::Round($peak / 1GB, 3)
        Minutes = [Math]::Round(((Get-Date) - $started).TotalMinutes, 3)
        Stdout = $stdout
    }
}

& "$PSScriptRoot/build_layer_dp.ps1"
if ($LASTEXITCODE -ne 0) { throw "layer-DP build failed" }
$exe = (Resolve-Path -LiteralPath "build/layer_dp_gate.exe").Path
$s4 = (Resolve-Path -LiteralPath "experiments/proto/s4_exact_sum.py").Path
$layer3Path = (Resolve-Path -LiteralPath $Layer3).Path
$backupPath = (Resolve-Path -LiteralPath $Layer3Backup).Path

$expectedLayer3Hash =
    "1D882DB7B0B18490E981539F7C690FA90227ECEC243817F5B538BEC8DAC865B7"
$beforeLayer3Hash = Get-Sha256 $layer3Path
$beforeBackupHash = Get-Sha256 $backupPath
if ($beforeLayer3Hash -ne $expectedLayer3Hash -or
    $beforeBackupHash -ne $expectedLayer3Hash) {
    throw "active/external layer-3 snapshot hash does not match the manifest"
}

if (!$RunDir) {
    $stamp = Get-Date -Format "yyyyMMdd-HHmmss"
    $RunDir = "data/logs/layer-dp-c6-e2e-rehearsal-$stamp"
}
$runPath = [IO.Path]::GetFullPath((Join-Path $root $RunDir))
$logRoot = [IO.Path]::GetFullPath((Join-Path $root "data/logs"))
if (!$runPath.StartsWith($logRoot, [StringComparison]::OrdinalIgnoreCase)) {
    throw "RunDir must stay under data/logs"
}
if (Test-Path -LiteralPath $runPath) {
    throw "RunDir already exists: $runPath"
}
New-Item -ItemType Directory -Path $runPath | Out-Null

$common = @(
    "6", "--threads", "$Threads", "--caps", $Caps,
    "--rehearsal-denom", "$Denom",
    "--ckpt-chunk", "$ChunkParents"
)

# S1: start from the immutable exact L3 seed, wait for one durable timed
# checkpoint, kill the process, then resume the same transition to closure.
$s1Dir = Join-Path $runPath "s1-3to4"
$s1Base = Join-Path $s1Dir "ck"
$s1FreshArgs = $common + @(
    "--load-layer", "3", $layer3Path,
    "--checkpoint", $s1Base, "$CheckpointMinutes",
    "--stop-after", "4"
)
$s1Killed = Invoke-GuardedLayer "S1 3->4 deliberate interruption" `
    $s1FreshArgs (Join-Path $s1Dir "killed") $s1Base
if (!$s1Killed.Forced) { throw "S1 was not deliberately interrupted" }
if (!(Test-Path -LiteralPath ($s1Base + ".L3.snap"))) {
    throw "S1 checkpoint set is missing its immutable L3 parent snapshot"
}
$s1ResumeArgs = $common + @(
    "--checkpoint", $s1Base, "$CheckpointMinutes",
    "--resume", $s1Base, "--stop-after", "4"
)
$s1Resumed = Invoke-GuardedLayer "S1 3->4 resume to closed rehearsal L4" `
    $s1ResumeArgs (Join-Path $s1Dir "resumed")
$layer4 = $s1Base + ".L4.snap"
if (!(Test-Path -LiteralPath $layer4)) {
    throw "S1 resume did not produce the closed rehearsal L4 snapshot"
}

# S2: a fresh checkpoint namespace and one process for 4->5.
$s2Dir = Join-Path $runPath "s2-4to5"
$s2Base = Join-Path $s2Dir "ck"
$s2Args = $common + @(
    "--load-layer", "4", $layer4,
    "--checkpoint", $s2Base, "$CheckpointMinutes",
    "--stop-after", "5"
)
$s2 = Invoke-GuardedLayer "S2 4->5 closed rehearsal L5" `
    $s2Args $s2Dir
$layer5 = $s2Base + ".L5.snap"
if (!(Test-Path -LiteralPath $layer5)) {
    throw "S2 did not produce the closed rehearsal L5 snapshot"
}

# S3: final sparse contraction and a second independent replay from the same
# immutable L5 image.  The sorted CSVs must be byte-identical.
$s3Dir = Join-Path $runPath "s3-5to6"
$s3Base = Join-Path $s3Dir "ck"
$dump = Join-Path $s3Dir "rehearsal.csv"
$s3Args = $common + @(
    "--load-layer", "5", $layer5,
    "--checkpoint", $s3Base, "$CheckpointMinutes",
    "--dump", $dump
)
$s3 = Invoke-GuardedLayer "S3 5->6 rehearsal contraction" $s3Args $s3Dir

$replayDir = Join-Path $runPath "s3-replay"
$replayBase = Join-Path $replayDir "ck"
$replayDump = Join-Path $replayDir "rehearsal.csv"
$replayArgs = $common + @(
    "--load-layer", "5", $layer5,
    "--checkpoint", $replayBase, "$CheckpointMinutes",
    "--dump", $replayDump
)
$replay = Invoke-GuardedLayer "S3 deterministic replay" `
    $replayArgs $replayDir
$dumpHash = Get-Sha256 $dump
$replayHash = Get-Sha256 $replayDump
if ($dumpHash -ne $replayHash) {
    throw "S3 replay CSV SHA-256 mismatch"
}

# S4: external arbitrary-precision checksum over the visibly watermarked CSV.
$rows = ([IO.File]::ReadLines($dump) | Measure-Object).Count - 1
if ($rows -le 0) { throw "S3 rehearsal CSV has no data rows" }
$s4Log = Join-Path $runPath "s4-external-checksum.log"
& python $s4 $dump "--classes" "$rows" "--rehearsal" *> $s4Log
if ($LASTEXITCODE -ne 0) {
    Get-Content -LiteralPath $s4Log
    throw "S4 external rehearsal checksum failed"
}
$s4Text = Get-Content -Raw -LiteralPath $s4Log
if ($s4Text -notmatch "rehearsal_checksum\(6\) = ([0-9]+).*NOT N\(6\)") {
    Get-Content -LiteralPath $s4Log
    throw "S4 output is missing the watermarked checksum marker"
}
$checksum = $Matches[1]

$afterLayer3Hash = Get-Sha256 $layer3Path
$afterBackupHash = Get-Sha256 $backupPath
if ($afterLayer3Hash -ne $beforeLayer3Hash -or
    $afterBackupHash -ne $beforeBackupHash) {
    throw "active or external L3 snapshot changed during rehearsal"
}
if (Get-Process -Name "layer_dp_gate" -ErrorAction SilentlyContinue) {
    throw "a layer_dp_gate process remains after the rehearsal"
}

$summary = @"
mode=BOUNDED_REHEARSAL_NOT_N6
denominator=$Denom
threads=$Threads
caps=$Caps
chunk_parents=$ChunkParents
checkpoint_minutes=$CheckpointMinutes
rss_limit_gib=$RssLimitGB
max_stage_minutes=$MaxStageMinutes
layer3_sha256_before=$beforeLayer3Hash
layer3_sha256_after=$afterLayer3Hash
backup_sha256_before=$beforeBackupHash
backup_sha256_after=$afterBackupHash
s1_killed_minutes=$($s1Killed.Minutes)
s1_killed_peak_gib=$($s1Killed.PeakGiB)
s1_resume_minutes=$($s1Resumed.Minutes)
s1_resume_peak_gib=$($s1Resumed.PeakGiB)
s2_minutes=$($s2.Minutes)
s2_peak_gib=$($s2.PeakGiB)
s3_minutes=$($s3.Minutes)
s3_peak_gib=$($s3.PeakGiB)
s3_replay_minutes=$($replay.Minutes)
s3_replay_peak_gib=$($replay.PeakGiB)
classes_captured=$rows
csv_sha256=$dumpHash
replay_csv_sha256=$replayHash
rehearsal_checksum=$checksum
"@
$summaryPath = Join-Path $runPath "summary.txt"
$summary | Set-Content -LiteralPath $summaryPath -Encoding ascii

Write-Host "BOUNDED INTERRUPTED END-TO-END REHEARSAL PASSED"
Write-Host "artifact directory: $runPath"
Write-Host "captured classes: $rows"
Write-Host "CSV SHA-256: $dumpHash"
Write-Host "rehearsal checksum (NOT N(6)): $checksum"
