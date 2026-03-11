@echo off
setlocal

set BUILD_DIR=build_windows
set CONFIG=Release
set EXTRA_FLAGS=-DUPE_DETERMINISTIC=ON
set CMAKE_TOOLCHAIN_FLAGS=

if not exist "core\src" (
	echo [UPE] Indie headers-only package detected.
	echo [UPE] No engine implementation is included; examples are reference-only.
	set EXTRA_FLAGS=%EXTRA_FLAGS% -DUPE_BUILD_TESTS=OFF -DUPE_BUILD_EXAMPLES=OFF
) else (
	set EXTRA_FLAGS=%EXTRA_FLAGS% -DUPE_BUILD_TESTS=ON -DUPE_BUILD_EXAMPLES=ON
)

where cmake >nul 2>&1
if errorlevel 1 (
	echo [UPE] CMake was not found in PATH.
	exit /b 1
)

where cl >nul 2>&1
if not errorlevel 1 (
	echo [UPE] Using MSVC toolchain.
	set CMAKE_TOOLCHAIN_FLAGS=-G "Ninja"
	goto :configure
)

where g++ >nul 2>&1
if not errorlevel 1 (
	echo [UPE] MSVC not found. Using g++ toolchain.
	set CMAKE_TOOLCHAIN_FLAGS=-G "Ninja" -DCMAKE_CXX_COMPILER=g++
	goto :configure
)

where clang++ >nul 2>&1
if not errorlevel 1 (
	echo [UPE] MSVC/g++ not found. Using clang++ toolchain.
	set CMAKE_TOOLCHAIN_FLAGS=-G "Ninja" -DCMAKE_CXX_COMPILER=clang++
	goto :configure
)

echo [UPE] No supported C++ compiler found in PATH.
echo [UPE] Install one of: MSVC Build Tools, g++, or clang++.
exit /b 1

:configure

echo [UPE] Configuring with CMake...
cmake -S . -B "%BUILD_DIR%" %CMAKE_TOOLCHAIN_FLAGS% %EXTRA_FLAGS%
if errorlevel 1 exit /b 1

echo [UPE] Building %CONFIG%...
cmake --build "%BUILD_DIR%" --config %CONFIG% --parallel
if errorlevel 1 exit /b 1

echo [UPE] Build complete.
endlocal
