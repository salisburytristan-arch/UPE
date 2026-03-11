@echo off
setlocal enabledelayedexpansion

REM Set up Visual Studio environment
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1
if errorlevel 1 (
    echo ERROR: Could not set up MSVC environment
    exit /b 1
)

REM Run Python build
cd /d "D:\UnifiedPhysicsEngine"
python build\build.py
if errorlevel 1 (
    echo Build failed, but continuing with benchmark...
)

REM Compile benchmark if tests passed
if exist "build\out\upe.lib" (
    echo.
    echo === Compiling Benchmark Suite ===
    cl /nologo /std:c++17 /EHsc /MD /DUPE_BUILD /ID:\UnifiedPhysicsEngine\core\include ^
       benchmarks\perf_scaling_test.cpp ^
       build\out\upe.lib ^
       /Fe:build\out\upe_benchmarks.exe
    if errorlevel 1 (
        echo Benchmark compilation failed
        exit /b 1
    )
    
    echo.
    echo === Running Benchmark Suite ===
    build\out\upe_benchmarks.exe
) else (
    echo ERROR: upe.lib not found, skipping benchmark
    exit /b 1
)
