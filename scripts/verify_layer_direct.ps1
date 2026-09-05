param(
    [ValidateRange(2, 24)]
    [int]$Threads = 4,
    [switch]$QuickCounter,
    [ValidateRange(1, 600)]
    [int]$SecondsPerProcess = 300,
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root

if (!$SkipBuild) {
    & "$PSScriptRoot/build_layer_dp.ps1"
    if ($LASTEXITCODE -ne 0) { throw "reference layer-DP build failed" }
    & "$PSScriptRoot/build_layer_direct.ps1"
    if ($LASTEXITCODE -ne 0) { throw "direct-layer build failed" }
}

$required = @(
    "layer_dp_gate", "layer_direct_bench", "layer_native_gather_bench",
    "layer_support_probe", "layer_canonical_batch", "layer_pairing_fiber_bench",
    "layer_catalog_lookup_bench"
)
foreach ($name in $required) {
    if (!(Test-Path -LiteralPath "build/$name.exe")) {
        throw "Required verification executable is missing: build/$name.exe"
    }
}

# Retain only new small-C fixtures and bounded synthetic C=6 certificates.
# No cleanup or production checkpoint discovery occurs in this script.
$logRoot = Join-Path $root "data/logs"
if (!(Test-Path -LiteralPath $logRoot)) {
    New-Item -ItemType Directory -Path $logRoot | Out-Null
}
$work = Join-Path $logRoot ("layer-direct-release-gate-" + [guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Path $work | Out-Null
Write-Host "ARTIFACTS $work"

function Invoke-LoggedPython {
    param([string]$Label, [string[]]$Arguments, [string]$LogName)
    $log = Join-Path $work $LogName
    if (Test-Path -LiteralPath $log) { throw "Verification log already exists: $log" }
    Write-Host "checking $Label"
    & python @Arguments *> $log
    if ($LASTEXITCODE -ne 0) {
        Get-Content -LiteralPath $log -Tail 80
        throw "$Label failed; full log: $log"
    }
    Get-Content -LiteralPath $log -Tail 2
}

$fixtureDir = Join-Path $work "small-c"
Invoke-LoggedPython "complete C=2..5 direct, native gather, fibers, and parallel values" @(
    "experiments/proto/layer_direct_gate.py",
    "--max-c", "5", "--output", $fixtureDir,
    "--seconds-per-process", "$SecondsPerProcess", "--parallel-threads", "$Threads",
    "--layer-exe", (Join-Path $root "build/layer_dp_gate.exe"),
    "--bench-exe", (Join-Path $root "build/layer_direct_bench.exe"),
    "--native-exe", (Join-Path $root "build/layer_native_gather_bench.exe"),
    "--fiber-exe", (Join-Path $root "build/layer_pairing_fiber_bench.exe"),
    "--support-exe", (Join-Path $root "build/layer_support_probe.exe"),
    "--canonical-exe", (Join-Path $root "build/layer_canonical_batch.exe"),
    "--catalog-exe", (Join-Path $root "build/layer_catalog_lookup_bench.exe")
) "small-c-driver.log"

$counterMax = if ($QuickCounter) { "5" } else { "6" }
Invoke-LoggedPython "two-missing-box Burnside census through C=$counterMax" @(
    "experiments/proto/layer_two_missing_burnside.py", "--max-c", $counterMax,
    "--self-test", "--time-limit", "180", "--max-states", "5000"
) "two-missing-counter.log"

Invoke-LoggedPython "constructive C=6 support witness fixture" @(
    "experiments/proto/layer_support_witnesses.py", "--time-limit", "30",
    "--fixture", "data/golden/og2-c6-support-witnesses.json"
) "support-witnesses.log"

Write-Host "DIRECT LAYER CHECKS PASSED; artifacts retained at $work"
if ($QuickCounter) {
    Write-Host "Optional full C=6 Burnside census skipped by -QuickCounter."
}
