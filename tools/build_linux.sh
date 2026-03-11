#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${1:-build_linux}"
TS="$(date +%Y%m%d_%H%M%S)"
LOG_DIR="proof/RUNS"
LOG_FILE="${LOG_DIR}/build_linux_${TS}.log"
mkdir -p "${LOG_DIR}"

exec > >(tee -a "${LOG_FILE}") 2>&1

echo "[UPE] build_linux started at ${TS}"
echo "[UPE] Prerequisites: cmake, ninja, and g++ or clang++"

if [[ -f "${BUILD_DIR}/CMakeCache.txt" ]]; then
  EXPECTED_SOURCE_DIR="$(pwd -P)"
  CACHED_SOURCE_DIR="$(grep -m1 '^CMAKE_HOME_DIRECTORY:INTERNAL=' "${BUILD_DIR}/CMakeCache.txt" | cut -d= -f2-)"
  if [[ -n "${CACHED_SOURCE_DIR}" && "${CACHED_SOURCE_DIR}" != "${EXPECTED_SOURCE_DIR}" ]]; then
    echo "[UPE] Detected source/cache mismatch in ${BUILD_DIR}; removing stale build directory"
    rm -rf "${BUILD_DIR}"
  fi
fi

command -v cmake >/dev/null || { echo "[UPE] Missing prerequisite: cmake"; exit 1; }
command -v ninja >/dev/null || { echo "[UPE] Missing prerequisite: ninja"; exit 1; }

if command -v g++ >/dev/null; then
  CXX_COMPILER="g++"
elif command -v clang++ >/dev/null; then
  CXX_COMPILER="clang++"
else
  echo "[UPE] Missing prerequisite: g++ or clang++"
  exit 1
fi

echo "[UPE] Using compiler: ${CXX_COMPILER}"
cmake -S . -B "${BUILD_DIR}" -G Ninja -DCMAKE_CXX_COMPILER="${CXX_COMPILER}" -DUPE_BUILD_TESTS=ON -DUPE_BUILD_EXAMPLES=ON -DUPE_DETERMINISTIC=ON
cmake --build "${BUILD_DIR}" --parallel

echo "[UPE] SUCCESS"
