@echo off
setlocal

for /f %%i in ('powershell -NoProfile -Command "Get-Date -Format yyyy-MM-ddTHH:mm:ssK"') do set TS=%%i
echo [UPE] Environment diagnostic timestamp: %TS%

echo [UPE] OS:
ver

echo [UPE] CPU:
powershell -NoProfile -Command "Get-CimInstance Win32_Processor | Select-Object -ExpandProperty Name"

echo [UPE] CMake:
cmake --version

echo [UPE] Ninja:
where ninja
ninja --version

echo [UPE] Compiler discovery:
where cl
where g++
where clang++

echo [UPE] Compiler versions:
cl 2>nul
if errorlevel 1 echo [UPE] cl not available

g++ --version
if errorlevel 1 echo [UPE] g++ not available

clang++ --version
if errorlevel 1 echo [UPE] clang++ not available

echo [UPE] Python:
python --version

exit /b 0
