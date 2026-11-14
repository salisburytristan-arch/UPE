# Roadmap Completion Status

This document records the concrete implementation work that turns the UPE roadmap from conceptual planning into a buyer-verifiable execution path.

## Phase 1 — Core trust and production boundary

Completed with artifacts:

- [docs/API_STABILITY_POLICY.md](API_STABILITY_POLICY.md) defines the supported compatibility boundary, versioning rules, replay/snapshot expectations, and public API contract.
- [README.md](../README.md) positions the repo around the production core, applied packs, and laboratory modules.
- [ExpansionRoadmap.md](../ExpansionRoadmap.md) now records roadmap progress against source-level implementation rather than only narrative plans.

## Phase 2 — Independent validation

Completed with artifacts:

- [tools/run_benchmark_matrix.py](../tools/run_benchmark_matrix.py) runs a compact benchmark matrix across multiple rigid-body scene sizes.
- [docs/BENCHMARK_BASELINES.md](BENCHMARK_BASELINES.md) defines the benchmark policy and regression philosophy.
- [docs/DETERMINISM_CERTIFICATION.md](DETERMINISM_CERTIFICATION.md) documents the validation model and proof interpretation.

## Phase 3 — Showcase application

Completed with artifacts:

- [examples/upe_arena/main.cpp](../examples/upe_arena/main.cpp) is a compact flagship demo that prints body count, frame count, step timing, FPS, and a deterministic state hash.
- [docs/UPE_ARENA_DEMO.md](UPE_ARENA_DEMO.md) explains the demo's purpose and usage.

## Phase 4 — Integration simplicity

Completed with artifacts:

- [docs/BUYER_INTEGRATION_GUIDE.md](BUYER_INTEGRATION_GUIDE.md) provides a straightforward 30-minute buyer integration path.
- [tools/run_buyer_integration_check.py](../tools/run_buyer_integration_check.py) validates the build and demo surface in one command.

## Phase 5 — Independent performance validation

In progress with artifacts:

- [tools/run_independent_benchmark_harness.py](../tools/run_independent_benchmark_harness.py) creates a comparison-ready benchmark manifest for UPE, Jolt, Bullet, Box2D, and PhysX workloads.
- [docs/INDEPENDENT_BENCHMARK_HARNESS.md](INDEPENDENT_BENCHMARK_HARNESS.md) documents the standard workload set and the honest `not_available` status used when a competitor binary is absent.
- [proof/RUNS/independent_benchmark_harness/independent_benchmark_matrix.csv](../proof/RUNS/independent_benchmark_harness/independent_benchmark_matrix.csv) records the real UPE results and shows the competitor engines are not yet provisioned in this environment.

## Evidence summary

| Milestone | Evidence | Status |
| --- | --- | --- |
| Production core boundary | API stability policy | Complete |
| Benchmark validation | benchmark matrix + baseline policy | Complete |
| Showcase demo | UPE Arena demo | Complete |
| Buyer onboarding | integration guide + validation script | Complete |
| Roadmap evidence suite | single-command configure/build/test/demo/benchmark pass | Complete |
| Independent benchmark harness | comparison-ready UPE vs candidate engines manifest | In progress |

This is the roadmap layer needed before deeper scientific or domain-pack expansion becomes a strategic product decision.
