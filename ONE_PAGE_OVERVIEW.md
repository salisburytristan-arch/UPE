# UPE - Deterministic Native Physics Foundation

## What It Is

UPE is a source-available C/C++ physics foundation focused on deterministic simulation workflows, replay/rollback validation, and buyer-verifiable build/test/proof evidence.

## Why It Is Different

- Determinism is treated as a proof surface, not a marketing claim.
- CI publishes buyer-inspectable artifacts for build, test, determinism, benchmarks, and release packaging.
- Core integration boundary is a stable C API with explicit policy documentation.

## Who It Fits

- Teams building rollback/netcode-heavy gameplay systems
- Simulation products requiring replayability and auditability
- Integrators needing native runtime control without mandatory dependency chains

## Evidence Path (Fast)

- `docs/BUYER_START_HERE.md`
- `docs/PROOF_INDEX.md`
- `docs/CI_GATES.md`
- `docs/DETERMINISM_CERTIFICATION.md`
- `docs/BENCHMARK_BASELINES.md`

## Scope and Confidence

- Maturity authority: `docs/MATURITY_MATRIX.md`
- Platform support authority: `docs/PLATFORM_SUPPORT_MATRIX.md`
- API policy authority: `docs/API_STABILITY_POLICY.md`
- Known boundaries: `docs/KNOWN_LIMITATIONS.md`

## Commercial and Diligence Inputs

- `COMMERCIAL_LICENSE_AGREEMENT.md`
- `docs/PROVENANCE_AND_OWNERSHIP.md`
- `docs/DEPENDENCY_INVENTORY.md`
- `IP_CHAIN_OF_TITLE_CHECKLIST.md`

## Recommended Next Step

Run the deterministic proof path and inspect the generated artifacts before feature-level deep dives.
