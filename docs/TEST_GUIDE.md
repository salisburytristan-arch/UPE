# Test Guide

This guide covers the supported validation flow after a successful configure and build.

## Prerequisite

Complete the steps in [BUILD_GUIDE.md](BUILD_GUIDE.md) first.

## Run the Core Test Suite

### Generic CTest Invocation

```bash
ctest --test-dir build --output-on-failure
```

### Windows Release Configuration

```bat
ctest --test-dir build --output-on-failure -C Release
```

This is the expected buyer-facing validation path and the same command referenced in the acceptance checklist.

## Scripted Test Runs

Repository helpers are available for local validation and CI reproduction:

### Windows

```bat
tools\run_tests.bat build
```

### Linux and macOS

```bash
tools/run_tests.sh build
```

## What the Tests Cover

The CTest entry exercises the engine's high-value runtime behavior, including:

- Collision detection and contact generation
- Constraint solving and stepping
- Snapshot and replay paths
- Determinism hashing
- Representative subsystem coverage used by the release gate

For current implementation coverage, see [IMPLEMENTATION_STATUS.md](IMPLEMENTATION_STATUS.md).

## Recommended Validation Order

1. Run CTest.
2. Run example smoke tests if you changed runtime code.
3. Run determinism proof for rollback or replay-sensitive changes.

## Related Commands

### Example Smoke Test

```bat
tools\run_examples_smoketest.bat build
```

```bash
tools/run_examples_smoketest.sh build
```

### Determinism Proof

```bat
tools\run_determinism_proof.bat build
```

```bash
tools/run_determinism_proof.sh build
```

## Failure Triage

If tests fail:

1. Rebuild from a clean `build/` directory.
2. Re-run CTest with `--output-on-failure`.
3. Check [TROUBLESHOOTING.md](TROUBLESHOOTING.md) for common environment and configuration issues.