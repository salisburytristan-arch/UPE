# START HERE

This repository is the developer-facing UPE source bundle. Start here if you need to evaluate how the engine is built, tested, benchmarked, verified, and packaged.

## Read In Order

1. [README.md](README.md)
2. [BUYER_PACKAGE_CONTENTS.md](BUYER_PACKAGE_CONTENTS.md)
3. [MODULE_STABILITY.md](MODULE_STABILITY.md)
4. [LIMITATIONS.md](LIMITATIONS.md)
5. [CMakeLists.txt](CMakeLists.txt)
6. [docs/INDEX.md](docs/INDEX.md)
7. [docs/PROOF_INDEX.md](docs/PROOF_INDEX.md)
8. [proof/CI_DETERMINISM_REPORT.md](proof/CI_DETERMINISM_REPORT.md)
9. [scripts/verify_package.ps1](scripts/verify_package.ps1)
10. [scripts/verify_package.sh](scripts/verify_package.sh)

## What To Inspect First

- `core/include` and `core/src` for engine architecture and runtime behavior
- `tests` for coverage and current gaps
- `benchmarks` for performance entry points and baselines
- `proof` for determinism and release evidence
- `scripts` and `tools` for build, test, proof, and packaging flows

## What This Repo Already Exposes

- Full source tree under `core/`, `examples/`, `tests/`, `benchmarks/`, `tools/`, `scripts/`, and `docs/`
- Deterministic replay and proof artifacts under `proof/`
- Build configuration and packaging entry points through CMake and helper scripts

## What The Trust Path Requires

- Build from a clean checkout
- Run tests
- Run a determinism proof
- Run at least one benchmark
- Record the result in `proof/latest_verification_log.txt`