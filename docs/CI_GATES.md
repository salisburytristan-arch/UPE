# CI Gates

This document explains the buyer-visible proof surface enforced in continuous integration.

## Goals

The CI workflow is intended to provide visible evidence for the claims made in the buyer and technical documentation:

- Cross-platform build viability
- Repeatable test execution
- Determinism proof capture
- Benchmark reproducibility
- Release package completeness

## Current Gates

### Quick Documentation Link Gate

Runs:

```bash
python scripts/check_markdown_links.py
```

Purpose:

- Prevent broken internal documentation references from shipping.

### Platform Build Matrix

Platforms covered:

- Windows
- Linux
- macOS

Signals captured:

- Successful configure and build
- Test execution logs
- Build output artifacts

### Determinism Certification

Runs the standard test binary repeatedly and captures proof logs using the repository proof tooling.

Artifacts include:

- `proof/RUNS/`
- repeated test output logs
- proof summary values extracted from the determinism proof script

### Cross-Compiler Check

Builds and runs the test binary with GCC and Clang to expose compiler-specific determinism drift.

Artifacts include:

- `gcc_output.log`
- `clang_output.log`

### World-Step Benchmarks

Builds and runs the world-step benchmark suite and publishes:

- `proof/ci/canonical-benchmarks.csv`
- `proof/ci/canonical-benchmarks.json`
- `proof/ci/canonical-benchmarks.log`
- `proof/ci/benchmark_report.html`

This is the primary machine-readable performance proof surface.

When a world-step benchmark baseline is present, the benchmark gate also checks frame-time regressions against that baseline.

### Strict Warnings Gate

Runs the strict warnings script with GCC and Clang to keep compilation discipline visible.

### Release Package Gate

Runs the release verification flow and publishes:

- generated release artifacts
- topic truth audit artifacts

See [PROOF_INDEX.md](PROOF_INDEX.md) for the one-page artifact inspection path.

## Buyer Interpretation

For buyers, the most useful CI outputs are:

1. Determinism proof artifacts
2. World-step benchmark artifacts
3. Release gate artifacts
4. Platform build/test logs

Those outputs provide stronger evidence than feature lists alone because they can be reproduced from committed workflow steps.
