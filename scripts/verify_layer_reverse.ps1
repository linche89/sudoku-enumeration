param(
    [ValidateRange(2, 24)] [int]$Threads = 4,
    [ValidateRange(30, 600)] [int]$SecondsPerProcess = 180,
    [switch]$SkipBuild,
    [string]$ReverseExe = "build/layer_reverse_f5.exe",
    [string]$PrefixGateExe = "build/layer_two_missing_prefix_gate.exe"
)
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root
if (!$SkipBuild) {
    if ($ReverseExe -ne "build/layer_reverse_f5.exe" -or
        $PrefixGateExe -ne "build/layer_two_missing_prefix_gate.exe") {
        throw "Candidate executable overrides require -SkipBuild and an explicit candidate build"
    }
    & "$PSScriptRoot/build_layer_reverse.ps1"
    if ($LASTEXITCODE -ne 0) { throw "Production reverse-F5 build failed" }
}
# These existing verified dependencies are deliberately NOT rebuilt here.
# Run the full/direct/shared release gates first on a fresh checkout.
foreach ($path in @($ReverseExe, $PrefixGateExe, "build/layer_shared_f4.exe", "build/layer_dp_gate.exe")) {
    if (!(Test-Path -LiteralPath $path)) {
        throw "Required verified gate executable is missing: $path"
    }
}
$logRoot = Join-Path $root "data/logs"
if (!(Test-Path -LiteralPath $logRoot)) { New-Item -ItemType Directory -Path $logRoot | Out-Null }
$work = Join-Path $logRoot ("layer-reverse-release-gate-" + [guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Path $work | Out-Null
$log = Join-Path $work "driver.log"
Write-Host "ARTIFACTS $work"
& python experiments/proto/layer_reverse_gate.py --threads $Threads `
    --seconds-per-process $SecondsPerProcess --reverse-exe $ReverseExe `
    --prefix-gate-exe $PrefixGateExe --output (Join-Path $work "fixtures") *> $log
if ($LASTEXITCODE -ne 0) {
    Get-Content -LiteralPath $log -Tail 70
    throw "Actual reverse-F5 release gate failed; full log: $log"
}
Get-Content -LiteralPath $log -Tail 4
Write-Host "PRODUCTION REVERSE F5 CHECKS PASSED; disposable artifacts retained at $work"
