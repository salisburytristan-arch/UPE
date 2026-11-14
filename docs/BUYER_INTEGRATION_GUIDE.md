# Buyer Integration Guide

This guide is the "30-minute integration" step of the roadmap: a buyer should be able to clone the repo, configure a build, and run one deterministic core demo without needing custom patches or undocumented setup.

## 1. Prerequisites

- CMake 3.15+
- C++20 compiler
- Git

## 2. Configure the project

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DUPE_BUILD_EXAMPLES=ON -DUPE_BUILD_TESTS=ON -DUPE_DETERMINISTIC=ON
```

## 3. Build the core and demo

```bash
cmake --build build --parallel
```

## 4. Run the flagship showcase

```bash
./build/bin/examples/upe_arena
```

On Windows:

```bat
build\bin\examples\upe_arena.exe
```

## 5. Run the benchmark matrix

```bash
python tools/run_benchmark_matrix.py --build-dir build --grid-sizes 2,3,4,5 --frames 60
```

## 6. Check the deterministic proof surface

```bash
python scripts/run_buyer_determinism_proof.py
```

## 7. Notes for buyers

- The supported path is the deterministic core surface, not the full lab/science area.
- The benchmark script gives a repeatable performance signal for a fixed scene family.
- The example is intentionally small and easy to understand, making it suitable for early proof and onboarding.

This is the commercial-facing onboarding path: build, run, verify, and compare. That is the practical integration experience required for the roadmap's value expansion phase.
