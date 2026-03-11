@echo off
setlocal

set BUILD_DIR=build
if not "%~1"=="" set BUILD_DIR=%~1

for /f %%i in ('powershell -NoProfile -Command "Get-Date -Format yyyyMMdd_HHmmss"') do set TS=%%i
set LOG_DIR=proof\RUNS
set LOG_FILE=%LOG_DIR%\science_export_perf_%TS%.log
if not exist "%LOG_DIR%" mkdir "%LOG_DIR%"

echo [UPE] run_science_export_perf_gate started at %TS% > "%LOG_FILE%"
python scripts\run_science_export_perf_gate.py --build-dir "%BUILD_DIR%" --compare-baseline >> "%LOG_FILE%" 2>&1
if errorlevel 1 goto fail

echo [UPE] SUCCESS >> "%LOG_FILE%"
echo [UPE] Log: %LOG_FILE%
exit /b 0

:fail
echo [UPE] FAILURE >> "%LOG_FILE%"
echo [UPE] Perf gate failed. Review log: %LOG_FILE%
exit /b 1
