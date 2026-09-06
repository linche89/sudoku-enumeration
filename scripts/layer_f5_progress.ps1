# Read-only, advisory progress helpers. Never open a checkpoint or control a process.
# English/ASCII output is intentional for Windows PowerShell 5.1 encoding safety.

function ConvertFrom-F5ProgressLog([string[]]$Lines, [string[]]$HeadLines = @()) {
    $prefix = $null
    $startPrefix = $null
    $summaryStatus = $null
    $samples = [Collections.Generic.List[object]]::new()
    foreach ($line in $HeadLines) {
        if ($line -match '^RESUMED_F5 chunks=\d+ closed_prefix=(\d+) total_entries=(\d+)\s*$') {
            if ([long]$Matches[2] -ne 96452976) { throw 'Unexpected F5 progress domain' }
            $startPrefix = [long]$Matches[1]
        }
    }
    foreach ($line in $Lines) {
        $candidate = $null
        if ($line -match '^CLOSED_F5_CHUNK begin=(\d+) count=(\d+) live=(\d+) .+ wall_s=([0-9.]+) closed_prefix=(\d+)/(\d+)\s*$') {
            $begin = [long]$Matches[1]
            $count = [long]$Matches[2]
            $live = [long]$Matches[3]
            $seconds = [double]::Parse($Matches[4], [cultureinfo]::InvariantCulture)
            $candidate = [long]$Matches[5]
            if ([long]$Matches[6] -ne 96452976 -or $count -le 0 -or $count -gt 10000 -or
                $begin % 10000 -ne 0 -or ($count -ne 10000 -and $candidate -ne 96452976) -or
                $live -gt $count -or $begin + $count -ne $candidate -or $seconds -le 0) {
                throw 'Invalid committed-chunk progress record'
            }
            if ($samples.Count -gt 0 -and $samples[$samples.Count-1].End -ne $begin) {
                throw 'Non-contiguous progress sample'
            }
            $samples.Add([pscustomobject]@{Count=$count; Seconds=$seconds; End=$candidate})
        } elseif ($line -match '^RESUMED_F5 chunks=\d+ closed_prefix=(\d+) total_entries=(\d+)\s*$') {
            if ([long]$Matches[2] -ne 96452976) { throw 'Unexpected F5 progress domain' }
            $candidate = [long]$Matches[1]
            $startPrefix = $candidate
        } elseif ($line -match '^SUMMARY .*\bclosed_prefix=(\d+) total_entries=(\d+) .* N6=NOT_COMPUTED\s*$') {
            if ([long]$Matches[2] -ne 96452976) { throw 'Unexpected F5 summary domain' }
            $candidate = [long]$Matches[1]
            if ($line -notmatch '^SUMMARY status=(INCOMPLETE_RESUMABLE|CLOSED_F5_CATALOGUE) ') {
                throw 'Unexpected F5 summary scope'
            }
            $summaryStatus = $Matches[1]
            if (($candidate -eq 96452976) -ne ($summaryStatus -eq 'CLOSED_F5_CATALOGUE')) {
                throw 'F5 progress summary is not consistent with its prefix'
            }
        }
        # Ignore unrecognized/half-written lines; never count a partial accumulator.
        if ($null -ne $candidate) {
            if ($candidate -lt 0 -or $candidate -gt 96452976 -or
                ($candidate -ne 96452976 -and $candidate % 10000 -ne 0) -or
                ($null -ne $prefix -and $candidate -lt $prefix)) { throw 'Invalid F5 progress prefix' }
            $prefix = $candidate
        }
    }
    $recent = @($samples | Select-Object -Last 30)
    $rate = $null
    $remainingSeconds = $null
    if ($recent.Count -gt 0) {
        $rate = ($recent | Measure-Object Count -Sum).Sum / ($recent | Measure-Object Seconds -Sum).Sum
        if ($null -ne $prefix) { $remainingSeconds = (96452976 - $prefix) / $rate }
    }
    [pscustomobject]@{
        Prefix=$prefix; Total=[long]96452976; StartPrefix=$startPrefix;
        Percent=$(if ($null -ne $prefix) {100.0*$prefix/96452976} else {$null});
        Rate=$rate; RemainingComputeSeconds=$remainingSeconds; SampleChunks=$recent.Count;
        SummaryStatus=$summaryStatus; State='WAITING_FOR_RECORD'; Stage='F5';
        WindowSecondsLeft=$null; RssGiB=$null; WorkerId=$null; EngineActive=$false;
        ControllerPath=$null; LogDirectory=$null; SessionPath=$null
    }
}

