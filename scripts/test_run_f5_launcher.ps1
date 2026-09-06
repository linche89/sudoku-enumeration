# Synthetic receipt-parser tests. Do not execute the launcher's main body.
$ErrorActionPreference = 'Stop'
$tokens = $null
$errors = $null
$path = Join-Path (Split-Path -Parent $PSScriptRoot) 'run_f5.ps1'
$ast = [System.Management.Automation.Language.Parser]::ParseFile($path, [ref]$tokens, [ref]$errors)
if ($errors.Count) { throw ($errors | Out-String) }

# Exercise only the real parameter block and ordering guard, never main.
$timeGuard = $ast.Find({ param($node)
    $node -is [System.Management.Automation.Language.IfStatementAst] -and
    $node.Extent.Text -eq "if (`$WorkMinutes -ge `$MaxMinutes) { throw 'WorkMinutes must be below MaxMinutes.' }"
}, $true)
if (!$timeGuard) { throw 'Time ordering guard missing' }
$boundsFixture = [scriptblock]::Create($ast.ParamBlock.Extent.Text + "`n" + $timeGuard.Extent.Text + @'

[pscustomobject]@{WorkMinutes=$WorkMinutes; MaxMinutes=$MaxMinutes; CheckOnly=[bool]$CheckOnly}
'@)
$boundsAccepted = 0
foreach ($case in @(
    @{ Args=@{}; Work=330; Hard=360 },
    @{ Args=@{WorkMinutes=450; MaxMinutes=480}; Work=450; Hard=480 },
    @{ Args=@{WorkMinutes=450; MaxMinutes=480; CheckOnly=$true}; Work=450; Hard=480 },
    @{ Args=@{WorkMinutes=45; MaxMinutes=60}; Work=45; Hard=60 },
    @{ Args=@{WorkMinutes=1; MaxMinutes=2}; Work=1; Hard=2 }
)) {
    $caseArgs = $case.Args
    $actual = & $boundsFixture @caseArgs
    if ($actual.WorkMinutes -ne $case.Work -or $actual.MaxMinutes -ne $case.Hard) { throw 'Wrong time defaults/binding' }
    $boundsAccepted++
}
$boundsRejected = 0
foreach ($caseArgs in @(
    @{WorkMinutes=451; MaxMinutes=480}, @{WorkMinutes=450; MaxMinutes=481},
    @{WorkMinutes=0; MaxMinutes=480}, @{WorkMinutes=1; MaxMinutes=1},
    @{WorkMinutes=450; MaxMinutes=450}, @{WorkMinutes=450; MaxMinutes=449},
    @{WorkMinutes=450}
)) {
    $caught = $false
    try { $null = & $boundsFixture @caseArgs } catch { $caught = $true }
    if (!$caught) { throw 'Invalid synthetic time bounds accepted' }
    $boundsRejected++
}
# The unchanged child controller must also bind the advertised long window.
$controllerPath = Join-Path $PSScriptRoot 'run_layer_reverse_window.ps1'
$controllerAst = [System.Management.Automation.Language.Parser]::ParseFile($controllerPath, [ref]$tokens, [ref]$errors)
if ($errors.Count) { throw ($errors | Out-String) }
$childFixture = [scriptblock]::Create($controllerAst.ParamBlock.Extent.Text + @'

[pscustomobject]@{WorkMinutes=$WorkMinutes; MaxMinutes=$MaxMinutes; LimitGiB=$LimitGiB; Threads=$Threads; Limit=$Limit; Chunk=$Chunk}
'@)
$childArgs = @{
    FullGateEvidence='SYNTHETIC'; ReverseGateEvidence='SYNTHETIC';
    Layer4='SYNTHETIC'; Layer4Backup='SYNTHETIC'; Layer4Sha256=('A'*64);
    WorkMinutes=450; MaxMinutes=480; Limit=96452976; Chunk=10000
}
$childBounds = & $childFixture @childArgs
if ($childBounds.WorkMinutes -ne 450 -or $childBounds.MaxMinutes -ne 480 -or
    $childBounds.LimitGiB -ne 55 -or $childBounds.Threads -ne 24 -or
    $childBounds.Limit -ne 96452976 -or $childBounds.Chunk -ne 10000) { throw 'Child bounds differ from launcher' }
Write-Host "F5_TIME_BOUNDS_SYNTHETIC_PASS accepted=$boundsAccepted rejected=$boundsRejected child_450_480=accepted production_IO=none"

$definition = $ast.Find({ param($node)
    $node -is [System.Management.Automation.Language.FunctionDefinitionAst] -and $node.Name -eq 'Read-ExportReceipt'
}, $true)
if (!$definition) { throw 'Receipt validator missing' }

& {
    . ([scriptblock]::Create($definition.Extent.Text))
    $fixtureLength = [long]32522350484
    function Require-File([string]$Path) { return [pscustomobject]@{Length=$fixtureLength} }
    function Get-Content([string]$LiteralPath, [switch]$Raw) { return $fixtureJson }
    $fixture = @{
        schema='shared-f4-closed-export-v1'; c=6; layer=4; entries=903398621;
        live=903398603; holes=18; orbit_mass=[long]41602261536160; bytes=[long]32522350484;
        new_chunks=0; new_indices=0; alias_closure='producer_verified';
        serialization='independent_full_SHA_header_payload_count_mass_readback';
        output='E:\finite-fixture\closed.snap'; backup='D:\finite-fixture\closed.snap';
        namespace='E:\finite-fixture\chunks'; namespace_backup='D:\finite-fixture\chunks';
        source_sha256='ECF0837315B0FDF8AE21C394FDA6676490E43B1828A0825529A344EC17E4E844';
        manifest_sha256='848B9DC72452AB389C5BF5424E264AE86C673C7FBA673F5F14FDC845BF4DEB91';
        producer_sha256='2B8A14606F8F78748C9338130D35551CA199E53238BEB33D95BCB05A2555AFE0';
        reader_sha256='5E92E26EA5D2516B7B0A37C6C6CCEF3F7C42E55EFEA1576857A3264642888961';
        sha256=('A'*64)
    }
    function Invoke-Fixture {
        Read-ExportReceipt 'NOT_A_FILE' 'E:\finite-fixture\closed.snap' 'D:\finite-fixture\closed.snap' `
            'E:\finite-fixture\chunks' 'D:\finite-fixture\chunks'
    }
    $fixtureJson = $fixture | ConvertTo-Json
    $null = Invoke-Fixture
    $rejected = 0
    foreach ($field in $fixture.Keys) {
        $bad = $fixture.Clone()
        $bad[$field] = if ($bad[$field] -is [string]) { 'INVALID' } else { $bad[$field]+1 }
        $fixtureJson = $bad | ConvertTo-Json
        try { $null = Invoke-Fixture; throw "VALIDATOR_BYPASSED: $field" } catch {
            if ($_.Exception.Message -ne 'The existing F4 export receipt is not the verified, source-bound complete export.') { throw }
            $rejected++
        }
    }
    $fixtureJson = $fixture | ConvertTo-Json
    $fixtureLength = [long]32522350448
    try { $null = Invoke-Fixture; throw 'WRONG_FILE_SIZE_ACCEPTED' } catch {
        if ($_.Exception.Message -notlike 'Export length differs:*') { throw }
        $rejected++
    }
    Write-Host "F5_RECEIPT_SYNTHETIC_PASS accepted=1 rejected=$rejected production_IO=none"
}

$windowDefinition = $ast.Find({ param($node)
    $node -is [System.Management.Automation.Language.FunctionDefinitionAst] -and $node.Name -eq 'Read-F5Window'
}, $true)
if (!$windowDefinition) { throw 'Window result validator missing' }
& {
    . ([scriptblock]::Create($windowDefinition.Extent.Text))
    $controllerFixture = "PHYSICAL_BACKUP phase=after files=2 directory=D:\finite-fixture\after`r`n" +
        "REVERSE_WINDOW_END exit=0 logs=E:\finite-fixture\logs N6=NOT_COMPUTED`r`n"
    $validEngine = 'SUMMARY status=INCOMPLETE_RESUMABLE new_chunks=1 new_indices=10000 closed_prefix=10000 ' +
        'total_entries=96452976 live_closed=10000 peak_rss_bytes=1024 source_checkpointreadonly=yes N6=NOT_COMPUTED'
    $engineFixture = $validEngine
    function Get-Content([string]$LiteralPath, [switch]$Raw) {
        if ($LiteralPath -eq 'controller') { return $controllerFixture }
        return $engineFixture
    }
    function Get-Item([string]$LiteralPath) { return [pscustomobject]@{Length=0} }
    function Require-File([string]$Path) { return [pscustomobject]@{Length=1} }
    $fixtureBackupCount = 2
    function Import-Csv([string]$LiteralPath) { return @(1..$fixtureBackupCount) }
    $result = Read-F5Window 'controller' 0 10000
    if ($result.Prefix -ne 10000 -or $result.NewIndices -ne 10000) { throw 'Wrong finite prefix' }
    $rejected = 0
    foreach ($change in @(
        @('closed_prefix=10000','closed_prefix=9999'),
        @('total_entries=96452976','total_entries=96452975'),
        @('new_indices=10000','new_indices=10001'),
        @('peak_rss_bytes=1024','peak_rss_bytes=999999999999'),
        @('source_checkpointreadonly=yes','source_checkpointreadonly=no'),
        @('N6=NOT_COMPUTED','N6=FAKE_RESULT'),
        @('status=INCOMPLETE_RESUMABLE','status=CLOSED_F5_CATALOGUE')
    )) {
        $engineFixture = $validEngine.Replace($change[0], $change[1])
        $caught = $false
        try { $null = Read-F5Window 'controller' 0 10000 } catch { $caught = $true }
        if (!$caught) { throw "Invalid synthetic window accepted: $($change[0])" }
        $rejected++
    }
    $engineFixture = $validEngine
    $controllerFixture = $controllerFixture.Replace('files=2','files=3')
    $caught = $false
    try { $null = Read-F5Window 'controller' 0 10000 } catch { $caught = $true }
    if (!$caught) { throw 'Wrong backup inventory accepted' }
    $rejected++
    $controllerFixture = $controllerFixture.Replace('files=3','files=2').Replace('exit=0','exit=98')
    $engineFixture = 'NO TERMINAL SUMMARY AFTER SYNTHETIC HARD STOP'
    $result = Read-F5Window 'controller' 98 10000
    if ($null -ne $result) { throw 'Bounded stop upgraded to a success result' }
    $fixtureBackupCount = 9647
    $controllerFixture = "PHYSICAL_BACKUP phase=after files=9647 directory=D:\finite-fixture\after`r`n" +
        "REVERSE_WINDOW_END exit=0 logs=E:\finite-fixture\logs N6=NOT_COMPUTED`r`n"
    $engineFixture = 'SUMMARY status=CLOSED_F5_CATALOGUE new_chunks=1 new_indices=2976 closed_prefix=96452976 ' +
        'total_entries=96452976 live_closed=96452755 peak_rss_bytes=1024 source_checkpointreadonly=yes N6=NOT_COMPUTED'
    $result = Read-F5Window 'controller' 0 10000
    if ($result.Prefix -ne 96452976 -or $result.NewIndices -ne 2976) { throw 'Final short chunk not recognized' }
    $engineFixture = $engineFixture.Replace('new_chunks=1 new_indices=2976', 'new_chunks=0 new_indices=0')
    $result = Read-F5Window 'controller' 0 10000
    if ($result.Prefix -ne 96452976 -or $result.NewIndices -ne 0) { throw 'Already-complete resume not recognized' }
    Write-Host "F5_WINDOW_SYNTHETIC_PASS accepted=3 rejected=$rejected bounded_stop_not_success=1 production_IO=none"
}
