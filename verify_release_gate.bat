@echo off
setlocal

set PYTHON_EXE=D:/UnifiedPhysicsEngine/.venv/bin/python.exe
if exist "%PYTHON_EXE%" (
  "%PYTHON_EXE%" scripts/verify_release_gate.py --create --dir releases_buyout
) else (
  python scripts/verify_release_gate.py --create --dir releases_buyout
)

endlocal