function Get-F5ControllerProgress([string]$ControllerPath) {
    $head = @(Get-Content -LiteralPath $ControllerPath -TotalCount 16 -ErrorAction Stop)
    $tail = @(Get-Content -LiteralPath $ControllerPath -Tail 32 -ErrorAction Stop)
    $text = ($head + $tail) -join "`n"
    $directory = $null
    if ($text -match '(?m)^REVERSE_WINDOW logs=(.+?) namespace=.+\r?$') { $directory = $Matches[1].Trim() }
    $workSeconds = $null
    if ($text -match '\bworkseconds=(\d+)\b') { $workSeconds = [long]$Matches[1] }
    $exitCode = $null
    if ($text -match '(?m)^REVERSE_WINDOW_END exit=(\d+) logs=.+ N6=NOT_COMPUTED\r?$') { $exitCode = [int]$Matches[1] }
    [pscustomobject]@{
        Directory=$directory; WorkSeconds=$workSeconds; ExitCode=$exitCode;
        BackupReported=($text -match '(?m)^PHYSICAL_BACKUP phase=after files=\d+ directory=.+');
        Path=$ControllerPath
    }
}

function Get-F5ProgressSnapshot([string]$SessionPath) {
    $snapshot = ConvertFrom-F5ProgressLog @()
    $snapshot.SessionPath = $SessionPath
    $snapshot.State = 'GATES_OR_F4_EXPORT'
    $mainPath = Join-Path $SessionPath 'f5-window-controller.log'
    $canaryPath = Join-Path $SessionPath 'f5-canary-controller.log'
    $controller = $null
    $preparingMain = $false
    if (Test-Path -LiteralPath $mainPath -PathType Leaf) {
        $controller = Get-F5ControllerProgress $mainPath
        $snapshot.Stage = 'MAIN'
        if (!$controller.Directory) {
            $preparingMain = $true
            if (Test-Path -LiteralPath $canaryPath -PathType Leaf) { $controller = Get-F5ControllerProgress $canaryPath }
        }
    } elseif (Test-Path -LiteralPath $canaryPath -PathType Leaf) {
        $controller = Get-F5ControllerProgress $canaryPath
        $snapshot.Stage = 'CANARY'
    }
    if (!$controller) { return $snapshot }
    $snapshot.ControllerPath = $controller.Path
    $snapshot.State = 'CHECKING_INPUTS_OR_BACKUP'
    if (!$controller.Directory) { return $snapshot }
    $stdout = Join-Path $controller.Directory 'stdout.log'
    if (!(Test-Path -LiteralPath $stdout -PathType Leaf)) { return $snapshot }
    # Bounded tail reads: no full progress log, catalogue or checkpoint scan.
    $head = @(Get-Content -LiteralPath $stdout -TotalCount 8 -ErrorAction Stop)
    $lines = @(Get-Content -LiteralPath $stdout -Tail 160 -ErrorAction Stop)
    $parsed = ConvertFrom-F5ProgressLog $lines $head
    $parsed.Stage = $snapshot.Stage
    $parsed.SessionPath = $SessionPath
    $parsed.ControllerPath = $controller.Path
    $parsed.LogDirectory = $controller.Directory
    $snapshot = $parsed
    $rssPath = Join-Path $controller.Directory 'rss.csv'
    $heartbeatRecent = $false
    if (Test-Path -LiteralPath $rssPath -PathType Leaf) {
        $rssLines = @((Get-Content -LiteralPath $rssPath -TotalCount 1 -ErrorAction Stop)) +
            @((Get-Content -LiteralPath $rssPath -Tail 1 -ErrorAction Stop))
        $rssRows = @($rssLines | ConvertFrom-Csv)
        if ($rssRows.Count -eq 1 -and $rssRows[0].pid -match '^\d+$' -and $rssRows[0].rss_bytes -match '^\d+$') {
            $snapshot.WorkerId = [int]$rssRows[0].pid
            $snapshot.RssGiB = [long]$rssRows[0].rss_bytes / 1GB
            $worker = Get-Process -Id $snapshot.WorkerId -ErrorAction SilentlyContinue
            $created = (Get-Item -LiteralPath $stdout).CreationTimeUtc
            $snapshot.EngineActive = ($null -ne $worker -and $worker.ProcessName -eq 'layer_reverse_f5' -and
                [Math]::Abs(($worker.StartTime.ToUniversalTime() - $created).TotalSeconds) -lt 15)
            $heartbeat = [DateTimeOffset]::MinValue
            if ([DateTimeOffset]::TryParse($rssRows[0].timestamp, [ref]$heartbeat)) {
                $heartbeatRecent = ([DateTimeOffset]::Now - $heartbeat).TotalSeconds -lt 60
            }
            if ($snapshot.EngineActive -and $heartbeatRecent -and $null -ne $controller.WorkSeconds) {
                $snapshot.WindowSecondsLeft = [Math]::Max(0, $controller.WorkSeconds -
                    ([DateTime]::UtcNow - $worker.StartTime.ToUniversalTime()).TotalSeconds)
            }
        }
    }
    if ($preparingMain) {
        $snapshot.State = 'PREPARING_MAIN_AFTER_CANARY'
        $snapshot.WindowSecondsLeft = $null
    } elseif ($null -ne $controller.ExitCode) {
        if ($controller.ExitCode -ne 0 -or !$controller.BackupReported) { $snapshot.State = 'STOPPED_REVIEW_LOGS' }
        elseif ($snapshot.SummaryStatus -eq 'CLOSED_F5_CATALOGUE') { $snapshot.State = 'F5_CLOSED_LOGGED_AUDIT_PENDING' }
        elseif ($snapshot.SummaryStatus -eq 'INCOMPLETE_RESUMABLE') { $snapshot.State = 'WINDOW_SAVED' }
        else { $snapshot.State = 'STOPPED_REVIEW_LOGS' }
        $snapshot.WindowSecondsLeft = $null
    } elseif ($snapshot.SummaryStatus) {
        $snapshot.State = 'COMPUTE_ENDED_BACKUP_PENDING'
        $snapshot.WindowSecondsLeft = $null
    } elseif ($snapshot.EngineActive -and $heartbeatRecent) {
        $snapshot.State = if ($null -eq $snapshot.Prefix) {'LOADING_VALIDATING'} else {'COMPUTING'}
    } elseif ($snapshot.EngineActive) {
        $snapshot.State = 'NO_RECENT_HEARTBEAT'
    } else {
        $snapshot.State = 'NO_LIVE_WORKER_PREPARING_OR_STOPPED'
    }
    return $snapshot
}

