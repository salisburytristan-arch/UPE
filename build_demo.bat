@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1
cl /nologo /std:c++17 /EHsc /MD /DUPE_BUILD /ID:\UnifiedPhysicsEngine\core\include D:\UnifiedPhysicsEngine\examples\collision_demo\main.cpp D:\UnifiedPhysicsEngine\build\out\upe.lib /FeD:\UnifiedPhysicsEngine\build\out\collision_demo.exe
