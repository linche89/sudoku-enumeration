[CmdletBinding()]
param(
    [switch]$Watch,
    [ValidateRange(5,60)] [int]$IntervalSeconds = 15,
    [ValidateRange(0,10000)] [int]$Updates = 0,
    [string]$SessionPath = ''
)

# Safe in a SECOND terminal while run_f5 is active. No process start/stop,
# checkpoint access, locks, writes or backup/hash scans occur in this viewer.
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'scripts/layer_f5_progress.ps1')
Write-Host 'READ-ONLY F5 progress. ETA = remaining F5 COMPUTE time, not this session or N(6).'
if ($Watch) { Write-Host "Refreshes every $IntervalSeconds s. Ctrl+C in THIS viewer stops only the viewer." }
$shown = 0
$lastSession = ''
do {
    try {
        $selected = if ($SessionPath) { (Resolve-Path -LiteralPath $SessionPath -ErrorAction Stop).Path } else { Find-F5ManualSession $PSScriptRoot }
        if ($selected -ne $lastSession) { Write-Host "SESSION: $selected"; $lastSession = $selected }
        $snapshot = Get-F5ProgressSnapshot $selected
        Write-Host (Format-F5ProgressLine $snapshot)
    } catch {
        Write-Warning ("Progress temporarily unavailable; computation is unaffected: " + $_.Exception.Message)
    }
    $shown++
    if (!$Watch -or ($Updates -gt 0 -and $shown -ge $Updates)) { break }
    Start-Sleep -Seconds $IntervalSeconds
} while ($true)
