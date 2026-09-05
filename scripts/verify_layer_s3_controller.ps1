param([Parameter(Mandatory=$true)][string]$C5Source)

# Test only the actual controller's isolated pure/process/locking functions.
# Never evaluate its parameter block or launch its C6 production entrypoint.
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
Set-Location $root
$scriptPath=Join-Path $PSScriptRoot 'run_layer_dp_c6_s3_finalize.ps1'
$tokens=$null;$errors=$null
$ast=[Management.Automation.Language.Parser]::ParseFile($scriptPath,[ref]$tokens,[ref]$errors)
if ($errors.Count) {throw $errors}
$names=@('Assert-SessionTime','Quoted-Arguments','Stop-ChildTree','Invoke-LoggedProcess',
    'Hold-Input','Assert-SeparateInputCopy','Assert-DistinctStages','Assert-NoReparse','Get-Sha256','Invoke-Lineage')
foreach ($name in $names) {
    $node=$ast.Find({param($n) $n -is [Management.Automation.Language.FunctionDefinitionAst] -and $n.Name -eq $name},$true)
    if (!$node) {throw "Actual controller function missing: $name"}
    Invoke-Expression $node.Extent.Text
}
$work=Join-Path $root ('data/logs/s3-controller-functions-'+[guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $work | Out-Null
Write-Host "ARTIFACTS $work"
$sessionDeadline=(Get-Date).AddMinutes(3)
$operationDeadline=$sessionDeadline.AddSeconds(-30)
$script:inputLocks=New-Object 'Collections.Generic.List[IDisposable]'
$script:heldInputNames=New-Object 'Collections.Generic.HashSet[string]' ([StringComparer]::OrdinalIgnoreCase)
function Refused([scriptblock]$Probe,[string]$Reason) {
    $failed=$false
    try { & $Probe } catch {
        if ($_.Exception.Message -notmatch [regex]::Escape($Reason)) {throw}
        $failed=$true
    }
    if (!$failed) {throw "Guard failed to reject: $Reason"}
    Write-Host "REJECT $Reason"
}
try {
    Assert-DistinctStages 'E:\a\primary' 'E:\a\replay'
    Refused {Assert-DistinctStages 'E:\a\primary' 'e:\A\PRIMARY'} 'distinct and nonoverlapping'
    Refused {Assert-DistinctStages 'E:\a\primary' 'E:\a\primary.L5.snap'} 'distinct and nonoverlapping'
    Assert-SeparateInputCopy 'E:\source.snap' 'D:\backup.snap'
    Refused {Assert-SeparateInputCopy 'E:\source.snap' 'E:\other.snap'} 'separate-volume'
    $junctionTarget=Join-Path $work 'junction-target'
    $junctionPath=Join-Path $work 'junction-alias'
    New-Item -ItemType Directory -Path $junctionTarget | Out-Null
    New-Item -ItemType Junction -Path $junctionPath -Target $junctionTarget | Out-Null
    Refused {Assert-NoReparse (Join-Path $junctionPath 'not-yet-created.snap')} 'Reparse-point'
    $helperTest='import sys;sys.path.insert(0,sys.argv[1]);import layer_s3_lineage as m;m.reject_reparse(sys.argv[2])'
    $reparseCode=Invoke-LoggedProcess 'python' @('-c',$helperTest,(Join-Path $root 'scripts'),$junctionPath) `
        (Join-Path $work 'helper-reparse.log') (Join-Path $work 'helper-reparse.stderr') 'never'
    if ($reparseCode -eq 0 -or (Get-Content -Raw (Join-Path $work 'helper-reparse.stderr')) -notmatch
        'reparse/symlink paths') {throw 'Python lineage helper did not reject junction'}
    $source=Join-Path $work 'source with spaces.L4.snap'
    [IO.File]::Copy((Resolve-Path -LiteralPath $C5Source).Path,$source,$false)
    $before=Get-Sha256 $source
    # Control-flow unit only: stub the subprocess to check that diagnostic
    # stdout cannot contaminate Invoke-FinalAttempt's returned result object.
    & {
        function Invoke-LoggedProcess($FilePath,$Arguments,$StdoutPath,$StderrPath,$SuccessPattern) {
            [IO.File]::WriteAllText($StdoutPath,'S3_LINEAGE UNIT_STUB_NO_COMPUTATION')
            return 0
        }
        $logRootPath=$work;$lineageHelper='unit-stub';$layer5Path=$source
        $Layer5Sha256=$before;$Caps='unit-stub';$ChunkParents=1;$ExpectedN='unit-stub'
        $replayBasePath='unit-stub-replay'
        $returned=@(Invoke-Lineage 'validate' 'primary' 'unit-stub-primary' 'unit-stub-dump')
        if ($returned.Count) {throw 'Lineage diagnostics contaminated returned data'}
    }
    Hold-Input $source
    Refused {$stream=[IO.File]::Open($source,[IO.FileMode]::Open,[IO.FileAccess]::Write,[IO.FileShare]::ReadWrite);$stream.Dispose()} 'being used by another process'
    Refused {[IO.File]::Delete($source)} 'being used by another process'
    $dump=Join-Path $work 'locked-source final.csv'
    $base=Join-Path $work 'locked source stage'
    $result=Invoke-LoggedProcess (Join-Path $root 'build/layer_dp_gate.exe') @(
        '5','--threads','2','--caps','200,40000,40000,1000','--load-layer','4',$source,
        '--checkpoint',$base,'0.0001','--ckpt-chunk','1000','--dump',$dump
    ) (Join-Path $work 'engine.log') (Join-Path $work 'engine.stderr') 'complete classes = 355'
    if ($result -ne 0 -or (Get-Content -Raw (Join-Path $work 'engine.log')) -notmatch
        '1903816047972624930994913280000') {throw 'Locked source/quoted path native gate failed'}
    if ((Get-Sha256 $source) -ne $before -or (Get-Sha256 ($base+'.L4.snap')) -ne $before) {
        throw 'Locked-source parent SHA changed'
    }
    Write-Host 'PASS retained-source-lock and quoted-path actual C5 contraction'
    $operationDeadline=(Get-Date).AddMilliseconds(200)
    Refused {Invoke-LoggedProcess 'python' @('-c','import time; time.sleep(30)') `
        (Join-Path $work 'deadline.log') (Join-Path $work 'deadline.stderr') 'never'} 'session computing deadline'
    Write-Host 'S3 CONTROLLER FUNCTION CHECKS PASSED'
} finally {
    foreach($stream in $script:inputLocks) {$stream.Dispose()}
}
