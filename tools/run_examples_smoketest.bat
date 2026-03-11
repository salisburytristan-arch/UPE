@echo off
setlocal enabledelayedexpansion

set BUILD_DIR=build
if not "%~1"=="" set BUILD_DIR=%~1
set EXAMPLES_DIR=%BUILD_DIR%\bin\examples

for /f %%i in ('powershell -NoProfile -Command "Get-Date -Format yyyyMMdd_HHmmss"') do set TS=%%i
set LOG_DIR=proof\RUNS
set LOG_FILE=%LOG_DIR%\examples_smoketest_%TS%.log
if not exist "%LOG_DIR%" mkdir "%LOG_DIR%"

echo [UPE] examples smoketest started at %TS% > "%LOG_FILE%"
if not exist "%EXAMPLES_DIR%" (
  echo [UPE] Missing examples directory: %EXAMPLES_DIR% >> "%LOG_FILE%"
  exit /b 1
)

set COUNT=0
for %%N in (minimal_2d minimal_3d) do (
  if not exist "%EXAMPLES_DIR%\%%N.exe" (
    echo [UPE] Missing allowlisted example: %%N.exe >> "%LOG_FILE%"
    exit /b 1
  )
  set /a COUNT+=1
  echo [UPE] Running %%N.exe >> "%LOG_FILE%"
  "%EXAMPLES_DIR%\%%N.exe" >> "%LOG_FILE%" 2>&1
  if errorlevel 1 goto fail
)

echo [UPE] SUCCESS >> "%LOG_FILE%"
echo [UPE] Log: %LOG_FILE%
exit /b 0

:fail
echo [UPE] FAILURE >> "%LOG_FILE%"
echo [UPE] Examples smoketest failed. Review log: %LOG_FILE%
exit /b 1
