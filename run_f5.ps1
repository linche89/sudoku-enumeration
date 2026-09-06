[CmdletBinding()]
param(
    [switch]$CheckOnly,
    [ValidateRange(1,450)] [int]$WorkMinutes = 330,
    [ValidateRange(2,480)] [int]$MaxMinutes = 360
)

# Same command on every session: gates -> F4 export if needed -> canary -> F5.
# No production executable is rebuilt, no old checkpoint is overwritten.
# Each session is bounded; committed F5 chunks resume without recomputation.
# Optional final suffix window: -WorkMinutes 450 -MaxMinutes 480.
# The engine stops early when F5 closes; these are limits, not minimum times.
# This does not export L5 or perform the final N(6) outer contraction.
$ErrorActionPreference = 'Stop'

function Require-File([string]$Path) {
    if (!(Test-Path -LiteralPath $Path -PathType Leaf)) { throw "Missing required file: $Path" }
    return Get-Item -LiteralPath $Path
}
function Require-Sha([string]$Path, [string]$Expected) {
    $null = Require-File $Path
    if ((Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash -ne $Expected) {
        throw "SHA-256 differs from the qualified input/build: $Path"
    }
}
function Invoke-Logged([string]$Program, [string[]]$Items, [string]$LogPath, [switch]$AllowBoundStop) {
    if (Test-Path -LiteralPath $LogPath) { throw "Refusing log overwrite: $LogPath" }
    Write-Host "Running stage; log: $LogPath"
    $savedPreference = $ErrorActionPreference
    try {
        # Wait for a native controller's final backup even if it writes stderr.
        $ErrorActionPreference = 'Continue'
        & $Program @Items 2>&1 | Tee-Object -FilePath $LogPath -ErrorAction Stop | Out-Host
        $code = $LASTEXITCODE
    } finally { $ErrorActionPreference = $savedPreference }
    if ($code -ne 0 -and !($AllowBoundStop -and $code -eq 98)) {
        throw "Stage exited $code. Stop and inspect $LogPath; retain all files."
    }
    return $code
}
function Read-F5Window([string]$LogPath, [int]$ExitCode, [long]$MaxNew) {
    $text = Get-Content -LiteralPath $LogPath -Raw
    $ends = [regex]::Matches($text, '(?m)^REVERSE_WINDOW_END exit=([0-9]+) logs=(.+) N6=NOT_COMPUTED\r?$')
    if ($ends.Count -ne 1 -or [int]$ends[0].Groups[1].Value -ne $ExitCode) {
        throw "Missing F5 terminal/controller binding: $LogPath"
    }
    $copies = [regex]::Matches($text, '(?m)^PHYSICAL_BACKUP phase=after files=([0-9]+) directory=(.+)\r?$')
    if ($copies.Count -ne 1) { throw "No accepted final backup marker; retain files and inspect: $LogPath" }
    $receiptPath = Join-Path $copies[0].Groups[2].Value.Trim() 'receipt.csv'
    $null = Require-File $receiptPath
    if ($ExitCode -eq 98) {
        Write-Warning 'Time/RSS bound reached (sleep may trigger this after wake). The controller completed its backup.'
        Write-Host 'This is NOT a successful computation window. Run the SAME command to validate and resume committed chunks.'
        return $null
    }
    $engineLogDir = $ends[0].Groups[2].Value.Trim()
    if ((Get-Item -LiteralPath (Join-Path $engineLogDir 'stderr.log')).Length -ne 0) {
        throw "F5 stderr needs review: $engineLogDir"
    }
    $engineText = Get-Content -LiteralPath (Join-Path $engineLogDir 'stdout.log') -Raw
    $summaries = [regex]::Matches($engineText, '(?m)^SUMMARY .+$')
    if ($summaries.Count -ne 1) { throw "Missing unique F5 summary: $engineLogDir" }
    $fields = @{}
    foreach ($match in [regex]::Matches($summaries[0].Value, '(\w+)=([^\s]+)')) {
        if ($fields.ContainsKey($match.Groups[1].Value)) { throw 'Duplicate F5 summary field' }
        $fields[$match.Groups[1].Value] = $match.Groups[2].Value
    }
    foreach ($key in @('closed_prefix', 'total_entries', 'new_indices', 'peak_rss_bytes')) {
        if ($fields[$key] -notmatch '^[0-9]+$') { throw "Invalid F5 numeric summary field: $key" }
    }
    $prefix = [long]$fields.closed_prefix
    if ([long]$fields.total_entries -ne 96452976 -or $prefix -lt 0 -or $prefix -gt 96452976 -or
        ($prefix -ne 96452976 -and $prefix % 10000 -ne 0) -or
        [long]$fields.new_indices -gt $MaxNew -or [long]$fields.peak_rss_bytes -gt 55GB -or
        $fields.source_checkpointreadonly -ne 'yes' -or $fields.N6 -ne 'NOT_COMPUTED' -or
        [long]$copies[0].Groups[1].Value -ne ([long][Math]::Ceiling($prefix/10000.0)+1)) {
        throw "F5 result/backup/resource checks failed: $engineLogDir"
    }
    $expectedStatus = if ($prefix -eq 96452976) { 'CLOSED_F5_CATALOGUE' } else { 'INCOMPLETE_RESUMABLE' }
    if ($fields.status -ne $expectedStatus) { throw 'F5 scope marker differs from its closed prefix' }
    $backupRows = @(Import-Csv -LiteralPath $receiptPath)
    if ($backupRows.Count -ne [long]$copies[0].Groups[1].Value) { throw 'F5 backup receipt inventory count differs' }
    Write-Host ('F5 saved IDs: {0}/96452976 ({1:F4}%).' -f $prefix, (100.0*$prefix/96452976))
    return [pscustomobject]@{Prefix=$prefix; NewIndices=[long]$fields.new_indices; Logs=$engineLogDir}
}
function Read-ExportReceipt([string]$Path, [string]$Local, [string]$Copy, [string]$ChunkPath, [string]$ChunkCopy) {
    $null = Require-File $Path
    $value = Get-Content -LiteralPath $Path -Raw | ConvertFrom-Json
    if ($value.schema -ne 'shared-f4-closed-export-v1' -or $value.c -ne 6 -or $value.layer -ne 4 -or
        $value.entries -ne 903398621 -or $value.live -ne 903398603 -or $value.holes -ne 18 -or
        $value.orbit_mass -ne 41602261536160 -or $value.bytes -ne 32522350484 -or
        $value.new_chunks -ne 0 -or $value.new_indices -ne 0 -or
        $value.alias_closure -ne 'producer_verified' -or
        $value.serialization -ne 'independent_full_SHA_header_payload_count_mass_readback' -or
        $value.output -ne $Local -or $value.backup -ne $Copy -or
        $value.namespace -ne $ChunkPath -or $value.namespace_backup -ne $ChunkCopy -or
        $value.source_sha256 -ne 'ECF0837315B0FDF8AE21C394FDA6676490E43B1828A0825529A344EC17E4E844' -or
        $value.manifest_sha256 -ne '848B9DC72452AB389C5BF5424E264AE86C673C7FBA673F5F14FDC845BF4DEB91' -or
        $value.producer_sha256 -ne '2B8A14606F8F78748C9338130D35551CA199E53238BEB33D95BCB05A2555AFE0' -or
        $value.reader_sha256 -ne '5E92E26EA5D2516B7B0A37C6C6CCEF3F7C42E55EFEA1576857A3264642888961' -or
        $value.sha256 -notmatch '^[A-F0-9]{64}$') {
        throw 'The existing F4 export receipt is not the verified, source-bound complete export.'
    }
    foreach ($file in @($Local, $Copy)) {
        if ((Require-File $file).Length -ne 32522350484) { throw "Export length differs: $file" }
    }
    return $value # The F5 controller rehashes BOTH full files before starting.
}

Push-Location -LiteralPath $PSScriptRoot
$manualMutex = $null
$ownsManualMutex = $false
try {
    if ($WorkMinutes -ge $MaxMinutes) { throw 'WorkMinutes must be below MaxMinutes.' }
    $manualMutex = [Threading.Mutex]::new($false, 'Local\SudokuFJ-F5-Manual')
    try { $ownsManualMutex = $manualMutex.WaitOne(0) }
    catch [Threading.AbandonedMutexException] { $ownsManualMutex = $true }
    if (!$ownsManualMutex) { throw 'Another run_f5 launcher holds the session lock.' }
    $backupRoot = 'D:\sudoku_FJ_checkpoint_backups'
    $stamp = '20260906-085138-8974f5a375294f26852c7ac7392947d9-after'
    $namespace = Join-Path $PSScriptRoot 'data\checkpoints\c6_shared_f4_20260905'
    $namespaceBackup = Join-Path $backupRoot "c6_shared_f4_20260905\$stamp"
    $sourceName = 'session-008-20260810-170245-layer_dp_c6_s1_prod_20260802.a'
    $sourceBackup = Join-Path $backupRoot "layer_dp_c6_s1_prod_20260802\$sourceName"
    $layer4 = Join-Path $PSScriptRoot 'data\checkpoints\c6_shared_f4_closed_20260906.L4.snap'
    $layer4Backup = Join-Path $backupRoot 'c6_shared_f4_exports\c6_shared_f4_closed_20260906.L4.snap'
    $exportReceipt = $layer4 + '.receipt.json'
    $f5Namespace = Join-Path $PSScriptRoot 'data\checkpoints\c6_reverse_f5_20260905'
    $supportBackup = Join-Path $backupRoot 'c6_layer5_support_20260905\ck.L5.rehearsal.snap'
    $auditPath = 'data/logs/c6-direct-route-20260905/shared-complete-independent-audit-20260906.json'
    $controllerPattern = '(?i)(?:^|\s)-File\s+"?(?:[^"\r\n]*[\\/])?(?:run_continue|run_f5|run_layer_shared_window|export_layer_shared|run_layer_reverse_window)\.ps1(?:"|\s|$)'
    $busy = @(Get-CimInstance Win32_Process | Where-Object {
        $_.ProcessId -ne $PID -and (
            $_.Name -in @('layer_shared_f4.exe', 'layer_reverse_f5.exe', 'layer_dp_gate.exe') -or
            ($_.Name -in @('powershell.exe', 'pwsh.exe') -and
             $_.CommandLine -notmatch '(?i)(?:^|\s)-(?:Command|EncodedCommand|c|ec)\b' -and
             $_.CommandLine -match $controllerPattern)
        )
    })
    if ($busy.Count) { throw "Another computation/controller is active (PID: $($busy.ProcessId -join ', '))." }
    Require-Sha $auditPath '8FAA333C7F4CC38D6733AA94A3DCFE31D2E469D602B3A212065DE70EAF001741'
    $audit = Get-Content -LiteralPath $auditPath -Raw | ConvertFrom-Json
    if ($audit.status -ne 'PASS' -or !$audit.complete_domain_closed -or $audit.closed_prefix -ne 903398621) {
        throw 'The complete F4 audit is missing or incomplete.'
    }
    Require-Sha (Join-Path $namespace 'manifest.bin') '848B9DC72452AB389C5BF5424E264AE86C673C7FBA673F5F14FDC845BF4DEB91'
    Require-Sha (Join-Path $namespaceBackup 'receipt.csv') 'C0FBBDF098186FCA77E6C47E2CA64A2FD6DA60151FB51E08CB3B1ECC1EC6284E'
    Require-Sha 'build/layer_shared_f4.exe' '2B8A14606F8F78748C9338130D35551CA199E53238BEB33D95BCB05A2555AFE0'
    Require-Sha 'build/layer_reverse_f5.exe' 'E09123878A3098EFF955AA4DA1CCAF33B875F9CCEF7D9F337DF28F3ED08AFB9B'
    Require-Sha 'build/layer_support_probe.exe' '5E92E26EA5D2516B7B0A37C6C6CCEF3F7C42E55EFEA1576857A3264642888961'
    foreach ($file in @($sourceBackup, 'data/checkpoints/layer_dp_c6_s1_prod_20260802.a')) {
        if ((Require-File $file).Length -ne 32522350448) { throw "Original source size differs: $file" }
    }
    foreach ($file in @($supportBackup, 'data/logs/layer-dp-c6-e2e-rehearsal-20260802-1pct/s2-4to5/ck.L5.snap')) {
        if ((Require-File $file).Length -ne 3472307192) { throw "L5 support size differs: $file" }
    }
    foreach ($file in @('scripts/prepare_layer_f5_manual_gates.py', 'scripts/export_layer_shared.ps1', 'scripts/run_layer_reverse_window.ps1')) {
        $null = Require-File $file
    }
    $closed = $null
    if (Test-Path -LiteralPath $exportReceipt) {
        $closed = Read-ExportReceipt $exportReceipt $layer4 $layer4Backup $namespace $namespaceBackup
    } elseif ((Test-Path -LiteralPath $layer4) -or (Test-Path -LiteralPath $layer4Backup)) {
        throw 'An export file exists without its accepted receipt. Retain it and request recovery review; no overwrite.'
    }
    if (!$closed) {
        foreach ($target in @($layer4, $layer4Backup)) {
            $drive = [IO.DriveInfo]::new([IO.Path]::GetPathRoot($target))
            if ($drive.AvailableFreeSpace -lt 36GB) { throw "Need at least 36 GiB free for export: $target" }
        }
    }
    $powerShell = (Get-Command powershell.exe -CommandType Application -ErrorAction Stop | Select-Object -First 1).Source
    $python = (Get-Command python.exe -CommandType Application -ErrorAction Stop | Select-Object -First 1).Source
    & $python -c 'import psutil'
    if ($LASTEXITCODE -ne 0) { throw 'The Python environment needs psutil for bounded gate execution.' }
    Write-Host 'Preflight OK. Fresh verification gates run automatically (360s / 6 GiB per gate).'
    Write-Host 'Then: protected complete F4 export/readback/backup, unless its valid receipt already exists.'
    Write-Host 'Then: resume check/canary of up to 10000 new IDs, 8-minute soft / 10-minute hard child limit.'
    Write-Host "After canary/backup checks: regular F5 window, 24 threads, 55 GiB, $WorkMinutes-minute soft / $MaxMinutes-minute hard limit."
    Write-Host 'Rerun this SAME script on later days or after a bounded stop. Only missing chunks are computed.'
    Write-Host 'Export child limit: 30 minutes; independent readback: 300 seconds. Hashing/backups add time.'
    Write-Host 'Keep the PC awake and this window open until the final backup completes.'
    if ($CheckOnly) {
        Write-Host 'CHECK_ONLY_PASS: no gates, export, computation or backup started. Full input hashes remain for the controllers.'
        return
    }

    $runDir = Join-Path $PSScriptRoot ('data/logs/f5-manual-' + [guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Path $runDir | Out-Null
    Write-Host "SESSION LOGS: $runDir"
    $gateDir = Join-Path $runDir 'gates'
    $null = Invoke-Logged $python @('scripts/prepare_layer_f5_manual_gates.py', '--output-dir', $gateDir) (Join-Path $runDir 'gates-controller.log')
    $gates = Get-Content -LiteralPath (Join-Path $gateDir 'receipt.json') -Raw | ConvertFrom-Json
    if ($gates.status -ne 'PASS') { throw 'Fresh gate qualification failed; no export/F5 launch.' }
    foreach ($name in @('full', 'direct', 'shared', 'reverse')) {
        Require-Sha $gates.gates.$name.path $gates.gates.$name.sha256
    }
    if (!$closed) {
        New-Item -ItemType Directory -Path (Split-Path -Parent $layer4Backup) -Force | Out-Null
        $exportArgs = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', 'scripts/export_layer_shared.ps1',
            '-FullGateEvidence', $gates.gates.full.path, '-SharedGateEvidence', $gates.gates.shared.path,
            '-DirectGateEvidence', $gates.gates.direct.path, '-SourceBackup', $sourceBackup,
            '-NamespacePath', $namespace, '-NamespaceBackup', $namespaceBackup,
            '-ManifestSha256', '848B9DC72452AB389C5BF5424E264AE86C673C7FBA673F5F14FDC845BF4DEB91',
            '-OutputPath', $layer4, '-OutputBackup', $layer4Backup,
            '-MaxMinutes', '30', '-AuditMaxSeconds', '300', '-LimitGiB', '55', '-Threads', '24', '-Chunk', '25000')
        $null = Invoke-Logged $powerShell $exportArgs (Join-Path $runDir 'export-controller.log')
        $closed = Read-ExportReceipt $exportReceipt $layer4 $layer4Backup $namespace $namespaceBackup
    }
    $f5BaseArgs = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', 'scripts/run_layer_reverse_window.ps1',
        '-FullGateEvidence', $gates.gates.full.path, '-ReverseGateEvidence', $gates.gates.reverse.path,
        '-Layer4', $layer4, '-Layer4Backup', $layer4Backup, '-Layer4Sha256', $closed.sha256,
        '-SupportBackup', $supportBackup, '-NamespacePath', $f5Namespace,
        '-BackupRoot', (Join-Path $backupRoot 'c6_reverse_f5_20260905'),
        '-Chunk', '10000', '-Threads', '24', '-LimitGiB', '55')
    $canaryLog = Join-Path $runDir 'f5-canary-controller.log'
    $code = Invoke-Logged $powerShell ($f5BaseArgs + @('-Limit','10000','-WorkMinutes','8','-MaxMinutes','10')) $canaryLog -AllowBoundStop
    $canary = Read-F5Window $canaryLog $code 10000
    if ($null -eq $canary) { return }
    if ($canary.Prefix -lt 96452976) {
        if ($canary.NewIndices -ne 10000) { throw 'Canary did not close the requested new work; inspect logs before a longer run.' }
        $windowLog = Join-Path $runDir 'f5-window-controller.log'
        $code = Invoke-Logged $powerShell ($f5BaseArgs + @('-Limit','96452976','-WorkMinutes',"$WorkMinutes",'-MaxMinutes',"$MaxMinutes")) $windowLog -AllowBoundStop
        $window = Read-F5Window $windowLog $code 96452976
        if ($null -eq $window) { return }
    } else { $window = $canary }
    if ($window.Prefix -eq 96452976) {
        Write-Host 'F5 CATALOGUE COMPLETE AND BACKED UP. Stop here for independent audit, L5 export and the final N(6) stage.'
    } else {
        Write-Host 'SESSION FINISHED AND BACKED UP. Run the SAME command next time to resume the saved prefix.'
    }
    Write-Host "SESSION LOGS: $runDir"
    Write-Host 'N(6) is not computed by this script.'
} finally {
    if ($ownsManualMutex) { $manualMutex.ReleaseMutex() }
    if ($null -ne $manualMutex) { $manualMutex.Dispose() }
    Pop-Location
}
