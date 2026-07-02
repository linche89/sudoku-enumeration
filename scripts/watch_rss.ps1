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

$resolvedExe = (Resolve-Path -LiteralPath $Exe).Path
if ($LogPath -eq "") {
    $stamp = Get-Date -Format "yyyyMMdd-HHmmss"
    $name = [IO.Path]::GetFileNameWithoutExtension($resolvedExe)
    $LogPath = Join-Path (Get-Location) "data\$name-rss-$stamp.log"
    if ($StdoutPath -eq "") {
        $StdoutPath = Join-Path (Get-Location) "data\$name-out-$stamp.log"
    }
    if ($StderrPath -eq "") {
        $StderrPath = Join-Path (Get-Location) "data\$name-err-$stamp.log"
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
        "$((Get-Date).ToString('o')),$($proc.Id),$rss,$gb" | Add-Content -LiteralPath $LogPath
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
    if ($null -eq $code) { $code = 0 }
    Write-Host "process exited with code $code"
    exit $code
}
