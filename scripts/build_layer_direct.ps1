param(
    [switch]$AllowPendingTools
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root
New-Item -ItemType Directory -Force -Path build | Out-Null

# Match the verified Windows/MinGW baseline; never add -march=native.
$flags = @("-O3", "-mpopcnt", "-std=c++20", "-fopenmp", "-Wall", "-Wextra")
$programs = @(
    @{ Name = "layer_direct_bench"; Pending = $false; Libraries = @("-lpsapi") },
    @{ Name = "layer_native_gather_bench"; Pending = $false; Libraries = @("-lpsapi") },
    @{ Name = "layer_support_probe"; Pending = $false; Libraries = @("-lbcrypt", "-lpsapi") },
    @{ Name = "layer_canonical_batch"; Pending = $false; Libraries = @("-lpsapi") },
    @{ Name = "layer_pairing_fiber_bench"; Pending = $true; Libraries = @("-lpsapi") },
    @{ Name = "layer_catalog_lookup_bench"; Pending = $true; Libraries = @("-lbcrypt", "-lpsapi") }
)

# Release/default builds require every tool. The explicit development switch
# is only for working while a bounded decision prototype is still being added.
foreach ($program in $programs) {
    $source = "experiments/proto/$($program.Name).cpp"
    if (!(Test-Path -LiteralPath $source)) {
        if ($AllowPendingTools -and $program.Pending) { continue }
        throw "Required direct-layer source is missing: $source"
    }
}

$built = 0
foreach ($program in $programs) {
    $source = "experiments/proto/$($program.Name).cpp"
    if (!(Test-Path -LiteralPath $source)) {
        Write-Warning "Development-only skip of pending tool: $($program.Name)"
        continue
    }
    $output = "build/$($program.Name).exe"
    $libraries = @($program.Libraries)
    Write-Host "building $output"
    & g++ @flags $source "-o" $output @libraries
    if ($LASTEXITCODE -ne 0) { throw "g++ failed for $source" }
    $built++
}
Write-Host "built $built direct-layer support and decision tools"
