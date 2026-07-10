#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${1:-build_verify}"
PROOF_DIR="${ROOT_DIR}/proof"
LOG_FILE="${PROOF_DIR}/latest_verification_log.txt"

mkdir -p "${PROOF_DIR}"
: > "${LOG_FILE}"

write_log() {
  printf '%s\n' "$1" | tee -a "${LOG_FILE}"
}

run_step() {
  local title="$1"
  shift
  local command_text="$1"
  shift
  write_log ""
  write_log "=== ${title} ==="
  write_log "Command: ${command_text}"
  "$@" 2>&1 | tee -a "${LOG_FILE}"
}

run_step "Configure" "cmake -S . -B ${BUILD_DIR} -DCMAKE_BUILD_TYPE=Release -DUPE_BUILD_TESTS=ON -DUPE_BUILD_EXAMPLES=ON -DUPE_BUILD_BENCHMARKS=ON -DUPE_DETERMINISTIC=ON" \
  cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release -DUPE_BUILD_TESTS=ON -DUPE_BUILD_EXAMPLES=ON -DUPE_BUILD_BENCHMARKS=ON -DUPE_DETERMINISTIC=ON

run_step "Build" "cmake --build ${BUILD_DIR} --parallel" \
  cmake --build "${BUILD_DIR}" --parallel

run_step "Test" "ctest --test-dir ${BUILD_DIR} --output-on-failure" \
  ctest --test-dir "${BUILD_DIR}" --output-on-failure

if [[ -x "${ROOT_DIR}/tools/run_determinism_proof.sh" ]]; then
  run_step "Determinism Proof" "tools/run_determinism_proof.sh ${BUILD_DIR}" \
    "${ROOT_DIR}/tools/run_determinism_proof.sh" "${BUILD_DIR}"
else
  run_step "Determinism Proof" "tools/run_determinism_proof.bat ${BUILD_DIR}" \
    cmd.exe /c "\"${ROOT_DIR}/tools/run_determinism_proof.bat\" ${BUILD_DIR}"
fi

BENCH_EXE="$(find "${BUILD_DIR}" -type f \( -name 'upe_bench_world_step_v2' -o -name 'upe_bench_world_step_v2.exe' \) | head -n 1)"
if [[ -z "${BENCH_EXE}" ]]; then
  echo "Unable to locate upe_bench_world_step_v2 in ${BUILD_DIR}" | tee -a "${LOG_FILE}"
  exit 1
fi

run_step "Benchmark" "${BENCH_EXE} --bodies 128 --steps 30 --warmup 10" \
  "${BENCH_EXE}" --bodies 128 --steps 30 --warmup 10

write_log ""
write_log "Result: PASS"
write_log "Verification log written to: ${LOG_FILE}"

echo "[UPE] Verification complete: ${LOG_FILE}"