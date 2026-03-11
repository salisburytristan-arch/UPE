# Build Status

Current target posture:
- Primary product target: deterministic rollback-ready physics core.
- Build system: CMake, C++20.
- Test runner: CTest (`AllTests`).

## Last Known Validation (Workspace)
- Build: PASS
- Tests: PASS (`100% tests passed`)
- Determinism/perf tooling: PASS in local proof scripts

## Detailed Validation Matrix

### Source Build Paths

1. Standard workspace build
	- Command: `cmake --build build -j`
	- Result: PASS

2. Strict warning build profile
	- Configure flags: `-Wall -Wextra -Wformat=2 -Wmisleading-indentation`
	- Build directory: `build_warn`
	- Result: PASS on latest internal strict sweep; external validation may vary by compiler/version and should be re-run per package

### Test Execution

1. Standard build tree
	- Command: `ctest --test-dir build --output-on-failure -C Release`
	- Result: PASS

2. Strict warning build tree
	- Command: `ctest --test-dir build_warn --output-on-failure -C Release`
	- Result: PASS

### Runtime/Determinism Demonstrations

1. Determinism certification executable
	- Binary: `build/bin/examples/determinism_cert.exe`
	- Result: PASS (cert hash emitted)

2. Minimal runtime sanity demo
	- Binary: `build/bin/examples/minimal_2d.exe`
	- Result: PASS (world time + replay/state hash output)

### Packaging and Release Gate

1. Package create + verify
	- Command: `python scripts/verify_release_gate.py --create --dir releases_ci`
	- Result: PASS (`RELEASE GATE PASSED`)

2. Distribution cleanliness posture
	- Generated artifacts excluded from packaged source distribution.
	- Required legal/buyer/docs/proof files validated by release gate.

## Toolchain Confirmation Notes

- Windows harness build succeeded via `tools/build_windows_msvc.bat`.
- In this workspace, MSVC `cl` was unavailable, so build script used GNU fallback.
- Therefore: Windows+GNU validation is confirmed; MSVC-specific confirmation is pending environment/compiler availability.

## Critical Remediations Recently Applied

- Platform-check logic bug fixed (`char[64]` emptiness check and safe string termination).
- Uninitialized animation sampling path hardened.
- Missing enum handling for convex shape paths in query/sweep logic addressed.
- Misleading indentation hazards removed in wind-field clamping path.
- Fixed-point portability improved with MSVC-safe divide path and saturating fallback.
- Packaging exclusions expanded to avoid shipping build output clutter.

For reproducible buyer flow, use:
1. `docs/BUYER_START_HERE.md`
2. `docs/ACCEPTANCE_CHECKLIST.md`
3. `docs/STUDIO_SAFETY_VALIDATION_REPORT.md`
3. `tools/run_tests.*`, `tools/run_determinism_proof.*`, `tools/run_examples_smoketest.*`
