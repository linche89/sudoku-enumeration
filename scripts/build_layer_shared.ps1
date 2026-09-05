param([switch]$ReferenceKernel)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root
New-Item -ItemType Directory -Force -Path build | Out-Null

# The graph kernel remains a separate translation unit. The original bridge
# is retained as an explicit fallback and independent differential reference.
$flags = @("-O3", "-mpopcnt", "-std=c++20", "-fopenmp", "-Wall", "-Wextra")
$bridge = if ($ReferenceKernel) {
    "experiments/proto/layer_shared_f4_bridge.cpp"
} else {
    "experiments/proto/layer_shared_f4_incremental_bridge.cpp"
}
& g++ @flags experiments/proto/layer_shared_f4.cpp $bridge `
    -o build/layer_shared_f4.exe -lbcrypt -lpsapi
if ($LASTEXITCODE -ne 0) { throw "shared F4 build failed" }
Write-Host "built build/layer_shared_f4.exe using $bridge"
& g++ @flags experiments/proto/layer_reverse_f5_bench.cpp `
    -o build/layer_reverse_f5_bench.exe -lbcrypt -lpsapi
if ($LASTEXITCODE -ne 0) { throw "reverse F5 core benchmark build failed" }
Write-Host "built build/layer_reverse_f5_bench.exe"
