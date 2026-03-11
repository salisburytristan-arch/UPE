#!/usr/bin/env bash
set -euo pipefail

echo "[UPE] Environment diagnostic timestamp: $(date --iso-8601=seconds)"
echo "[UPE] OS:"
uname -a

echo "[UPE] CPU:"
if command -v lscpu >/dev/null; then
  lscpu | sed -n '1,20p'
else
  uname -m
fi

echo "[UPE] CMake:"
cmake --version

echo "[UPE] Ninja:"
command -v ninja
ninja --version

echo "[UPE] Compiler discovery:"
command -v g++ || true
command -v clang++ || true

echo "[UPE] Compiler versions:"
if command -v g++ >/dev/null; then g++ --version; fi
if command -v clang++ >/dev/null; then clang++ --version; fi

echo "[UPE] Python:"
python3 --version || python --version
