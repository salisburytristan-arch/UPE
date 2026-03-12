# Proof Index

This page is the buyer-facing index for CI-generated proof artifacts.

## Purpose

Use this page as the fastest path to verify current evidence, not marketing claims.
If another document conflicts with this proof surface, treat this page and the linked proof documents as authoritative.

## Build Proof

What to inspect:

- Windows, Linux, and macOS build jobs
- Per-platform test logs (`proof/ci/*tests*.log`)
- Build artifacts (`build/lib/`, `build/bin/`)

Why it matters:

- Confirms the repository builds under the declared matrix.
- Confirms tests execute with logs preserved for review.

## Determinism Proof

One-command local proof:

```bash
python scripts/run_buyer_determinism_proof.py
```

Windows alternative:

```bat
py -3 scripts\run_buyer_determinism_proof.py
```

Local proof outputs to inspect:

- `proof/RUNS/determinism_<timestamp>.log`
- `proof/RUNS/det_run1_<timestamp>.txt`
- `proof/RUNS/det_run2_<timestamp>.txt`

CI determinism artifacts to inspect:

- Linux cross-compiler logs: `gcc_output.log`, `clang_output.log`
- Windows determinism artifact lane (`upe-determinism-proof-windows`)
- macOS determinism artifact lane (`upe-determinism-proof-macos`)

Why it matters:

- Shows repeated-run hash stability.
- Exposes cross-compiler drift signals rather than hiding them.

## Benchmark Proof

What to inspect:

- `proof/ci/canonical-benchmarks.csv`
- `proof/ci/canonical-benchmarks.json`
- `proof/ci/benchmark_report.html`
- `proof/RUNS/world_step_benchmark_history.csv`

Example benchmark artifact snippet (from the JSON schema):

```json
{
  "schema": "upe.perf.world_step.v1",
  "benchmarks": [
    {
      "name": "rigid_body_512",
      "frame_time_ms": 13.868,
      "contacts": 4338.0,
      "avg_constraints": 4338.0
    }
  ]
}
```

Why it matters:

- Provides machine-readable performance evidence.
- Supports baseline-aware regression checks.

## Release Proof

What to inspect:

- `releases_ci/` artifacts
- `topic_truth_audit.csv`
- `docs/TOPIC_TRUTH_AUDIT.md`

Why it matters:

- Shows packaging and documentation integrity gates passed together.

## Latest Full-Matrix Verification Record

Use this section to record the most recent evidence-backed CI matrix pass.

- Verification date (UTC): `PENDING`
- Workflow run URL: `PENDING`
- Workflow run ID: `PENDING`
- Commit SHA verified: `d9164ee`
- Section B status in `SCIENTIFIC_COMPLETENESS_PLAN.md`: `pending CI evidence capture`

Expected lanes for this record:

- Linux Debug/Release (gcc, clang)
- Windows Debug/Release
- macOS Debug/Release
- strict warnings (gcc, clang)
- cross-compiler check
- determinism lanes (Windows, macOS)
- determinism certification
- world-step benchmarks
- release package gate

Artifacts to record after pass:

- Artifact names: `PENDING`
- Artifact IDs: `PENDING`
- Determinism artifact set: `PENDING`
- Benchmark artifact set: `PENDING`
- Release gate artifact set: `PENDING`

When this record is completed, update Section B checkboxes in `docs/SCIENTIFIC_COMPLETENESS_PLAN.md` from pending to verified.

## Related References

- [CI_GATES.md](CI_GATES.md)
- [DETERMINISM_CERTIFICATION.md](DETERMINISM_CERTIFICATION.md)
- [BENCHMARK_BASELINES.md](BENCHMARK_BASELINES.md)
- [PLATFORM_SUPPORT_MATRIX.md](PLATFORM_SUPPORT_MATRIX.md)
- [TRUTH_HIERARCHY.md](TRUTH_HIERARCHY.md)
