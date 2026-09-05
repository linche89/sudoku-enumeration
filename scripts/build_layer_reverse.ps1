param(
    [string]$ReverseExe = "build/layer_reverse_f5.exe",
    [string]$SupportTestExe = "build/layer_reverse_f5_support_test.exe",
    [string]$PrefixGateExe = "build/layer_two_missing_prefix_gate.exe"
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root
New-Item -ItemType Directory -Force -Path build | Out-Null

# Do not rebuild the independently released shared-F4 executable here.
$flags = @("-O3", "-mpopcnt", "-std=c++20", "-fopenmp", "-Wall", "-Wextra")
& g++ @flags experiments/proto/layer_reverse_f5.cpp -o $ReverseExe -lbcrypt -lpsapi
if ($LASTEXITCODE -ne 0) { throw "reverse F5 production build failed" }
& g++ @flags experiments/proto/layer_reverse_f5_support_test.cpp `
    -o $SupportTestExe -lbcrypt -lpsapi
if ($LASTEXITCODE -ne 0) { throw "reverse F5 read-only support test build failed" }
& g++ @flags experiments/proto/layer_two_missing_prefix_gate.cpp `
    -o $PrefixGateExe -lbcrypt -lpsapi
if ($LASTEXITCODE -ne 0) { throw "production-compatible prefix coefficient gate build failed" }
Write-Host "built reverse F5 engine, bounded support-loader test, and production-compatible prefix gate"
