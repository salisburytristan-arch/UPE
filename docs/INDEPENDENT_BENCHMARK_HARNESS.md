# Independent Benchmark Harness

This document defines the next milestone in the roadmap: an independent, comparison-ready benchmark harness that can be extended to external engines without pretending they are installed or benchmarked when they are not.

## Purpose

The repository already contains a strong internal validation layer. The missing proof point is external credibility:

- UPE results against alternative engines under identical workloads
- standardized recurring workloads for rigid bodies, stacking, CCD, joints, vehicles, and rollback
- an honest report that records `not_available` when a competitor binary is absent instead of inventing data

## Workloads

The harness standardizes the following benchmark families:

1. Rigid-body scale
   - 100, 500, 1,000, 5,000, 10,000 bodies
2. Stacking stability
   - 10x10, 25x25, 50x50 stack profiles
3. CCD high-speed thin-obstacle workload
4. Large articulated joint structures
5. Vehicle workload scenarios
6. Rollback, snapshot, replay, and state-hash cost

## Execution model

The harness emits a report in CSV, JSON, and Markdown under `proof/RUNS/independent_benchmark_harness/`.

The runner checks for candidate engine binaries in any of these places:

- environment variables such as `UPE_BINARY`, `JOLT_BINARY`, `BULLET_BINARY`, `BOX2D_BINARY`, and `PHYSX_BINARY`
- repo-relative paths such as `build/bin/examples/upe_arena`
- repo-wide file discovery for matching benchmark names

If an external engine is not installed, it is reported as `not_available` and retains a note explaining the required benchmark or driver binary.

## Usage

```bash
python tools/run_independent_benchmark_harness.py
```

Optional repo-root override:

```bash
python tools/run_independent_benchmark_harness.py --repo-root . --output-dir proof/RUNS/independent_benchmark_harness
```

## Intended buyer-facing value

This makes the next roadmap stage operational:

- the repo can produce a comparison-ready manifest immediately
- external engines can be plugged in when they are available
- the project does not overclaim benchmark results it has not actually validated

## Current status

In this environment, UPE executes through the deterministic arena binary and records real values. The competitor engines remain `not_available`, which is the correct, honest status until those toolchains or benchmark runners are actually provided.
