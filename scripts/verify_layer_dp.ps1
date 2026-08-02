param(
    [int]$Threads = 8,
    [int]$KillIterations = 2,
    [switch]$KeepArtifacts
)

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
Set-Location $root

if ($Threads -lt 2) { throw "Threads must be at least 2" }
if ($KillIterations -lt 1) { throw "KillIterations must be positive" }

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

& "$PSScriptRoot/build_layer_dp.ps1"
if ($LASTEXITCODE -ne 0) { throw "layer-DP build failed" }

$exe = (Resolve-Path -LiteralPath "build/layer_dp_gate.exe").Path
$penultimateExe = (Resolve-Path -LiteralPath `
    "build/layer_penultimate_burnside.exe").Path
$ref = (Resolve-Path -LiteralPath `
    "docs/expert/2026-07-21/native_c5_response_quotient_triples.csv").Path
$expectedRefHash = "D2FDEB354ED4C1443E9870B5727CE35C88BA6B392C6DAA32CD92D2601BCDB1F5"
if ((Get-Sha256 $ref) -ne $expectedRefHash) {
    throw "C=5 reference triple fixture hash mismatch"
}
$s4 = (Resolve-Path -LiteralPath "experiments/proto/s4_exact_sum.py").Path
$caps = "200,20000,20000,600"
$expectedN = "1903816047972624930994913280000"
$tempRoot = [IO.Path]::GetFullPath([IO.Path]::GetTempPath())
$work = Join-Path $tempRoot ("sudoku-layer-dp-gate-" + [guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Path $work | Out-Null
$succeeded = $false

function Invoke-Layer {
    param(
        [string]$Label,
        [string[]]$Arguments,
        [string]$Log
    )
    Write-Host "== $Label =="
    & $exe @Arguments *> $Log
    if ($LASTEXITCODE -ne 0) {
        Get-Content -LiteralPath $Log
        throw "$Label failed with exit code $LASTEXITCODE"
    }
}

function Assert-FileEqual {
    param([string]$Expected, [string]$Actual, [string]$Label)
    $a = Get-Sha256 $Expected
    $b = Get-Sha256 $Actual
    if ($a -ne $b) { throw "$Label SHA-256 mismatch" }
}

function Invoke-ExpectExit {
    param(
        [string]$Label,
        [string[]]$Arguments,
        [int]$ExpectedExit,
        [string]$Dir
    )
    Write-Host "== $Label =="
    $stdout = Join-Path $Dir ($Label.Replace(" ", "-") + ".out")
    $stderr = Join-Path $Dir ($Label.Replace(" ", "-") + ".err")
    $proc = Start-Process -FilePath $exe -ArgumentList $Arguments `
        -RedirectStandardOutput $stdout -RedirectStandardError $stderr `
        -PassThru -Wait -WindowStyle Hidden
    if ($proc.ExitCode -ne $ExpectedExit) {
        Get-Content -LiteralPath $stdout -ErrorAction SilentlyContinue
        Get-Content -LiteralPath $stderr -ErrorAction SilentlyContinue
        throw "$Label exited $($proc.ExitCode), expected $ExpectedExit"
    }
}

function Start-Kill-And-Resume {
    param(
        [string]$Dir,
        [int]$LoadLayer,
        [string]$LoadPath,
        [string]$GoldenDump,
        [switch]$ForceWide,
        [int]$ChunkParents = 500,
        [int]$ExtraDelayMs = 100,
        [switch]$RequireBothGenerations,
        [int]$LockGenerationMs = 0
    )
    New-Item -ItemType Directory -Path $Dir | Out-Null
    $base = Join-Path $Dir "ck"
    $dump = Join-Path $Dir "dump.csv"
    $stdout = Join-Path $Dir "killed.out"
    $stderr = Join-Path $Dir "killed.err"
    $args = @(
        "5", "--threads", "$Threads", "--caps", $caps,
        "--ref", $ref, "--dump", $dump,
        "--load-layer", "$LoadLayer", $LoadPath,
        "--checkpoint", $base, "0",
        "--ckpt-chunk", "$ChunkParents"
    )
    if ($ForceWide) { $args += "--force-wide" }

    $proc = Start-Process -FilePath $exe -ArgumentList $args `
        -RedirectStandardOutput $stdout -RedirectStandardError $stderr `
        -PassThru -WindowStyle Hidden
    $deadline = (Get-Date).AddSeconds(30)
    $sawCheckpoint = $false
    while ((Get-Date) -lt $deadline) {
        Start-Sleep -Milliseconds 25
        $proc.Refresh()
        $haveA = Test-Path -LiteralPath ($base + ".a")
        $haveB = Test-Path -LiteralPath ($base + ".b")
        if (($RequireBothGenerations -and $haveA -and $haveB) -or
            (!$RequireBothGenerations -and ($haveA -or $haveB))) {
            $sawCheckpoint = $true
            break
        }
        if ($proc.HasExited) { break }
    }
    if (!$sawCheckpoint) {
        if (!$proc.HasExited) { Stop-Process -Id $proc.Id -Force }
        throw "checkpoint was not produced before timeout"
    }
    Start-Sleep -Milliseconds $ExtraDelayMs
    $proc.Refresh()
    if ($proc.HasExited) {
        throw "test process completed before forced kill"
    }
    Stop-Process -Id $proc.Id -Force
    Wait-Process -Id $proc.Id -Timeout 10 -ErrorAction SilentlyContinue

    $parentSnap = "$base.L$LoadLayer.snap"
    if (!(Test-Path -LiteralPath $parentSnap)) {
        throw "self-contained parent snapshot was not created"
    }

    $resumeArgs = @(
        "5", "--threads", "$Threads", "--caps", $caps,
        "--ref", $ref, "--dump", $dump,
        "--checkpoint", $base, "0",
        "--ckpt-chunk", "$ChunkParents",
        "--resume", $base
    )
    if ($ForceWide) { $resumeArgs += "--force-wide" }
    $resumeLog = Join-Path $Dir "resume.log"
    if ($LockGenerationMs -gt 0) {
        $locks = @()
        foreach ($generation in @("$base.a", "$base.b")) {
            if (Test-Path -LiteralPath $generation) {
                $locks += [IO.File]::Open(
                    $generation, [IO.FileMode]::Open,
                    [IO.FileAccess]::Read, [IO.FileShare]::Read)
            }
        }
        $resumeErr = Join-Path $Dir "resume.err"
        $retryObserved = $false
        try {
            Write-Host "== resume layer $LoadLayer with locked generations =="
            $resumeProc = Start-Process -FilePath $exe `
                -ArgumentList $resumeArgs `
                -RedirectStandardOutput $resumeLog `
                -RedirectStandardError $resumeErr `
                -PassThru -WindowStyle Hidden
            $lockDeadline = (Get-Date).AddMilliseconds($LockGenerationMs)
            while ((Get-Date) -lt $lockDeadline) {
                Start-Sleep -Milliseconds 25
                $resumeProc.Refresh()
                if ((Test-Path -LiteralPath $resumeErr) -and
                    (Get-Content -Raw -LiteralPath $resumeErr) -match
                    "replace temporarily blocked") {
                    $retryObserved = $true
                    break
                }
                if ($resumeProc.HasExited) { break }
            }
        }
        finally {
            foreach ($lock in $locks) { $lock.Dispose() }
        }
        if (!$retryObserved) {
            if (!$resumeProc.HasExited) {
                Stop-Process -Id $resumeProc.Id -Force
                $resumeProc.WaitForExit()
            }
            Get-Content -LiteralPath $resumeLog -ErrorAction SilentlyContinue
            Get-Content -LiteralPath $resumeErr -ErrorAction SilentlyContinue
            throw "locked-generation test did not exercise rename retry"
        }
        if (!$resumeProc.WaitForExit(60000)) {
            Stop-Process -Id $resumeProc.Id -Force
            $resumeProc.WaitForExit()
            throw "locked-generation resume timed out"
        }
        $resumeProc.WaitForExit()
        $resumeProc.Refresh()
        $resumeExit = $resumeProc.ExitCode
        if ($null -ne $resumeExit -and $resumeExit -ne 0) {
            Get-Content -LiteralPath $resumeLog -ErrorAction SilentlyContinue
            Get-Content -LiteralPath $resumeErr -ErrorAction SilentlyContinue
            throw "locked-generation resume failed with exit code $resumeExit"
        }
        if ((Get-Content -Raw -LiteralPath $resumeLog) -notmatch
            "ALL GATES PASSED for C=5") {
            throw "locked-generation resume did not reach the success marker"
        }
    } else {
        Invoke-Layer "resume layer $LoadLayer" $resumeArgs $resumeLog
    }
    if ((Get-Content -Raw -LiteralPath $resumeLog) -notmatch
        "resume emission accounting") {
        throw "resume log omitted per-invocation emission accounting"
    }
    Assert-FileEqual $GoldenDump $dump "resumed class dump"
    return $base
}

try {
    Write-Host "== exact penultimate-layer Burnside count =="
    $penultimateLog = Join-Path $work "penultimate-burnside.log"
    & $penultimateExe *> $penultimateLog
    if ($LASTEXITCODE -ne 0) {
        Get-Content -LiteralPath $penultimateLog
        throw "penultimate-layer Burnside counter failed"
    }
    $penultimateText = Get-Content -Raw -LiteralPath $penultimateLog
    if ($penultimateText -notmatch "M_1=1 expected=1 \[OK\]" -or
        $penultimateText -notmatch "M_2=5 expected=5 \[OK\]" -or
        $penultimateText -notmatch "M_3=54 expected=54 \[OK\]" -or
        $penultimateText -notmatch "M_4=17120 expected=17120 \[OK\]" -or
        $penultimateText -notmatch "M_5=96452755" -or
        $penultimateText -notmatch
            "PENULTIMATE-LAYER BURNSIDE GATES PASSED") {
        Get-Content -LiteralPath $penultimateLog
        throw "penultimate-layer Burnside output failed its exact gate"
    }

    $resourceDir = Join-Path $work "resource"
    New-Item -ItemType Directory -Path $resourceDir | Out-Null
    $resourceBase = Join-Path $resourceDir "ck"
    $resourceLog = Join-Path $resourceDir "preflight.log"
    Write-Host "== resource preflight dry run =="
    & $exe "5" "--threads" "$Threads" "--caps" $caps `
        "--force-wide" "--checkpoint" $resourceBase "0" `
        "--resource-preflight" "4" *> $resourceLog
    if ($LASTEXITCODE -notin @(0, 13)) {
        Get-Content -LiteralPath $resourceLog
        throw "resource preflight exited $LASTEXITCODE"
    }
    $resourceText = Get-Content -Raw -LiteralPath $resourceLog
    if ($resourceText -notmatch "cap-reserved image moved in place" -or
        $resourceText -notmatch "active A\+B\+tmp" -or
        $resourceText -notmatch "RESOURCE PREFLIGHT (PASS|FAIL)") {
        throw "resource preflight did not report the required RAM/disk model"
    }
    if (Get-ChildItem -LiteralPath $resourceDir -Filter "ck*" |
        Select-Object -First 1) {
        throw "resource preflight created checkpoint files"
    }

    Invoke-Layer "C=2 exact + canon" `
        @("2", "--scan-check", "200") (Join-Path $work "c2.log")
    Invoke-Layer "C=3 exact + canon" `
        @("3", "--scan-check", "400") (Join-Path $work "c3.log")
    Invoke-Layer "C=4 threaded exact + canon" `
        @("4", "--threads", "$Threads", "--caps", "60,120,60",
          "--invariance", "1000", "--scan-check", "1000") `
        (Join-Path $work "c4.log")

    $sampleLog = Join-Path $work "m4-random-sample.log"
    Invoke-Layer "key-hash M4 and 4-to-5 calibration" `
        @("5", "--threads", "$Threads", "--caps", $caps,
          "--m4probe", "2000", "2", "6000",
          "--m4-parent-seed", "20260731",
          "--fan-sample", "200",
          "--fan-canon-sample", "50", "5000") $sampleLog
    $sampleText = Get-Content -Raw -LiteralPath $sampleLog
    if ($sampleText -notmatch
            "parent_sample=canonical-key-hash seed=20260731" -or
        $sampleText -notmatch
            "m4probe fan 4->5: sample=canonical-key-hash" -or
        $sampleText -notmatch
            "m4probe canonical 4->5: sample=canonical-key-hash") {
        throw "key-hash M4/fan calibration output failed its C=5 gate"
    }

    $gold = Join-Path $work "gold.csv"
    $layer3 = Join-Path $work "clean.L3.snap"
    Invoke-Layer "C=5 full 355-class gate" `
        @("5", "--threads", "$Threads", "--caps", $caps,
          "--ref", $ref, "--invariance", "2000", "--scan-check", "1000",
          "--save-layer", "3", $layer3, "--dump", $gold) `
        (Join-Path $work "c5.log")

    $loadedDump = Join-Path $work "loaded.csv"
    Invoke-Layer "layer snapshot round trip" `
        @("5", "--threads", "$Threads", "--caps", $caps,
          "--ref", $ref, "--load-layer", "3", $layer3,
          "--dump", $loadedDump) (Join-Path $work "load.log")
    Assert-FileEqual $gold $loadedDump "loaded class dump"

    $stagedDir = Join-Path $work "staged-stop"
    New-Item -ItemType Directory -Path $stagedDir | Out-Null
    $stagedBase = Join-Path $stagedDir "ck"
    $stagedLog = Join-Path $stagedDir "run.log"
    Invoke-Layer "staged layer 3 to 4 stop" `
        @("5", "--threads", "$Threads", "--caps", $caps,
          "--load-layer", "3", $layer3,
          "--checkpoint", $stagedBase, "0", "--ckpt-chunk", "500",
          "--stop-after", "4") $stagedLog
    $stagedText = Get-Content -Raw -LiteralPath $stagedLog
    if ($stagedText -notmatch "layer 2: not loaded in staged process" -or
        $stagedText -notmatch "STOPPED AFTER LAYER 4" -or
        !(Test-Path -LiteralPath ($stagedBase + ".L4.snap"))) {
        throw "staged stop did not produce a closed layer-4 snapshot"
    }

    # Production-shaped sparse-lineage rehearsal.  Layer 3 is an exact
    # import; all descendants are watermarked by the checkpoint fingerprint,
    # deliberately interrupted, and forbidden to load in production mode.
    Write-Host "== watermarked rehearsal stage 3->4 kill/resume =="
    $reh34Dir = Join-Path $work "rehearsal-34"
    New-Item -ItemType Directory -Path $reh34Dir | Out-Null
    $reh34Base = Join-Path $reh34Dir "ck"
    $reh34Out = Join-Path $reh34Dir "killed.out"
    $reh34Err = Join-Path $reh34Dir "killed.err"
    $reh34Args = @(
        "5", "--threads", "$Threads", "--caps", $caps,
        "--rehearsal-denom", "2",
        "--load-layer", "3", $layer3,
        "--checkpoint", $reh34Base, "0", "--ckpt-chunk", "500",
        "--stop-after", "4"
    )
    $reh34Proc = Start-Process -FilePath $exe -ArgumentList $reh34Args `
        -RedirectStandardOutput $reh34Out `
        -RedirectStandardError $reh34Err `
        -PassThru -WindowStyle Hidden
    $rehDeadline = (Get-Date).AddSeconds(30)
    while ((Get-Date) -lt $rehDeadline -and
           !(Test-Path -LiteralPath ($reh34Base + ".a")) -and
           !(Test-Path -LiteralPath ($reh34Base + ".b"))) {
        Start-Sleep -Milliseconds 25
        $reh34Proc.Refresh()
        if ($reh34Proc.HasExited) { break }
    }
    $reh34Proc.Refresh()
    if ($reh34Proc.HasExited -or
        (!(Test-Path -LiteralPath ($reh34Base + ".a")) -and
         !(Test-Path -LiteralPath ($reh34Base + ".b")))) {
        Get-Content -LiteralPath $reh34Out -ErrorAction SilentlyContinue
        Get-Content -LiteralPath $reh34Err -ErrorAction SilentlyContinue
        throw "rehearsal stage completed or failed before forced kill"
    }
    Stop-Process -Id $reh34Proc.Id -Force
    Wait-Process -Id $reh34Proc.Id -Timeout 10 -ErrorAction SilentlyContinue
    $reh34Resume = @(
        "5", "--threads", "$Threads", "--caps", $caps,
        "--rehearsal-denom", "2",
        "--checkpoint", $reh34Base, "0", "--ckpt-chunk", "500",
        "--resume", $reh34Base, "--stop-after", "4"
    )
    $reh34ResumeLog = Join-Path $reh34Dir "resume.log"
    Invoke-Layer "watermarked rehearsal resume" `
        $reh34Resume $reh34ResumeLog
    $reh4 = $reh34Base + ".L4.snap"
    $reh34Text = Get-Content -Raw -LiteralPath $reh34ResumeLog
    if (!(Test-Path -LiteralPath $reh4) -or
        $reh34Text -notmatch "BOUNDED REHEARSAL lineage" -or
        $reh34Text -notmatch "STOPPED AFTER LAYER 4") {
        throw "watermarked rehearsal resume did not close layer 4"
    }

    Invoke-ExpectExit "rehearsal production-lineage refusal" `
        @("5", "--threads", "$Threads", "--caps", $caps,
          "--load-layer", "4", $reh4,
          "--dump", (Join-Path $reh34Dir "forbidden.csv")) 12 $work
    Invoke-ExpectExit "rehearsal denominator mismatch refusal" `
        @("5", "--threads", "$Threads", "--caps", $caps,
          "--rehearsal-denom", "3",
          "--load-layer", "4", $reh4,
          "--checkpoint", (Join-Path $reh34Dir "wrong-ck"), "0",
          "--dump", (Join-Path $reh34Dir "wrong.csv")) 12 $work

    $reh45Dir = Join-Path $work "rehearsal-45"
    New-Item -ItemType Directory -Path $reh45Dir | Out-Null
    $rehDump = Join-Path $reh45Dir "rehearsal.csv"
    $reh45Args = @(
        "5", "--threads", "$Threads", "--caps", $caps,
        "--rehearsal-denom", "2", "--load-layer", "4", $reh4,
        "--checkpoint", (Join-Path $reh45Dir "ck"), "0",
        "--ckpt-chunk", "500", "--dump", $rehDump
    )
    Invoke-Layer "watermarked rehearsal final stage" `
        $reh45Args (Join-Path $reh45Dir "run.log")
    $reh45ReplayDir = Join-Path $work "rehearsal-45-replay"
    New-Item -ItemType Directory -Path $reh45ReplayDir | Out-Null
    $rehReplay = Join-Path $reh45ReplayDir "rehearsal.csv"
    $rehReplayArgs = @(
        "5", "--threads", "$Threads", "--caps", $caps,
        "--rehearsal-denom", "2", "--load-layer", "4", $reh4,
        "--checkpoint", (Join-Path $reh45ReplayDir "ck"), "0",
        "--ckpt-chunk", "500", "--dump", $rehReplay
    )
    Invoke-Layer "watermarked rehearsal final replay" `
        $rehReplayArgs (Join-Path $reh45ReplayDir "run.log")
    Assert-FileEqual $rehDump $rehReplay "rehearsal final replay"
    & python $s4 $rehDump "--classes" "354" "--rehearsal" `
        "--expect-n" "119726523775193340292890624000" `
        *> (Join-Path $reh45Dir "external-checksum.log")
    if ($LASTEXITCODE -ne 0) {
        Get-Content -LiteralPath (Join-Path $reh45Dir "external-checksum.log")
        throw "watermarked rehearsal external checksum failed"
    }

    Write-Host "== S4 exact summation =="
    & python $s4 $gold "--classes" "355" "--expect-n" $expectedN `
        *> (Join-Path $work "s4.log")
    if ($LASTEXITCODE -ne 0) {
        Get-Content -LiteralPath (Join-Path $work "s4.log")
        throw "S4 exact summation failed"
    }

    $rng = [Random]::new(20260730)
    $lastBase = ""
    for ($i = 1; $i -le $KillIterations; $i++) {
        Write-Host "== checkpoint kill/resume iteration $i =="
        $delay = $rng.Next(50, 350)
        $lastBase = Start-Kill-And-Resume `
            (Join-Path $work "kill-$i") 3 $layer3 $gold `
            -ChunkParents 500 -ExtraDelayMs $delay
    }

    $wideDump = Join-Path $work "wide.csv"
    $layer4Wide = Join-Path $work "wide.L4.snap"
    Invoke-Layer "u128 wide-path parity" `
        @("5", "--threads", "$Threads", "--caps", $caps,
          "--force-wide", "--ref", $ref,
          "--save-layer", "4", $layer4Wide, "--dump", $wideDump) `
        (Join-Path $work "wide.log")
    Assert-FileEqual $gold $wideDump "wide class dump"

    Write-Host "== wide checkpoint kill/resume =="
    $wideBase = Start-Kill-And-Resume `
        (Join-Path $work "wide-kill") 4 $layer4Wide $gold `
        -ForceWide -ChunkParents 100 -ExtraDelayMs 50 `
        -RequireBothGenerations -LockGenerationMs 10000

    Invoke-ExpectExit "checkpoint mode mismatch refusal" `
        @("5", "--threads", "$Threads", "--caps", $caps, "--ref", $ref,
          "--checkpoint", $wideBase, "0", "--ckpt-chunk", "100",
          "--resume", $wideBase) 12 $work

    $staleDir = Join-Path $work "stale-base"
    New-Item -ItemType Directory -Path $staleDir | Out-Null
    $staleBase = Join-Path $staleDir "ck"
    Copy-Item -LiteralPath ($lastBase + ".a") -Destination ($staleBase + ".a")
    Invoke-ExpectExit "stale checkpoint base refusal" `
        @("5", "--threads", "$Threads", "--caps", $caps, "--ref", $ref,
          "--load-layer", "3", $layer3,
          "--checkpoint", $staleBase, "0", "--ckpt-chunk", "500") 2 $work

    $missingBase = Join-Path (Join-Path $work "missing-parent") "ck"
    Invoke-ExpectExit "checkpoint write failure is fatal" `
        @("5", "--threads", "$Threads", "--caps", $caps, "--ref", $ref,
          "--load-layer", "3", $layer3,
          "--checkpoint", $missingBase, "0", "--ckpt-chunk", "500") 12 $work

    Write-Host "== newest-checkpoint corruption fallback =="
    $fallbackDir = Join-Path $work "fallback"
    New-Item -ItemType Directory -Path $fallbackDir | Out-Null
    $fallback = Join-Path $fallbackDir "ck"
    Copy-Item -LiteralPath ($lastBase + ".a") -Destination ($fallback + ".a")
    Copy-Item -LiteralPath ($lastBase + ".b") -Destination ($fallback + ".b")
    Copy-Item -LiteralPath ($lastBase + ".L4.snap") `
        -Destination ($fallback + ".L4.snap")
    $newest = Get-ChildItem -LiteralPath ($fallback + ".a"),($fallback + ".b") |
        Sort-Object LastWriteTime -Descending | Select-Object -First 1
    $stream = [IO.File]::Open(
        $newest.FullName, [IO.FileMode]::Append,
        [IO.FileAccess]::Write, [IO.FileShare]::None)
    try { $stream.WriteByte(0x7f) } finally { $stream.Dispose() }
    $fallbackDump = Join-Path $fallbackDir "dump.csv"
    Invoke-Layer "fallback to previous checkpoint generation" `
        @("5", "--threads", "$Threads", "--caps", $caps,
          "--ref", $ref, "--dump", $fallbackDump,
          "--checkpoint", $fallback, "0", "--ckpt-chunk", "500",
          "--resume", $fallback) (Join-Path $fallbackDir "resume.log")
    Assert-FileEqual $gold $fallbackDump "fallback class dump"

    Write-Host "== C=6 known-class bridge =="
    & $exe 6 "--bridge-only" *> (Join-Path $work "bridge.log")
    if ($LASTEXITCODE -ne 0) { throw "C=6 bridge failed" }
    $bridge = Get-Content -Raw -LiteralPath (Join-Path $work "bridge.log")
    if ($bridge -notmatch "stab=120" -or $bridge -notmatch "stab=8") {
        throw "C=6 bridge stabilizers do not match outer orbit sizes"
    }

    $monolithBase = Join-Path $work "forbidden-monolith"
    Invoke-ExpectExit "acknowledged C6 monolith refusal" `
        @("6", "--threads", "$Threads",
          "--caps", "2000,14000000,20000,20000,100000",
          "--checkpoint", $monolithBase, "0",
          "--stop-after", "4", "--ack-full-c6") 2 $work

    Invoke-ExpectExit "unbounded C6 refusal" @("6") 2 $work

    $succeeded = $true
    Write-Host "LAYER-DP CHECKPOINT AND EXACTNESS GATES PASSED"
}
finally {
    if (!$succeeded -or $KeepArtifacts) {
        Write-Host "layer-DP gate artifacts retained at $work"
    } else {
        $resolvedWork = [IO.Path]::GetFullPath($work)
        if (!$resolvedWork.StartsWith(
                $tempRoot, [StringComparison]::OrdinalIgnoreCase)) {
            throw "refusing to remove non-temporary path $resolvedWork"
        }
        Remove-Item -LiteralPath $resolvedWork -Recurse -Force
    }
}
