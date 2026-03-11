@echo off
setlocal enabledelayedexpansion

set BUILD_DIR=build_windows
if not "%~1"=="" set BUILD_DIR=%~1

for /f %%i in ('powershell -NoProfile -Command "Get-Date -Format yyyyMMdd_HHmmss"') do set TS=%%i
set LOG_DIR=proof\RUNS
set LOG_FILE=%LOG_DIR%\build_windows_%TS%.log
if not exist "%LOG_DIR%" mkdir "%LOG_DIR%"

echo [UPE] build_windows_msvc started at %TS% > "%LOG_FILE%"
echo [UPE] Prerequisites: cmake, ninja, and one compiler (cl/g++/clang++) >> "%LOG_FILE%"

call tools\diagnose_environment.bat >> "%LOG_FILE%" 2>&1
if errorlevel 1 goto fail

echo [UPE] Configuring and building in %BUILD_DIR% >> "%LOG_FILE%"
call build_msvc.bat "%BUILD_DIR%" >> "%LOG_FILE%" 2>&1
if errorlevel 1 goto fail

echo [UPE] SUCCESS >> "%LOG_FILE%"
echo [UPE] Log: %LOG_FILE%
exit /b 0

:fail
echo [UPE] FAILURE >> "%LOG_FILE%"
echo [UPE] Build failed. Review log: %LOG_FILE%
exit /b 1
