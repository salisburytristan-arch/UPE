@echo off
setlocal enabledelayedexpansion

if "%~1"=="" (
  echo Usage: tools\hash_package.bat ^<zip_path^> [output_file]
  exit /b 1
)

set ZIP_PATH=%~1
set OUT_FILE=%~2
if "%OUT_FILE%"=="" set OUT_FILE=proof\CLOSING_HASH_RECORD.txt

python tools\hash_package.py "%ZIP_PATH%" "%OUT_FILE%"
if errorlevel 1 exit /b 1

exit /b 0
