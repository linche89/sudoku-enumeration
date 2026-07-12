param(
    [switch]$Clean
)

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
Set-Location $root

if ($Clean -and (Test-Path build)) {
    Remove-Item -LiteralPath build -Recurse -Force
}
New-Item -ItemType Directory -Force -Path build | Out-Null

$common = @("-O3", "-mpopcnt", "-std=c++20", "-I", "src")
$omp = @("-fopenmp")

function Invoke-Compile {
    param(
        [string]$Source,
        [string]$Output,
        [string[]]$Extra = @()
    )
    Write-Host "building $Output"
    & g++ @common @Extra $Source "-o" $Output
    if ($LASTEXITCODE -ne 0) {
        throw "g++ failed for $Source"
    }
}

Invoke-Compile "src/multiset_fast.cpp" "build/ms_fast.exe"
Invoke-Compile "src/multiset_par.cpp" "build/mp_fast.exe" $omp
Invoke-Compile "src/multiset_c6.cpp" "build/mp_c6.exe" $omp
Invoke-Compile "src/multiset_q.cpp" "build/mp_q.exe" $omp
Invoke-Compile "src/canon_refine2.cpp" "build/canon_refine2.exe"
Invoke-Compile "src/factorization_orbit.cpp" "build/factorization_orbit.exe" $omp

Write-Host "built OG-2 tools in build/"
