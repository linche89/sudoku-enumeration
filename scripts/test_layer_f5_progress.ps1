# Finite progress/parser and native-logging tests, with no C6 checkpoint I/O.
$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'layer_f5_progress.ps1')
function Assert-Progress($Condition, [string]$Message) { if (!$Condition) { throw $Message } }
function New-SyntheticChunk([long]$Begin, [long]$Count=10000, [double]$Seconds=5) {
    'CLOSED_F5_CHUNK begin={0} count={1} live={1} labelled_matchings=1 wall_s={2} closed_prefix={3}/96452976' -f
        $Begin, $Count, $Seconds.ToString('F6',[cultureinfo]::InvariantCulture), ($Begin+$Count)
}
$accepted = 0
$unknown = ConvertFrom-F5ProgressLog @('PREFLIGHT loading', 'CLOSED_F5_CHUNK begin=0 count=10000')
Assert-Progress ($null -eq $unknown.Prefix -and $null -eq $unknown.Rate) 'Partial line became progress'
$accepted++
$resumed = 'RESUMED_F5 chunks=3020 closed_prefix=30200000 total_entries=96452976'
$value = ConvertFrom-F5ProgressLog @($resumed)
Assert-Progress ($value.Prefix -eq 30200000 -and $null -eq $value.Rate) 'Wrong resumed-only record'
$accepted++
$one = New-SyntheticChunk 30200000
$two = New-SyntheticChunk 30210000 10000 10
$value = ConvertFrom-F5ProgressLog @($resumed,$one,$two) @($resumed)
Assert-Progress ($value.Prefix -eq 30220000 -and $value.StartPrefix -eq 30200000 -and
    [Math]::Abs($value.Rate - 20000.0/15) -lt 0.000001) 'Wrong weighted recent speed'
Assert-Progress ([Math]::Abs($value.RemainingComputeSeconds - ((96452976-30220000)/$value.Rate)) -lt 0.000001) 'Wrong remaining work estimate'
$accepted++
$many = @(0..34 | ForEach-Object { New-SyntheticChunk ($_*10000) 10000 $(if ($_ -lt 5) {100} else {5}) })
$value = ConvertFrom-F5ProgressLog $many
Assert-Progress ($value.SampleChunks -eq 30 -and $value.Rate -eq 2000) 'Rolling window failed'
$accepted++
$summary = 'SUMMARY status=CLOSED_F5_CATALOGUE new_chunks=1 new_indices=2976 closed_prefix=96452976 total_entries=96452976 live_closed=96452755 N6=NOT_COMPUTED'
$value = ConvertFrom-F5ProgressLog @((New-SyntheticChunk 96450000 2976 2),$summary)
Assert-Progress ($value.Percent -eq 100 -and $value.RemainingComputeSeconds -eq 0 -and
    $value.SummaryStatus -eq 'CLOSED_F5_CATALOGUE') 'Wrong final short-chunk progress'
$accepted++
$value = ConvertFrom-F5ProgressLog @($one, 'CLOSED_F5_CHUNK begin=30210000 count=10000 live=10000 wall_s=')
Assert-Progress ($value.Prefix -eq 30210000 -and $value.SampleChunks -eq 1) 'Partial tail corrupted last committed progress'
$accepted++
$culture = [Threading.Thread]::CurrentThread.CurrentCulture
try {
    [Threading.Thread]::CurrentThread.CurrentCulture = [cultureinfo]'fr-FR'
    $value = ConvertFrom-F5ProgressLog @((New-SyntheticChunk 0 10000 6.5))
    Assert-Progress ([Math]::Abs($value.Rate - 10000.0/6.5) -lt 0.000001) 'Locale changed decimal parsing'
    Assert-Progress ((Format-F5ProgressLine $value) -match 'saved=10,000/96,452,976') 'Locale changed display separators'
} finally { [Threading.Thread]::CurrentThread.CurrentCulture = $culture }
$accepted++
Assert-Progress ((Format-F5ProgressDuration $null) -eq '--' -and
    (Format-F5ProgressDuration 3601) -eq '1h01m') 'Wrong duration display'
