#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 1 ]]; then
  echo "Usage: tools/hash_package.sh <zip_path> [output_file]"
  exit 1
fi

ZIP_PATH="$1"
OUT_FILE="${2:-proof/CLOSING_HASH_RECORD.txt}"

if command -v python3 >/dev/null 2>&1; then
  python3 tools/hash_package.py "${ZIP_PATH}" "${OUT_FILE}"
elif command -v python >/dev/null 2>&1; then
  python tools/hash_package.py "${ZIP_PATH}" "${OUT_FILE}"
else
  echo "[UPE] Python is required for hash_package.sh"
  exit 1
fi
