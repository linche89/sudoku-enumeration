param()

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root
New-Item -ItemType Directory -Force -Path build | Out-Null

# Do not rebuild the independently released shared-F4 executable here.
$flags = @("-O3", "-mpopcnt", "-std=c++20", "-fopenmp", "-Wall", "-Wextra")
& g++ @flags experiments/proto/layer_reverse_f5.cpp -o build/layer_reverse_f5.exe -lbcrypt -lpsapi
if ($LASTEXITCODE -ne 0) { throw "reverse F5 production build failed" }
& g++ @flags experiments/proto/layer_reverse_f5_support_test.cpp `
    -o build/layer_reverse_f5_support_test.exe -lbcrypt -lpsapi
if ($LASTEXITCODE -ne 0) { throw "reverse F5 read-only support test build failed" }
Write-Host "built reverse F5 engine and bounded support-loader test"
