param(
    [ValidateRange(2, 24)]
    [int]$Threads = 4,
    [ValidateRange(30, 600)]
    [int]$SecondsPerProcess = 180,
    [string]$C6SampleText = "",
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root
if (!$SkipBuild) {
    & "$PSScriptRoot/build_layer_shared.ps1"
    if ($LASTEXITCODE -ne 0) { throw "shared-layer build failed" }
}
foreach ($name in @("layer_shared_f4", "layer_dp_gate", "layer_native_gather_bench", "layer_reverse_f5_bench")) {
    if (!(Test-Path -LiteralPath "build/$name.exe")) {
        throw "Required gate executable is missing: build/$name.exe"
    }
}
$logRoot = Join-Path $root "data/logs"
if (!(Test-Path -LiteralPath $logRoot)) { New-Item -ItemType Directory -Path $logRoot | Out-Null }
$work = Join-Path $logRoot ("layer-shared-release-gate-" + [guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Path $work | Out-Null
$log = Join-Path $work "driver.log"
Write-Host "ARTIFACTS $work"
$driverArgs = @("experiments/proto/layer_shared_gate.py", "--threads", "$Threads",
    "--seconds-per-process", "$SecondsPerProcess", "--output", (Join-Path $work "fixtures"))
if ($C6SampleText) { $driverArgs += @("--c6-sample", $C6SampleText) }
& python @driverArgs *> $log
if ($LASTEXITCODE -ne 0) {
    Get-Content -LiteralPath $log -Tail 60
    throw "Shared F4 gate failed; full log: $log"
}
Get-Content -LiteralPath $log -Tail 4
Write-Host "SHARED LAYER CHECKS PASSED; disposable artifacts retained at $work"
