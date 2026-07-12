param()

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
Set-Location $root

& "$PSScriptRoot/build_og2.ps1"
if ($LASTEXITCODE -ne 0) { throw "OG-2 build failed" }

$flags = @(
    "-O3", "-mtune=native", "-mbmi", "-mbmi2", "-mpopcnt", "-mlzcnt",
    "-funroll-loops", "-std=c++20", "-pthread", "-I", "src"
)

function Invoke-Compile {
    param([string]$Source, [string]$Output)
    Write-Host "building $Output"
    & g++ @flags $Source "-o" $Output
    if ($LASTEXITCODE -ne 0) { throw "g++ failed for $Source" }
}

Invoke-Compile "src/main.cpp" "build/fj_sudoku.exe"
Invoke-Compile "src/reduce_main.cpp" "build/fj_reduce.exe"
Invoke-Compile "src/bench.cpp" "build/bench.exe"
Invoke-Compile "src/combinatorial2.cpp" "build/combinatorial2.exe"

Write-Host "built all active programs in build/"
