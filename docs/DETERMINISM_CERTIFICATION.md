# Determinism Certification

This document defines the determinism proof surface for Unified Physics Engine and explains how the repository turns determinism claims into verifiable artifacts.

## Scope

The determinism certification path is intended to answer four buyer questions:

1. Does the same test binary produce the same state hash across repeated runs?
2. Are proof logs captured and preserved as artifacts?
3. Can compiler-specific drift be inspected directly?
4. Can the proof be reproduced locally with repository scripts?

## Current Certification Signals

### Repeated-Run Proof

The CI workflow runs the standard test binary repeatedly and checks whether the resulting output digests are identical.

It also runs the repository proof script:

- `tools/run_determinism_proof.sh`
- `tools/run_determinism_proof.bat`

Those scripts write proof artifacts under `proof/RUNS/`, including:

- `determinism_<timestamp>.log`
- `det_run1_<timestamp>.txt`
- `det_run2_<timestamp>.txt`

Each proof log records two extracted determinism hash values and whether the proof matched.

### Cross-Compiler Proof

The CI workflow also builds the test binary with GCC and Clang and captures their raw outputs separately.

This does not over-promise cross-compiler bit-exact identity. Instead, it makes any drift visible and reviewable.

Published artifacts include:

- `gcc_output.log`
- `clang_output.log`

### Workflow Summaries

The CI workflow publishes a summary block describing:

- repeated-run count
- unique repeated-run output count
- extracted proof hashes
- uploaded artifact locations

That summary is meant to be readable by buyers without requiring them to parse raw logs first.

## Local Reproduction

### Windows

```bat
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DUPE_BUILD_TESTS=ON -DUPE_DETERMINISTIC=ON
cmake --build build --parallel
tools\run_determinism_proof.bat build
```

### Linux and macOS

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DUPE_BUILD_TESTS=ON -DUPE_DETERMINISTIC=ON
cmake --build build --parallel
tools/run_determinism_proof.sh build
```

## Interpretation Guidance

- Matching proof hashes across repeated runs is the primary bit-exact signal.
- Cross-compiler output equality is a stronger condition, but not the only useful signal.
- Raw proof logs should always be reviewed alongside any summary line when a mismatch occurs.

## Related References

- [DETERMINISM_GUIDE.md](DETERMINISM_GUIDE.md)
- [CI_GATES.md](CI_GATES.md)
- [ACCEPTANCE_CHECKLIST.md](ACCEPTANCE_CHECKLIST.md)
