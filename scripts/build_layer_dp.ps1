param()

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
Set-Location $root
New-Item -ItemType Directory -Force -Path build | Out-Null

# Keep the Windows/MinGW build on the repository's known-safe ISA baseline.
# In particular, do not add -march=native: the workstation has a documented
# AVX stack-alignment failure under that flag.
$flags = @(
    "-O3", "-mpopcnt", "-std=c++20", "-fopenmp",
    "-Wall", "-Wextra"
)

Write-Host "building build/layer_dp_gate.exe"
& g++ @flags "experiments/proto/layer_dp_gate.cpp" "-o" `
    "build/layer_dp_gate.exe" "-lpsapi"
if ($LASTEXITCODE -ne 0) {
    throw "g++ failed for experiments/proto/layer_dp_gate.cpp"
}

Write-Host "built layer-DP gate engine"
