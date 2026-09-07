param(
    [Parameter(Mandatory=$true)]
    [string]$Exe,

    [string[]]$Arguments = @(),

    [int]$LimitGB = 58,

    [int]$IntervalSeconds = 5,

    [int]$MaxMinutes = 0,

    [string]$LogPath = "",

    [string]$StdoutPath = "",

    [string]$StderrPath = ""
)

$ErrorActionPreference = "Stop"

# Telemetry must not abort the watchdog and enter WaitForExit before its
# time/RSS checks. Use an explicitly shared write-only append stream instead
# of the PowerShell Add-Content provider (which failed during live viewing).
function Write-RssTelemetry([string]$Path, [string]$Line) {
    $stream = $null
    try {
        $stream = [IO.FileStream]::new($Path, [IO.FileMode]::Append,
            [IO.FileAccess]::Write, [IO.FileShare]::ReadWrite)
        $bytes = [Text.Encoding]::UTF8.GetBytes($Line + [Environment]::NewLine)
        $stream.Write($bytes, 0, $bytes.Length)
    } catch {
        Write-Host ('RSS_TELEMETRY_WARNING: ' + $_.Exception.Message)
    } finally {
        if ($null -ne $stream) {
            try { $stream.Dispose() }
            catch { Write-Host ('RSS_TELEMETRY_WARNING: ' + $_.Exception.Message) }
        }
    }
}

$resolvedExe = (Resolve-Path -LiteralPath $Exe).Path
if ($LogPath -eq "") {
    $stamp = Get-Date -Format "yyyyMMdd-HHmmss"
    $name = [IO.Path]::GetFileNameWithoutExtension($resolvedExe)
    $logDir = Join-Path (Get-Location) "data\logs"
    New-Item -ItemType Directory -Force -Path $logDir | Out-Null
    $LogPath = Join-Path $logDir "$name-rss-$stamp.log"
    if ($StdoutPath -eq "") {
        $StdoutPath = Join-Path $logDir "$name-out-$stamp.log"
    }
    if ($StderrPath -eq "") {
        $StderrPath = Join-Path $logDir "$name-err-$stamp.log"
    }
}

foreach ($path in @($LogPath, $StdoutPath, $StderrPath)) {
    if ($path -eq "") { continue }
    $dir = Split-Path -Parent $path
    if ($dir -and !(Test-Path $dir)) {
        New-Item -ItemType Directory -Force -Path $dir | Out-Null
    }
}

$limitBytes = [int64]$LimitGB * 1024 * 1024 * 1024
$startInfo = @{
    FilePath = $resolvedExe
    ArgumentList = $Arguments
    PassThru = $true
    WindowStyle = "Hidden"
    RedirectStandardOutput = $StdoutPath
    RedirectStandardError = $StderrPath
}

Write-Host "starting: $resolvedExe $($Arguments -join ' ')"
Write-Host "RSS limit: ${LimitGB}GB; log: $LogPath"
Write-Host "stdout: $StdoutPath"
Write-Host "stderr: $StderrPath"
if ($MaxMinutes -gt 0) {
    Write-Host "time limit: ${MaxMinutes} minutes"
}

$proc = Start-Process @startInfo
# Retain the OS handle while the child is alive. Windows PowerShell can
# otherwise lose ExitCode after Refresh and silently return null.
$null = $proc.Handle
"timestamp,pid,rss_bytes,rss_gb" | Set-Content -LiteralPath $LogPath
$started = Get-Date

try {
    while (!$proc.HasExited) {
        Start-Sleep -Seconds $IntervalSeconds
        $proc.Refresh()
        if ($proc.HasExited) { break }
        $p = Get-Process -Id $proc.Id -ErrorAction SilentlyContinue
        if ($null -eq $p) { break }
        $rss = [int64]$p.WorkingSet64
        $gb = [Math]::Round($rss / 1GB, 3)
        Write-RssTelemetry $LogPath "$((Get-Date).ToString('o')),$($proc.Id),$rss,$gb"
        Write-Host ("rss={0}GB" -f $gb)
        if ($rss -gt $limitBytes) {
            Write-Host "RSS exceeded ${LimitGB}GB; stopping process $($proc.Id)"
            Stop-Process -Id $proc.Id -Force
            exit 99
        }
        if ($MaxMinutes -gt 0 -and ((Get-Date) - $started).TotalMinutes -ge $MaxMinutes) {
            Write-Host "time limit reached; stopping process $($proc.Id)"
            Stop-Process -Id $proc.Id -Force
            exit 98
        }
    }
}
finally {
    if (!$proc.HasExited) {
        $proc.WaitForExit()
    } else {
        $proc.WaitForExit()
    }
    $proc.Refresh()
}

if ($proc.HasExited) {
    $code = $proc.ExitCode
    if ($null -eq $code) { throw 'Child exited without a readable exit code; refusing success' }
    Write-Host "process exited with code $code"
    exit $code
}
