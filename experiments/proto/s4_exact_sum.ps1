param(
    [Parameter(Mandatory = $true, Position = 0)]
    [string]$Dump,

    [Parameter(Mandatory = $true)]
    [ValidateRange(1, 1000000)]
    [int]$Classes,

    [string]$ExpectN = ""
)

# Independent .NET BigInteger cross-check for the final layer-DP square sum.
# This intentionally shares no parsing or arithmetic code with
# s4_exact_sum.py or s4_certificate_verify.py.

$ErrorActionPreference = "Stop"

function Convert-ToBigInteger {
    param(
        [Parameter(Mandatory = $true)][string]$Text,
        [Parameter(Mandatory = $true)][string]$Label
    )
    $value = [Numerics.BigInteger]::Zero
    if (![Numerics.BigInteger]::TryParse(
            $Text,
            [Globalization.NumberStyles]::Integer,
            [Globalization.CultureInfo]::InvariantCulture,
            [ref]$value)) {
        throw "$Label is not an integer: $Text"
    }
    return $value
}

function Get-Binomial {
    param([int]$N, [int]$K)
    if ($K -lt 0 -or $K -gt $N) { return [Numerics.BigInteger]::Zero }
    $K = [Math]::Min($K, $N - $K)
    [Numerics.BigInteger]$answer = 1
    for ($i = 1; $i -le $K; $i++) {
        $answer = ($answer * ($N - $K + $i)) / $i
    }
    return $answer
}

try {
    $resolved = (Resolve-Path -LiteralPath $Dump).Path
    $rows = @(Import-Csv -LiteralPath $resolved)
    if ($rows.Count -ne $Classes) {
        throw "class count $($rows.Count) != expected $Classes"
    }
    $expectedHeader = @(
        "qid",
        "representative_words",
        "coordinate_orbit_size",
        "labelled_multiplicity",
        "F"
    )
    if (!$rows.Count) { throw "certificate has no rows" }
    $actualHeader = @($rows[0].PSObject.Properties.Name)
    if (($actualHeader -join "`n") -ne ($expectedHeader -join "`n")) {
        throw "unexpected CSV header: $($actualHeader -join ',')"
    }

    $wordCount = -1
    [Numerics.BigInteger]$mass = 0
    [Numerics.BigInteger]$total = 0
    for ($i = 0; $i -lt $rows.Count; $i++) {
        $row = $rows[$i]
        $qid = 0
        if (![int]::TryParse($row.qid, [ref]$qid) -or $qid -ne $i) {
            throw "row $($i + 2): qid $($row.qid) != $i"
        }
        $words = @($row.representative_words -split ' ' |
            Where-Object { $_ -ne "" })
        if ($wordCount -lt 0) {
            $wordCount = $words.Count
            if ($wordCount -le 0 -or ($wordCount % 2)) {
                throw "row 2: representative width $wordCount is not positive even"
            }
        }
        elseif ($words.Count -ne $wordCount) {
            throw "row $($i + 2): representative width $($words.Count) " +
                  "!= $wordCount"
        }
        foreach ($word in $words) {
            $parsedWord = 0
            if (![int]::TryParse($word, [ref]$parsedWord)) {
                throw "row $($i + 2): non-integer representative word $word"
            }
        }
        $m = Convert-ToBigInteger $row.coordinate_orbit_size `
            "row $($i + 2) coordinate_orbit_size"
        $ell = Convert-ToBigInteger $row.labelled_multiplicity `
            "row $($i + 2) labelled_multiplicity"
        $f = Convert-ToBigInteger $row.F "row $($i + 2) F"
        if ($m -le 0 -or $ell -le 0 -or $f -lt 0) {
            throw "row $($i + 2): m/ell must be positive and F nonnegative"
        }
        $mass += $m * $ell
        $total += $m * $ell * $f * $f
    }

    $C = [int]($wordCount / 2)
    [Numerics.BigInteger]$expectedMass = 1
    $choose = Get-Binomial (2 * $C) $C
    for ($i = 0; $i -lt $C; $i++) { $expectedMass *= $choose }
    if ($mass -ne $expectedMass) {
        throw "sum(m*ell) $mass != binomial($(2 * $C),$C)^$C = " +
              "$expectedMass"
    }

    Write-Output "dump = $resolved"
    Write-Output "C = $C"
    Write-Output "classes = $($rows.Count)  OK"
    Write-Output "sum(m*ell) = $mass  OK"
    Write-Output "N($C) = $total"
    if ($ExpectN) {
        $expected = Convert-ToBigInteger $ExpectN "ExpectN"
        if ($total -ne $expected) {
            throw "N($C) $total != expected $expected"
        }
        Write-Output "N check vs expected $expected`: PASS"
    }
    Write-Output "POWERSHELL EXACT SUM PASS"
}
catch {
    Write-Error "POWERSHELL EXACT SUM FAIL: $($_.Exception.Message)"
    exit 2
}
