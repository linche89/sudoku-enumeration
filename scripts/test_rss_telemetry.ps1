$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
$tokens = $null; $errors = $null
$ast = [Management.Automation.Language.Parser]::ParseFile(
    (Join-Path $PSScriptRoot 'watch_rss.ps1'), [ref]$tokens, [ref]$errors)
if ($errors.Count) { throw 'watchdog parse error' }
$definition = $ast.Find({param($node)
    $node -is [Management.Automation.Language.FunctionDefinitionAst] -and
    $node.Name -eq 'Write-RssTelemetry'
}, $true)
. ([scriptblock]::Create($definition.Extent.Text))
$dir = Join-Path $repo ('data/logs/rss-telemetry-test-' + [guid]::NewGuid().ToString('N'))
$null = New-Item -ItemType Directory -Path $dir
$path = Join-Path $dir 'rss.csv'
Write-RssTelemetry $path 'header'
$reader = [IO.FileStream]::new($path,[IO.FileMode]::Open,[IO.FileAccess]::Read,[IO.FileShare]::ReadWrite)
try { Write-RssTelemetry $path 'shared-reader-write' } finally { $reader.Dispose() }
$lock = [IO.FileStream]::new($path,[IO.FileMode]::Open,[IO.FileAccess]::ReadWrite,[IO.FileShare]::None)
try { Write-RssTelemetry $path 'must-not-throw'; $guardReached = $true } finally { $lock.Dispose() }
if (!$guardReached) { throw 'Telemetry failure bypassed following guard' }
Write-RssTelemetry $path 'recovered'
$lines = @(Get-Content -LiteralPath $path)
if (($lines -join '|') -ne 'header|shared-reader-write|recovered') { throw 'Unexpected telemetry records' }
Write-Host "RSS_TELEMETRY_PASS shared_reader=yes exclusive_lock_isolated=yes recovery=yes checkpoint_IO=none logs=$dir"

# Real finite child: lock the telemetry file while the watcher is running.
# It must keep monitoring, recover logging, and preserve the child's exit 7.
$livePath = Join-Path $dir 'live.csv'
$childCode = "Start-Sleep -Seconds 2; " +
    "`$held = [IO.FileStream]::new('$livePath',[IO.FileMode]::Open,[IO.FileAccess]::ReadWrite,[IO.FileShare]::None); " +
    'try { Start-Sleep -Seconds 4 } finally { $held.Dispose() }; Start-Sleep -Seconds 2; exit 7'
$encoded = [Convert]::ToBase64String([Text.Encoding]::Unicode.GetBytes($childCode))
$watchLog = Join-Path $dir 'watch.log'
& powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot 'watch_rss.ps1') `
    -Exe (Join-Path $PSHOME 'powershell.exe') -Arguments "-NoProfile -EncodedCommand $encoded" `
    -IntervalSeconds 1 -LimitGB 1 -MaxMinutes 1 -LogPath $livePath `
    -StdoutPath (Join-Path $dir 'child.stdout') -StderrPath (Join-Path $dir 'child.stderr') > $watchLog
if ($LASTEXITCODE -ne 7) { throw 'Watchdog did not preserve real child exit 7' }
$watchText = Get-Content -LiteralPath $watchLog -Raw
if ($watchText -notmatch 'RSS_TELEMETRY_WARNING' -or $watchText -notmatch 'process exited with code 7') {
    throw 'Live lock did not exercise telemetry failure and terminal recovery'
}
if (@(Import-Csv -LiteralPath $livePath).Count -lt 2) { throw 'Live logging did not recover' }
Write-Host 'RSS_TELEMETRY_LIVE_PASS locked_child=yes exit=7 recovery=yes checkpoint_IO=none'

$sleepCode = [Convert]::ToBase64String([Text.Encoding]::Unicode.GetBytes('Start-Sleep -Seconds 20; exit 0'))
& powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot 'watch_rss.ps1') `
    -Exe (Join-Path $PSHOME 'powershell.exe') -Arguments "-NoProfile -EncodedCommand $sleepCode" `
    -IntervalSeconds 1 -LimitGB 0 -MaxMinutes 1 -LogPath (Join-Path $dir 'limit.csv') `
    -StdoutPath (Join-Path $dir 'limit.stdout') -StderrPath (Join-Path $dir 'limit.stderr') > (Join-Path $dir 'limit.log')
if ($LASTEXITCODE -ne 99) { throw 'Real memory guard failed to stop synthetic child' }
Write-Host 'RSS_TELEMETRY_GUARD_PASS forced_memory_stop=99 checkpoint_IO=none'
