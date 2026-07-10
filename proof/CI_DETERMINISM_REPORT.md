# CI DETERMINISM REPORT

This report is a compact buyer-facing summary of the current determinism proof surface.

## Current Signal

- Repeated-run proof hash: `0xCC9DBEDE83059DAD`
- Repeated-run proof match: `true`
- Recorded expected hash line: `hash_run_1=0xCC9DBEDE83059DAD`
- Recorded expected hash line: `hash_run_2=0xCC9DBEDE83059DAD`

## Evidence Files

- `proof/EXPECTED_OUTPUTS/determinism_hash_expected.txt`
- `proof/CLOSING_HASH_RECORD.txt`
- `proof/REFERENCE_RUNS/`
- `proof/RUNS/`
- `tools/run_determinism_proof.bat`
- `tools/run_determinism_proof.sh`

## What This Does Not Claim

- Universal cross-platform bit-exact determinism
- Production certainty for every experimental module
- Full coverage of every future compiler and CPU combination

## Reproduction Path

1. Configure a deterministic build with tests enabled.
2. Build the test target.
3. Run the determinism proof script.
4. Inspect the generated log under `proof/RUNS/`.