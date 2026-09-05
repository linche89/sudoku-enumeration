param()

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root
New-Item -ItemType Directory -Force -Path build | Out-Null

# The graph kernel is a separate translation unit: both engines keep their
# own dimension globals, and the retained numerical kernel is not copied.
$flags = @("-O3", "-mpopcnt", "-std=c++20", "-fopenmp", "-Wall", "-Wextra")
& g++ @flags experiments/proto/layer_shared_f4.cpp experiments/proto/layer_shared_f4_bridge.cpp `
    -o build/layer_shared_f4.exe -lbcrypt -lpsapi
if ($LASTEXITCODE -ne 0) { throw "shared F4 build failed" }
Write-Host "built build/layer_shared_f4.exe"
& g++ @flags experiments/proto/layer_reverse_f5_bench.cpp `
    -o build/layer_reverse_f5_bench.exe -lbcrypt -lpsapi
if ($LASTEXITCODE -ne 0) { throw "reverse F5 core benchmark build failed" }
Write-Host "built build/layer_reverse_f5_bench.exe"
