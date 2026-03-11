# Benchmark Baselines

This document describes how world-step performance baselines are handled for UPE.

## Current Policy

World-step benchmarks are run in CI and published as artifacts in CSV, JSON, log, and HTML form.

Regression checking is baseline-aware:

- If `proof/EXPECTED_OUTPUTS/world_step_benchmark_baseline.json` exists, CI compares current benchmark frame times against it.
- If the baseline is missing, CI still publishes artifacts and trend history but skips regression failure.

That policy avoids inventing a fake baseline while still making the benchmark surface immediately usable.

## Files

- Baseline file: `proof/EXPECTED_OUTPUTS/world_step_benchmark_baseline.json`
- Trend history: `proof/RUNS/world_step_benchmark_history.csv`
- Trend snapshots: `proof/RUNS/world_step_benchmark_trend_<timestamp>.json`

## Seeding a Baseline

Recommended process:

1. Run the world-step benchmark suite on the reference CI configuration.
2. Review the CSV, JSON, and HTML outputs.
3. Promote the approved JSON artifact to `proof/EXPECTED_OUTPUTS/world_step_benchmark_baseline.json`.
4. Keep the regression threshold conservative until CI variance is understood.

## Threshold Guidance

The current world-step benchmark workflow uses a `20%` regression threshold.

That threshold should be tightened only after several green runs establish stable CI behavior.

## Related References

- [CI_GATES.md](CI_GATES.md)
- [DETERMINISM_CERTIFICATION.md](DETERMINISM_CERTIFICATION.md)
