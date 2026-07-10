param(
    [Parameter(Mandatory = $true)]
    [string]$PackagePath,
    [string]$ExpectedRecordPath = ""
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot

if (-not $ExpectedRecordPath) {
    $ExpectedRecordPath = Join-Path $Root "proof\CLOSING_HASH_RECORD.txt"
}

if (-not (Test-Path $PackagePath)) {
    throw "Package not found: $PackagePath"
}

$TempRecord = Join-Path ([System.IO.Path]::GetTempPath()) ("upe_hash_{0}.txt" -f ([guid]::NewGuid().ToString("N")))
try {
    python (Join-Path $Root "tools\hash_package.py") $PackagePath $TempRecord

    if (-not (Test-Path $ExpectedRecordPath)) {
        Write-Host "[UPE] No expected record found, wrote: $TempRecord"
        exit 0
    }

    $Expected = Get-Content $ExpectedRecordPath | Where-Object { $_ -match '^(package_sha256|zip_manifest_sha256)=' }
    $Actual = Get-Content $TempRecord | Where-Object { $_ -match '^(package_sha256|zip_manifest_sha256)=' }

    if (($Expected | Measure-Object).Count -eq 0) {
        throw "Expected record missing hash fields: $ExpectedRecordPath"
    }
    if (($Actual | Measure-Object).Count -eq 0) {
        throw "Generated record missing hash fields: $TempRecord"
    }

    if (@($Expected) -ne @($Actual)) {
        throw "Checksum mismatch between $ExpectedRecordPath and generated record"
    }

    Write-Host "[UPE] Checksum verification passed against $ExpectedRecordPath"
} finally {
    if (Test-Path $TempRecord) {
        Remove-Item $TempRecord -Force
    }
}