param(
    [string]$BuildDir = "build_verify"
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
$ProofDir = Join-Path $Root "proof"
$LogFile = Join-Path $ProofDir "latest_verification_log.txt"

New-Item -ItemType Directory -Force -Path $ProofDir | Out-Null
Set-Content -Path $LogFile -Value "UPE verification run started: $(Get-Date -Format o)"

function Write-Log {
    param([string]$Text)
    Add-Content -Path $LogFile -Value $Text
}

function Invoke-Step {
    param(
        [string]$Title,
        [string]$CommandText,
        [scriptblock]$Action
    )

    Write-Log ""
    Write-Log "=== $Title ==="
    Write-Log "Command: $CommandText"
    & $Action 2>&1 | Tee-Object -FilePath $LogFile -Append | Out-Host
    if ($LASTEXITCODE -ne 0) {
        throw "$Title failed with exit code $LASTEXITCODE"
    }
}

Invoke-Step -Title "Configure" -CommandText "cmake -S . -B $BuildDir -DCMAKE_BUILD_TYPE=Release -DUPE_BUILD_TESTS=ON -DUPE_BUILD_EXAMPLES=ON -DUPE_BUILD_BENCHMARKS=ON -DUPE_DETERMINISTIC=ON" -Action {
    cmake -S $Root -B $BuildDir -DCMAKE_BUILD_TYPE=Release -DUPE_BUILD_TESTS=ON -DUPE_BUILD_EXAMPLES=ON -DUPE_BUILD_BENCHMARKS=ON -DUPE_DETERMINISTIC=ON
}

Invoke-Step -Title "Build" -CommandText "cmake --build $BuildDir --parallel --config Release" -Action {
    cmake --build $BuildDir --parallel --config Release
}

Invoke-Step -Title "Test" -CommandText "ctest --test-dir $BuildDir --output-on-failure -C Release" -Action {
    ctest --test-dir $BuildDir --output-on-failure -C Release
}

$ProofScript = Join-Path $Root "tools\run_determinism_proof.bat"
Invoke-Step -Title "Determinism Proof" -CommandText "tools\run_determinism_proof.bat $BuildDir" -Action {
    & $ProofScript $BuildDir
}

$BenchmarkExe = Get-ChildItem -Path $BuildDir -Recurse -File -Filter "upe_bench_world_step_v2*" | Select-Object -First 1
if (-not $BenchmarkExe) {
    throw "Unable to locate upe_bench_world_step_v2 in $BuildDir"
}

Invoke-Step -Title "Benchmark" -CommandText "$($BenchmarkExe.FullName) --bodies 128 --steps 30 --warmup 10" -Action {
    & $BenchmarkExe.FullName --bodies 128 --steps 30 --warmup 10
}

Write-Log ""
Write-Log "Result: PASS"
Write-Log "Verification log written to: $LogFile"

Write-Host "[UPE] Verification complete: $LogFile"