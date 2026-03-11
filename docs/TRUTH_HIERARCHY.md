# Trust and Truth Hierarchy

Use this hierarchy whenever two documents appear to conflict.
Higher levels override lower levels.

## Level 1: Proof and Validation Authority

- `PROOF_INDEX.md`
- `CI_GATES.md`
- `DETERMINISM_CERTIFICATION.md`
- `BENCHMARK_BASELINES.md`

These define what is currently evidenced and reproducible.

## Level 2: Maturity and Support Authority

- `MATURITY_MATRIX.md`
- `PLATFORM_SUPPORT_MATRIX.md`
- `API_STABILITY_POLICY.md`
- `KNOWN_LIMITATIONS.md`

These define scope boundaries and confidence labels.

## Level 3: Operational Guidance

- Build, test, and integration guides
- Troubleshooting and implementation notes

These explain how to run workflows but do not expand claim scope.

## Level 4: Framing and Summary Documents

- Release notes
- Executive summary
- Buyer summaries

These are descriptive and must defer to Levels 1 and 2 for current truth.

## Determinism Claim Levels (Standard Terms)

Use the same terminology in all docs:

- Level 1: repeated-run deterministic (same binary and environment)
- Level 2: toolchain-scoped reproducibility (specific compiler, flags, platform)
- Level 3: cross-compiler visibility (drift inspected, not guaranteed identical)
- Level 4: cross-platform bit-exact (only claim when explicitly proven)

## Platform Label Set (Standard Terms)

Use only these labels in status tables:

- `verified-in-ci`
- `verified-locally`
- `build-config-present`
- `not-yet-validated`
- `experimental`

If a document needs older labels for readability, map them explicitly to this set.