$accepted++

$rejected = 0
foreach ($badLines in @(
    @($one.Replace('/96452976','/96452975')),
    @($one.Replace('count=10000','count=9999')),
    @($one.Replace('live=10000','live=10001')),
    @($one.Replace('wall_s=5.000000','wall_s=0.000000')),
    @((New-SyntheticChunk 30205000 5000)),
    @($one,(New-SyntheticChunk 30220000)),
    @($one, $resumed),
    @($resumed.Replace('30200000','96460000')),
    @($summary.Replace('CLOSED_F5_CATALOGUE','INCOMPLETE_RESUMABLE'))
)) {
    $caught = $false
    try { $null = ConvertFrom-F5ProgressLog $badLines } catch { $caught = $true }
    Assert-Progress $caught 'Malformed progress fixture accepted'
    $rejected++
}
Write-Host "F5_PROGRESS_PARSER_PASS accepted=$accepted rejected=$rejected checkpoint_IO=none"

# Exercise stage labels with in-memory logs and mocked read-only process data.
& {
    $hasMain = $true
    $preparing = $false
    $active = $true
    $started = [DateTime]::Now.AddMinutes(-10)
    $heartbeat = [DateTimeOffset]::Now
    $exitCode = $null
    $backedUp = $false
    $engineLines = @($resumed, $one)
    function Test-Path { param($LiteralPath,$PathType)
        if ($LiteralPath -like '*f5-window-controller.log') { return $hasMain }; return $true
    }
    function Get-F5ControllerProgress([string]$ControllerPath) {
        [pscustomobject]@{ Directory=$(if ($preparing -and $ControllerPath -like '*f5-window-controller.log') {$null} else {'E:\synthetic-logs'});
            Path=$ControllerPath; WorkSeconds=19800; ExitCode=$exitCode; BackupReported=$backedUp }
    }
    function Get-Content { [CmdletBinding()] param($LiteralPath,[int]$TotalCount,[int]$Tail)
        if ($LiteralPath -like '*rss.csv') {
            if ($TotalCount -eq 1) { return 'timestamp,pid,rss_bytes,rss_gb' }
            return ($heartbeat.ToString('o') + ',42,1073741824,1')
        }
        if ($TotalCount -gt 0) { return @($resumed) }; return $engineLines
    }
    function Get-Item([string]$LiteralPath) { [pscustomobject]@{CreationTimeUtc=$started.ToUniversalTime()} }
    function Get-Process { [CmdletBinding()] param([int]$Id)
        if ($active) { return [pscustomobject]@{ProcessName='layer_reverse_f5';StartTime=$started;Id=42} }
        return $null
    }
    $result = Get-F5ProgressSnapshot 'E:\synthetic-session'
    Assert-Progress ($result.State -eq 'COMPUTING' -and $result.WindowSecondsLeft -gt 19000 -and $result.RssGiB -eq 1) 'Active phase wrong'
    $heartbeat = [DateTimeOffset]::Now.AddMinutes(-5)
    $result = Get-F5ProgressSnapshot 'E:\synthetic-session'
    Assert-Progress ($result.State -eq 'NO_RECENT_HEARTBEAT' -and $null -eq $result.WindowSecondsLeft) 'Stale heartbeat presented as active ETA'
    $active = $false
    $engineLines = @($summary)
    $result = Get-F5ProgressSnapshot 'E:\synthetic-session'
    Assert-Progress ($result.State -eq 'COMPUTE_ENDED_BACKUP_PENDING' -and $result.Percent -eq 100) '100 percent falsely presented as backed up'
    $exitCode = 0; $backedUp = $true
    $result = Get-F5ProgressSnapshot 'E:\synthetic-session'
    Assert-Progress ($result.State -eq 'F5_CLOSED_LOGGED_AUDIT_PENDING') 'Final audit boundary missing'
    $exitCode = 98
    $result = Get-F5ProgressSnapshot 'E:\synthetic-session'
    Assert-Progress ($result.State -eq 'STOPPED_REVIEW_LOGS') 'Hard stop upgraded to success'
    $preparing = $true; $exitCode = 0
    $result = Get-F5ProgressSnapshot 'E:\synthetic-session'
    Assert-Progress ($result.State -eq 'PREPARING_MAIN_AFTER_CANARY' -and $null -eq $result.WindowSecondsLeft) 'Canary/main handoff mislabeled'
    $preparing = $false; $hasMain = $false; $engineLines = @($resumed, $one)
    $engineLines += 'SUMMARY status=INCOMPLETE_RESUMABLE new_chunks=1 new_indices=10000 closed_prefix=30210000 total_entries=96452976 live_closed=30209969 N6=NOT_COMPUTED'
    $result = Get-F5ProgressSnapshot 'E:\synthetic-session'
    Assert-Progress ($result.Stage -eq 'CANARY' -and $result.State -eq 'WINDOW_SAVED') 'Canary scope lost'
    Write-Host 'F5_PROGRESS_PHASE_PASS cases=7 checkpoint_IO=none'
}

