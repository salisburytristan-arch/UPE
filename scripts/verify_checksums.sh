#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 1 ]]; then
  echo "Usage: scripts/verify_checksums.sh <package_zip> [expected_record]"
  exit 1
fi

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PACKAGE_PATH="$1"
EXPECTED_RECORD="${2:-${ROOT_DIR}/proof/CLOSING_HASH_RECORD.txt}"
TEMP_RECORD="$(mktemp)"

cleanup() {
  rm -f "${TEMP_RECORD}"
}
trap cleanup EXIT

if [[ ! -f "${PACKAGE_PATH}" ]]; then
  echo "Package not found: ${PACKAGE_PATH}"
  exit 1
fi

python3 "${ROOT_DIR}/tools/hash_package.py" "${PACKAGE_PATH}" "${TEMP_RECORD}" || python "${ROOT_DIR}/tools/hash_package.py" "${PACKAGE_PATH}" "${TEMP_RECORD}"

if [[ ! -f "${EXPECTED_RECORD}" ]]; then
  echo "[UPE] No expected record found, wrote: ${TEMP_RECORD}"
  exit 0
fi

expected_hash=$(grep -m1 '^package_sha256=' "${EXPECTED_RECORD}" | cut -d= -f2-)
expected_manifest=$(grep -m1 '^zip_manifest_sha256=' "${EXPECTED_RECORD}" | cut -d= -f2-)
actual_hash=$(grep -m1 '^package_sha256=' "${TEMP_RECORD}" | cut -d= -f2-)
actual_manifest=$(grep -m1 '^zip_manifest_sha256=' "${TEMP_RECORD}" | cut -d= -f2-)

if [[ -z "${expected_hash}" || -z "${expected_manifest}" || -z "${actual_hash}" || -z "${actual_manifest}" ]]; then
  echo "[UPE] Missing hash fields in checksum records"
  exit 1
fi

if [[ "${expected_hash}" != "${actual_hash}" || "${expected_manifest}" != "${actual_manifest}" ]]; then
  echo "[UPE] Checksum mismatch between expected record and generated record"
  exit 1
fi

echo "[UPE] Checksum verification passed against ${EXPECTED_RECORD}"