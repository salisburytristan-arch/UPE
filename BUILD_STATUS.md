# Build Status

This document reports current build and validation posture using explicit confidence tiers.
For buyer-verifiable proof artifacts, see `docs/PROOF_INDEX.md`.

## Verified Now

- Documentation link gate passes (`python scripts/check_markdown_links.py`).
- CI matrix executes build/test flows on Windows, Linux, and macOS.
- Determinism proof artifacts are generated in CI and local proof scripts.
- World-step benchmark artifacts are generated in CI with baseline-aware checks.
- Release package gate verifies package contents and topic-truth audit freshness.

## Verified With Caveats

- Strict warning posture is validated in dedicated gate jobs, but compiler/version differences can still introduce new warnings outside that exact matrix.
- Cross-compiler determinism is visible via artifact logs (`gcc_output.log`, `clang_output.log`) but is not guaranteed bit-identical.
- Cross-platform determinism lanes exist, but universal cross-platform bit-exact identity is not claimed.

## Pending Verification

- Any toolchain/platform combination not present in the current CI matrix.
- MSVC-specific local verification in environments where MSVC tooling is unavailable.
- Optional or experimental subsystem guarantees beyond the core buyer acceptance path.

## Not Claimed

- Universal bit-exact determinism across all operating systems, CPU architectures, and compiler versions.
- Equal maturity across all repository subsystems.
- Production readiness for modules marked experimental or present-not-buyer-critical.

## Reproduce Key Checks

- Build/test CI-equivalent flows: see `docs/CI_GATES.md`.
- Determinism proof: `python scripts/run_buyer_determinism_proof.py`.
- Benchmark proof: see `docs/BENCHMARK_BASELINES.md` and `scripts/run_canonical_benchmark_gate.py`.
- Release package verification: `python scripts/verify_release_gate.py --create --dir releases_ci`.

## Related References

- `docs/PROOF_INDEX.md`
- `docs/CI_GATES.md`
- `docs/DETERMINISM_CERTIFICATION.md`
- `docs/PLATFORM_SUPPORT_MATRIX.md`
- `docs/MATURITY_MATRIX.md`
