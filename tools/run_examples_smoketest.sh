#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${1:-build_linux}"
EXAMPLES_DIR="${BUILD_DIR}/bin/examples"
TS="$(date +%Y%m%d_%H%M%S)"
LOG_DIR="proof/RUNS"
LOG_FILE="${LOG_DIR}/examples_smoketest_${TS}.log"
mkdir -p "${LOG_DIR}"

exec > >(tee -a "${LOG_FILE}") 2>&1

echo "[UPE] examples smoketest started at ${TS}"
if [[ ! -d "${EXAMPLES_DIR}" ]]; then
  echo "[UPE] Missing examples directory: ${EXAMPLES_DIR}"
  exit 1
fi

count=0
ALLOWLIST=("minimal_2d" "minimal_3d" "collision_demo")

for name in "${ALLOWLIST[@]}"; do
  exe="${EXAMPLES_DIR}/${name}"
  if [[ ! -x "${exe}" ]]; then
    echo "[UPE] Missing allowlisted example: ${name}"
    exit 1
  fi

  echo "[UPE] Running ${name}"
  if ! timeout 15s "${exe}"; then
    echo "[UPE] Example failed or timed out: ${name}"
    exit 1
  fi
  count=$((count + 1))
done

if [[ ${count} -ne ${#ALLOWLIST[@]} ]]; then
  echo "[UPE] Unexpected smoketest count: ${count}"
  exit 1
fi

echo "[UPE] SUCCESS"
