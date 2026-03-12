@echo off
setlocal enabledelayedexpansion

set BUILD_DIR=build
if not "%~1"=="" set BUILD_DIR=%~1

for /f %%i in ('powershell -NoProfile -Command "Get-Date -Format yyyyMMdd_HHmmss"') do set TS=%%i
set LOG_DIR=proof\RUNS
set LOG_FILE=%LOG_DIR%\determinism_%TS%.log
if not exist "%LOG_DIR%" mkdir "%LOG_DIR%"

set TEST_EXE=
if exist "%BUILD_DIR%\bin\upe_tests.exe" set TEST_EXE=%BUILD_DIR%\bin\upe_tests.exe
if "%TEST_EXE%"=="" if exist "%BUILD_DIR%\bin\Release\upe_tests.exe" set TEST_EXE=%BUILD_DIR%\bin\Release\upe_tests.exe
if "%TEST_EXE%"=="" if exist "%BUILD_DIR%\bin\RelWithDebInfo\upe_tests.exe" set TEST_EXE=%BUILD_DIR%\bin\RelWithDebInfo\upe_tests.exe
if "%TEST_EXE%"=="" if exist "%BUILD_DIR%\Release\upe_tests.exe" set TEST_EXE=%BUILD_DIR%\Release\upe_tests.exe
if "%TEST_EXE%"=="" if exist "%BUILD_DIR%\RelWithDebInfo\upe_tests.exe" set TEST_EXE=%BUILD_DIR%\RelWithDebInfo\upe_tests.exe
if "%TEST_EXE%"=="" if exist "%BUILD_DIR%\upe_tests.exe" set TEST_EXE=%BUILD_DIR%\upe_tests.exe
if not exist "%TEST_EXE%" (
  echo [UPE] Missing test executable. > "%LOG_FILE%"
  echo [UPE] Build dir: %BUILD_DIR% >> "%LOG_FILE%"
  echo [UPE] Checked: >> "%LOG_FILE%"
  echo [UPE]   - %BUILD_DIR%\bin\upe_tests.exe >> "%LOG_FILE%"
  echo [UPE]   - %BUILD_DIR%\bin\Release\upe_tests.exe >> "%LOG_FILE%"
  echo [UPE]   - %BUILD_DIR%\bin\RelWithDebInfo\upe_tests.exe >> "%LOG_FILE%"
  echo [UPE]   - %BUILD_DIR%\Release\upe_tests.exe >> "%LOG_FILE%"
  echo [UPE]   - %BUILD_DIR%\RelWithDebInfo\upe_tests.exe >> "%LOG_FILE%"
  echo [UPE]   - %BUILD_DIR%\upe_tests.exe >> "%LOG_FILE%"
  echo [UPE] Missing test executable. Review log: %LOG_FILE%
  exit /b 1
)

echo [UPE] determinism proof started at %TS% > "%LOG_FILE%"
echo [UPE] Using test executable: %TEST_EXE% >> "%LOG_FILE%"
"%TEST_EXE%" > "%LOG_DIR%\det_run1_%TS%.txt" 2>&1
if errorlevel 1 goto fail
"%TEST_EXE%" > "%LOG_DIR%\det_run2_%TS%.txt" 2>&1
if errorlevel 1 goto fail

set HASH1=
set HASH2=
for /f "tokens=4" %%h in ('findstr /c:"Test determinism hash:" "%LOG_DIR%\det_run1_%TS%.txt"') do set HASH1=%%h
for /f "tokens=4" %%h in ('findstr /c:"Test determinism hash:" "%LOG_DIR%\det_run2_%TS%.txt"') do set HASH2=%%h

if "%HASH1%"=="" goto fail
if "%HASH2%"=="" goto fail

echo hash_run_1=!HASH1! >> "%LOG_FILE%"
echo hash_run_2=!HASH2! >> "%LOG_FILE%"
if /I "!HASH1!"=="!HASH2!" (
  echo MATCH=true >> "%LOG_FILE%"
  echo [UPE] SUCCESS >> "%LOG_FILE%"
  echo [UPE] Log: %LOG_FILE%
  exit /b 0
)

echo MATCH=false >> "%LOG_FILE%"

:fail
echo [UPE] FAILURE >> "%LOG_FILE%"
echo [UPE] Determinism proof failed. Review log: %LOG_FILE%
exit /b 1
