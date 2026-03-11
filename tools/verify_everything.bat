@echo off
setlocal

if exist ".venv\Scripts\python.exe" (
  set PYTHON_EXE=.venv\Scripts\python.exe
) else (
  set PYTHON_EXE=py -3
)

%PYTHON_EXE% scripts\verify_everything.py --release-dir releases_ci
exit /b %errorlevel%
