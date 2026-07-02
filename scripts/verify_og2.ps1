param(
    [int]$CanonTests = 500,
    [int]$HistTests = 500
)

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
Set-Location $root

if (!(Test-Path "build/mp_fast.exe") -or !(Test-Path "build/mp_c6.exe") -or !(Test-Path "build/ms_fast.exe")) {
    & "$PSScriptRoot/build_og2.ps1"
}

function Invoke-Checked {
    param([string]$Label, [scriptblock]$Command)
    Write-Host "== $Label =="
    & $Command
    if ($LASTEXITCODE -ne 0) {
        throw "$Label failed"
    }
}

Invoke-Checked "ms_fast C=2" { & ".\build\ms_fast.exe" 2 }
Invoke-Checked "ms_fast C=3" { & ".\build\ms_fast.exe" 3 }
Invoke-Checked "mp_fast C=4" { & ".\build\mp_fast.exe" 4 }
Invoke-Checked "mp_c6 C=4" { & ".\build\mp_c6.exe" 4 }

Invoke-Checked "mp_fast histogram difftest C=4" { & ".\build\mp_fast.exe" 4 difftest $HistTests }
Invoke-Checked "mp_c6 refinement canontest C=4" { & ".\build\mp_c6.exe" 4 canontest $CanonTests }

Write-Host "OG-2 short verification passed"
