#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${1:-build_linux}"
TS="$(date +%Y%m%d_%H%M%S)"
LOG_DIR="proof/RUNS"
LOG_FILE="${LOG_DIR}/tests_${TS}.log"
mkdir -p "${LOG_DIR}"

exec > >(tee -a "${LOG_FILE}") 2>&1

echo "[UPE] run_tests started at ${TS}"
ctest --test-dir "${BUILD_DIR}" --output-on-failure

echo "[UPE] SUCCESS"