# Extract only Invoke-Logged; do not execute run_f5.ps1 main or any C6 child.
$tokens = $null; $errors = $null
$ast = [Management.Automation.Language.Parser]::ParseFile((Join-Path $repo 'run_f5.ps1'),[ref]$tokens,[ref]$errors)
Assert-Progress ($errors.Count -eq 0) 'Launcher parse failed'
$definition = $ast.Find({param($node)
    $node -is [Management.Automation.Language.FunctionDefinitionAst] -and $node.Name -eq 'Invoke-Logged'
},$true)
Assert-Progress ($null -ne $definition) 'Native invocation function missing'
$testDir = Join-Path $repo ('data/logs/f5-progress-tests-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $testDir | Out-Null
$powershellExe = (Get-Command powershell.exe -CommandType Application | Select-Object -First 1).Source
& {
    . ([scriptblock]::Create($definition.Extent.Text))
    # Deliberately break the advisory reader. Native logs/exits must survive.
    function Get-F5ProgressSnapshot([string]$SessionPath) { throw 'SYNTHETIC_DISPLAY_FAILURE' }
    foreach ($expectedExit in @(0,98,7)) {
        $childCode = 'Write-Output "rss=1.25GB"; Write-Output "SYNTHETIC_FINAL_BACKUP_DONE"; exit ' + $expectedExit
        $encoded = [Convert]::ToBase64String([Text.Encoding]::Unicode.GetBytes($childCode))
        $log = Join-Path $testDir ("native-exit-$expectedExit.log")
        $caught = $false
        try {
            $actualExit = Invoke-Logged $powershellExe @('-NoProfile','-EncodedCommand',$encoded) $log -AllowBoundStop -ShowF5Progress
            Assert-Progress ($actualExit -eq $expectedExit) 'Native exit changed by progress display'
        } catch {
            if ($expectedExit -ne 7 -or $_.Exception.Message -notmatch '^Stage exited 7\.') { throw }
            $caught = $true
        }
        Assert-Progress (($expectedExit -eq 7) -eq $caught) 'Wrong native exit refusal behavior'
        $raw = Get-Content -LiteralPath $log -Raw
        Assert-Progress ($raw -match 'rss=1.25GB' -and $raw -match 'SYNTHETIC_FINAL_BACKUP_DONE') 'Raw output/backup marker lost'
    }
}
Write-Host "F5_PROGRESS_NATIVE_LOGGING_PASS exits=0,98,7 display_failure_isolated=yes raw_RSS_and_terminal_preserved=yes checkpoint_IO=none logs=$testDir"
