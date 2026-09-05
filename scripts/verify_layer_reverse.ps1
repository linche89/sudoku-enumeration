param(
    [ValidateRange(2, 24)] [int]$Threads = 4,
    [ValidateRange(30, 600)] [int]$SecondsPerProcess = 180,
    [switch]$SkipBuild
)
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root
if (!$SkipBuild) {
    & "$PSScriptRoot/build_layer_reverse.ps1"
    if ($LASTEXITCODE -ne 0) { throw "Production reverse-F5 build failed" }
}
# These existing verified dependencies are deliberately NOT rebuilt here.
# Run the full/direct/shared release gates first on a fresh checkout.
foreach ($name in @("layer_reverse_f5", "layer_shared_f4", "layer_dp_gate")) {
    if (!(Test-Path -LiteralPath "build/$name.exe")) {
        throw "Required verified gate executable is missing: build/$name.exe"
    }
}
$logRoot = Join-Path $root "data/logs"
if (!(Test-Path -LiteralPath $logRoot)) { New-Item -ItemType Directory -Path $logRoot | Out-Null }
$work = Join-Path $logRoot ("layer-reverse-release-gate-" + [guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Path $work | Out-Null
$log = Join-Path $work "driver.log"
Write-Host "ARTIFACTS $work"
& python experiments/proto/layer_reverse_gate.py --threads $Threads `
    --seconds-per-process $SecondsPerProcess --output (Join-Path $work "fixtures") *> $log
if ($LASTEXITCODE -ne 0) {
    Get-Content -LiteralPath $log -Tail 70
    throw "Actual reverse-F5 release gate failed; full log: $log"
}
Get-Content -LiteralPath $log -Tail 4
Write-Host "PRODUCTION REVERSE F5 CHECKS PASSED; disposable artifacts retained at $work"