function Format-F5ProgressDuration($Seconds) {
    if ($null -eq $Seconds -or [double]::IsNaN($Seconds) -or [double]::IsInfinity($Seconds) -or $Seconds -lt 0) { return '--' }
    $minutes = [long][Math]::Ceiling($Seconds / 60.0)
    return ('{0}h{1:00}m' -f [long][Math]::Floor($minutes/60.0), ($minutes%60))
}

function Format-F5ProgressLine($Snapshot) {
    $culture = [cultureinfo]::InvariantCulture
    $fraction = if ($null -eq $Snapshot.Prefix) {'-- (waiting for saved-ID record)'} else {
        '{0}% | saved={1}/{2}' -f $Snapshot.Percent.ToString('F2',$culture),
            $Snapshot.Prefix.ToString('N0',$culture), $Snapshot.Total.ToString('N0',$culture)
    }
    $speed = if ($null -ne $Snapshot.Rate) {$Snapshot.Rate.ToString('N0',$culture)+' IDs/s'} else {'--'}
    $rss = if ($null -ne $Snapshot.RssGiB) {$Snapshot.RssGiB.ToString('F2',$culture)+' GiB'} else {'--'}
    '[{0}] F5 {1} | stage={2}/{3} | recent={4} | F5_compute_left~{5} | window_left~{6} | RSS={7}' -f
        (Get-Date -Format 'HH:mm:ss'), $fraction, $Snapshot.Stage, $Snapshot.State, $speed,
        (Format-F5ProgressDuration $Snapshot.RemainingComputeSeconds),
        (Format-F5ProgressDuration $Snapshot.WindowSecondsLeft), $rss
}

function Find-F5ManualSession([string]$RepositoryRoot) {
    $logs = Join-Path $RepositoryRoot 'data/logs'
    $sessions = @(Get-ChildItem -LiteralPath $logs -Directory -Filter 'f5-manual-*' -ErrorAction Stop |
        Where-Object { $_.Name -match '^f5-manual-[0-9a-f]{32}$' } |
        Sort-Object CreationTimeUtc -Descending | Select-Object -First 1)
    if ($sessions.Count -eq 0) { throw 'No manual F5 session log directory found' }
    return $sessions[0].FullName
}
