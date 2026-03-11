#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${1:-build_linux}"
TS="$(date +%Y%m%d_%H%M%S)"
LOG_DIR="proof/RUNS"
LOG_FILE="${LOG_DIR}/determinism_${TS}.log"
mkdir -p "${LOG_DIR}"

TEST_EXE="${BUILD_DIR}/bin/upe_tests"
if [[ ! -x "${TEST_EXE}" ]]; then
  TEST_EXE="${BUILD_DIR}/upe_tests"
fi
if [[ ! -x "${TEST_EXE}" ]]; then
  echo "[UPE] Missing test executable in ${BUILD_DIR}" | tee -a "${LOG_FILE}"
  exit 1
fi

exec > >(tee -a "${LOG_FILE}") 2>&1

echo "[UPE] determinism proof started at ${TS}"
"${TEST_EXE}" > "${LOG_DIR}/det_run1_${TS}.txt" 2>&1
"${TEST_EXE}" > "${LOG_DIR}/det_run2_${TS}.txt" 2>&1

HASH1="$(grep -m1 "Test determinism hash:" "${LOG_DIR}/det_run1_${TS}.txt" | awk '{print $4}')"
HASH2="$(grep -m1 "Test determinism hash:" "${LOG_DIR}/det_run2_${TS}.txt" | awk '{print $4}')"

if [[ -z "${HASH1}" || -z "${HASH2}" ]]; then
  echo "[UPE] Determinism hash line not found"
  echo "[UPE] FAILURE"
  exit 1
fi

echo "hash_run_1=${HASH1}"
echo "hash_run_2=${HASH2}"
if [[ "${HASH1}" == "${HASH2}" ]]; then
  echo "MATCH=true"
  echo "[UPE] SUCCESS"
  exit 0
fi

echo "MATCH=false"
echo "[UPE] FAILURE"
exit 1
