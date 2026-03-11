# Acceptance Checklist

A package is considered accepted when the following are true on evaluator hardware.

## Core Build + Test
- [ ] Build succeeds from clean checkout.
- [ ] `ctest --test-dir build --output-on-failure -C Release` reports pass.

Detailed acceptance criteria:

- [ ] Build is reproducible in a **fresh** build directory (not reusing checked-in build outputs).
- [ ] Compiler diagnostics are acceptable for buyer policy (`-Werror` environments should be considered in evaluation).
- [ ] No blocker warnings indicating potential undefined behavior in critical runtime paths.

## Determinism and Rollback Tooling
- [ ] `tools/run_determinism_proof.*` completes successfully.
- [ ] Replay/hash outputs are produced in `proof/` logs.

Detailed acceptance criteria:

- [ ] `determinism_cert` executes to completion and emits final cert hash.
- [ ] Replay/state hash flow is exercised in at least one minimal demo executable.
- [ ] Determinism outputs are archived under `proof/RUNS/` and/or `proof/REFERENCE_RUNS/`.

## Demo Execution
- [ ] `minimal_3d` runs.
- [ ] `network_demo` runs.
- [ ] `determinism_cert` runs.

Detailed acceptance criteria:

- [ ] Demos execute without immediate crash/assert on evaluator machine.
- [ ] Console output includes expected telemetry/status lines where applicable.
- [ ] At least one minimal sample confirms world-step progression and hash/digest output.

## Performance / Benchmark
- [ ] `scripts/run_science_export_perf_gate.py --compare-baseline` passes.
- [ ] Output artifacts are generated in `proof/RUNS/`.

Detailed acceptance criteria:

- [ ] Performance gate compares against baseline with clear pass/fail report.
- [ ] Benchmark outputs include timestamped artifacts and are traceable to build/toolchain.
- [ ] No unexplained severe regressions in core throughput metrics for target workloads.

## Documentation Completeness
- [ ] `BUYER_START_HERE.md` path is reproducible.
- [ ] `UPE_CORE_SCOPE.md` clearly separates core vs modules.
- [ ] `FEATURE_MATRIX.md` matches observed runtime behavior.

Detailed acceptance criteria:

- [ ] Build, test, determinism, and troubleshooting docs match actual commands used.
- [ ] Known limitations are explicit and current.
- [ ] Release notes and status docs align with observed validation outcomes.

## Legal / Distribution
- [ ] License and distribution policy files are present.
- [ ] Chain-of-title checklist file is present.

Detailed acceptance criteria:

- [ ] Release package excludes generated build artifacts (`build/`, `bin/`, `lib/`, `CMakeFiles/`, etc.).
- [ ] Commercial/license files are present and readable in package root.
- [ ] Release gate verification passes for packaged zip(s).

## Toolchain Confidence (Recommended)

- [ ] At least one Windows toolchain build/test path verified.
- [ ] Preferred: explicit MSVC (`cl`) validation when targeting Windows studio buyers.
- [ ] If fallback compiler is used, it is documented in proof logs.
