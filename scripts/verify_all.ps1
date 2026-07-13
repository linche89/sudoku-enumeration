param(
    [int]$Threads = 24,
    [int]$CanonTests = 500,
    [int]$HistTests = 500
)

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
Set-Location $root

function Invoke-Checked {
    param([string]$Label, [scriptblock]$Command)
    Write-Host "== $Label =="
    & $Command
    if ($LASTEXITCODE -ne 0) { throw "$Label failed" }
}

& "$PSScriptRoot/build_all.ps1"
if ($LASTEXITCODE -ne 0) { throw "build failed" }

& "$PSScriptRoot/verify_og2.ps1" -CanonTests $CanonTests -HistTests $HistTests
if ($LASTEXITCODE -ne 0) { throw "OG-2 short gate failed" }

foreach ($c in 2,3,4,5) {
    Invoke-Checked "factorization C=$c" {
        & ".\build\factorization_orbit.exe" $c pivot rooted4
    }
}

Invoke-Checked "future-twin per-class C=5" {
    & ".\build\factorization_orbit.exe" 5 futurecheck pivot rooted4
}

Invoke-Checked "future-twin joint C=5" {
    & ".\build\factorization_orbit.exe" 5 futurecheck futureorder=canonical-last pivot rooted4
}

Invoke-Checked "FJ9 full reproduction" {
    & ".\build\fj_sudoku.exe" --threads $Threads
}

Invoke-Checked "FJ9 independent combinatorial route" {
    & ".\build\combinatorial2.exe"
}

$checkpoint = "data\checkpoints\factorization_orbit_c6_graphmemo.bin"
if (Test-Path -LiteralPath $checkpoint) {
    $expectedHash = "FE8B68DE6C15592848D7CF69BF43928C000F0B2EE59FC263A094BCA2D376A865"
    $before = (Get-FileHash -Algorithm SHA256 -LiteralPath $checkpoint).Hash
    if ($before -ne $expectedHash) {
        throw "C=6 checkpoint hash does not match data/checkpoints/MANIFEST.md"
    }
    Invoke-Checked "C=6 checkpoint read-only" {
        & ".\build\factorization_orbit.exe" 6 limit=1 canonbudget=100 `
            canoncachecap=300000 pivotinner parallelparents rooted4 parentchunk=128 `
            "checkpoint=$checkpoint" checkpointreadonly
    }
    $after = (Get-FileHash -Algorithm SHA256 -LiteralPath $checkpoint).Hash
    if ($after -ne $before) { throw "read-only C=6 gate modified the checkpoint" }
} else {
    Write-Warning "C=6 checkpoint not present; skipped optional reload gate"
}

Write-Host "ALL REPOSITORY CHECKS PASSED"
