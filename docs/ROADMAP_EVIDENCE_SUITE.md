# Roadmap Evidence Suite

This document captures the complete buyer-readiness workflow for the current roadmap phase.

## What this validates

The evidence suite checks the practical readiness of the engine across the four roadmap milestones:

1. production-core trust and support boundary
2. benchmark validation
3. flagship showcase operation
4. integration and onboarding ease

## Run it

```bash
python scripts/run_roadmap_evidence_suite.py
```

## Expected behavior

The script should:

- configure the project with deterministic settings
- build the `upe_arena` showcase target
- run the unit-test target
- execute the demo and print a deterministic hash
- run the benchmark matrix and record results into `proof/RUNS/upe_arena_benchmark_matrix.csv`

## Outcome

A successful run is the strongest in-repo evidence that the roadmap layer is operational rather than aspirational.
